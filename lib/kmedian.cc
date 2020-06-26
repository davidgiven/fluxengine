#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include <algorithm>
#include <numeric>
#include <math.h>
#include <limits.h>

/* Implement a k * n log n method for optimal 1D K-median.
 * The K-median problem consists of finding k clusters so that the
 * sum of absolute distances from each of the n points to the closest
 * cluster is minimized.
 *
 * GRØNLUND, Allan, et al. Fast exact k-means, k-medians and Bregman
 * divergence clustering in 1d. arXiv preprint arXiv:1701.07204, 2017.
 */

class KCluster
{
public:
	int numPoints;                    /* number of points */
	std::vector<float> points;        /* sorted list of points */
	std::vector<float> cumulativeSum; /* cumulative sum of points */

	KCluster(const std::vector<float>& inputPoints)
	{
		points = inputPoints;
		std::sort(points.begin(), points.end());

		numPoints = points.size();
		cumulativeSum.resize(numPoints);
		std::partial_sum(points.begin(), points.end(), cumulativeSum.begin());
	}

	/* Returns the median point from the sorted list of points in the supplied
	 * half-open range. */

	float medianPoint(int low, int high)
	{
		float m = (high + low - 1) / 2.0;
		return (points[(int)floor(m)] + points[(int)ceil(m)]) / 2.0;
	}

	 /* Grønlund et al.'s CC function for the K-median problem.
	  *
	  * CC(i,j) is the cost of grouping points_i...points_j into one cluster with the
	  * optimal cluster point (the median point). By programming convention, the
	  * interval is half-open and indexed from 0, unlike the paper's convention
	  * of closed intervals indexed from 1.
	  */

	float CC(int i, int j)
	{
		if (i >= j)
			return 0;

		float m = (j + i - 1) / 2.0;
		float mu = (points[(int)floor(m)] + points[(int)ceil(m)]) / 2.0;

		/* Lower part: entry i to median point between i and j, median excluded. */

		int sum_below = cumulativeSum[(int)m];
		if (i > 0)
			sum_below -= cumulativeSum[i-1];

		/* Upper part: everything from the median up, including the median if it's a
		 * real point. */

		int sum_above = cumulativeSum[j-1] - cumulativeSum[(int)m];

		return floor(m - i + 1.0)*mu - sum_below + sum_above - ceil(j - m - 1.0)*mu;
	}

	/* D_previous is the D vector for (i-1) clusters, or empty if i < 2.
	 * It's possible to do this even faster (and more incomprehensibly).
	 * See Grønlund et al. for that. */

	/* Calculate C_i[m][j] given D_previous = D[i-1]. p. 4 */

	float C_i(int i, const std::vector<float>& D_previous, int m, int j)
	{
		switch (i)
		{
			case 1:
				return CC(0, m);

			case 2:
				return CC(0, std::min(j, m)) + CC(j, m);

			default:
				return D_previous[std::min(j, m)] + CC(j, m);
		}
	}

	/* Find the (location of the) minimum value of each row of an nxn matrix in
	 * n log n time, given that the matrix is monotone (by the definition of
	 * Grønlund et al.) and defined by a function M that takes row and column
	 * as parameters. All boundary values are closed, [minRow, maxRow]
	 * p. 197 of
	 * AGGARWAL, Alok, et al. Geometric applications of a matrix-searching
	 * algorithm. Algorithmica, 1987, 2.1-4: 195-208.
	 */

	void monotoneMatrixIndices(std::function<float(int, int)>& M,
			int minRow, int maxRow, int minCol, int maxCol, 
			std::vector<int>& Tout,
			std::vector<float>& Dout)
	{
		if (maxRow == minRow)
			return;

		int currentRow = minRow + (maxRow-minRow)/2;

		/* Find the minimum column for this row. */

		float minValue = INT_MAX;
		int minHere = minCol;
		for (int i = minCol; i<=maxCol; i++)
		{
			float v = M(currentRow, i);
			if (v < minValue)
			{
				minValue = v;
				minHere = i;
			}
		}

		if (Tout[currentRow])
			throw std::runtime_error("tried to set a variable already set");

		Tout[currentRow] = minHere;
		Dout[currentRow] = M(currentRow, minHere);

		/* No lower row can have a minimum to the right of the current minimum.
		 * Recurse on that assumption. */

		monotoneMatrixIndices(M, minRow, currentRow, minCol, minHere+1, Tout, Dout);

		/* And no higher row can have a minimum to the left. */

		monotoneMatrixIndices(M, currentRow+1, maxRow, minHere, maxCol, Tout, Dout);
	}

	/* Calculates the optimal cluster centres for the points. */

	std::vector<float> optimalKMedian(int numClusters)
	{
		std::vector<std::vector<int>> T(numClusters+1);
		std::vector<std::vector<float>> D(numClusters+1);

		for (int i=0; i<numClusters+1; i++)
		{
			T[i].resize(numPoints + 1);
			D[i].resize(numPoints + 1);
		}

		for (int i=1; i<numClusters+1; i++)
		{
			/* Stop if we achieve optimal clustering with fewer clusters than the
			 * user asked for. */
			
			std::vector<float>& lastD = D[i-1];
			if ((i != 1) && (lastD.back() == 0.0))
				continue;

			std::function<float(int, int)> M = [&](int m, int j) {
				return C_i(i, lastD, m, j);
			};

			monotoneMatrixIndices(M, i, numPoints+1, i-1, numPoints+1,
				T[i], D[i]);
		}

		/* Backtrack. The last cluster has to encompass everything, so the
		 * rightmost boundary of the last cluster is the last point in the
		 * array, hence given by the last position of T. Then the previous
		 * cluster transition boundary is given by where that cluster starts,
		 * and so on.
		 */

		int currentClusteringRange = numPoints;
		std::vector<float> centres;

		for (int i=numClusters; i>0; i--)
		{
			int newClusterRange = T[i][currentClusteringRange];

			centres.push_back(medianPoint(newClusterRange, currentClusteringRange));
			currentClusteringRange = newClusterRange;
		}
	
		std::sort(centres.begin(), centres.end());
		return centres;
	}
};
	
static std::vector<float> getPointsFromFluxmap(const Fluxmap& fluxmap)
{
	FluxmapReader fmr(fluxmap);
	std::vector<float> points;
	while (!fmr.eof())
	{
		float point = fmr.findEvent(F_BIT_PULSE) * US_PER_TICK;
		points.push_back(point);
	}
	return points;
}

/* Analyses the fluxmap and determines the optimal cluster centres for it. The
 * number of clusters is taken from the size of the output array. */

std::vector<float> optimalKMedian(const std::vector<float>& points, int clusters)
{
	KCluster kcluster(points);
	return kcluster.optimalKMedian(clusters);
}

std::vector<float> optimalKMedian(const Fluxmap& fluxmap, int clusters)
{
	return optimalKMedian(getPointsFromFluxmap(fluxmap), clusters);
}

