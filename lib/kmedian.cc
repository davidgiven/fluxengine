#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "kmedian.h"
#include <algorithm>
#include <numeric>
#include <math.h>
#include <limits.h>

/* Implement a O(kn log n) method for optimal 1D K-median. The K-median problem
 * consists of finding k clusters so that the sum of absolute distances from
 * each of the n points to the closest cluster is minimized.
 *
 * GRØNLUND, Allan, et al. Fast exact k-means, k-medians and Bregman divergence
 * clustering in 1d. arXiv preprint arXiv:1701.07204, 2017.
 *
 * The points all have one of a very small range of values (compared to the
 * number of points). Since k-medians clustering is either a point or the mean
 * of two points, we can then do calculations with n being the number of
 * distinct points, rather than the number of points, so that O(kn log n)
 * becomes much quicker in concrete terms.
 *
 * Finding the median takes O(log n) time instead of O(1) time, which increases
 * the constant factor in front of the kn log n term.
 */

class KCluster
{
public:
    struct CDF
    {
        int pointsSoFar;
        KValue sumToHere;
        KValue thisValue;
    };
        
    int uniquePoints;            /* number of unique points */
    std::vector<struct CDF> cdf; /* cumulative sum of points */

    KCluster(const std::vector<KValue>& inputPoints)
    {
        std::vector<KValue> points = inputPoints;
        std::sort(points.begin(), points.end());

        KValue current = NAN;
        int count = 0;
        KValue sum = 0.0;
        for (KValue point : points)
        {
            if (point != current)
            {
                struct CDF empty = {0};
                cdf.push_back(empty);
            }

            sum += point;
            count++;

            auto& thiscdf = cdf.back();
            thiscdf.pointsSoFar = count;
            thiscdf.sumToHere = sum;
            thiscdf.thisValue = point;
            current = point;
        }

        uniquePoints = cdf.size();
    }

    /* Determines the median point of all points between the ith category
     * in cdf (inclusive), and the jth (exclusive). It's used to
     * reconstruct clusters later.
     */

    KValue medianPoint(int i, int j)
    {
        if (i >= j)
            return 0;

        int lowerCount = 0;
        if (i > 0)
            lowerCount = cdf[i-1].pointsSoFar;
        int upperCount = cdf[j-1].pointsSoFar;

        /* Note, this is not the index, but number of points start inclusive,
         * which is why the -1 has to be turned into +1. */

        KValue medianPoint = (lowerCount + upperCount + 1) / 2.0;
        
        auto lowerMedian = std::lower_bound(cdf.begin(), cdf.end(), (int)medianPoint,
            [&](const struct CDF& lhs, int rhs) {
                return lhs.pointsSoFar < rhs;
            }
        );

        if (((int)ceil(medianPoint) == (int)floor(medianPoint))
                || ((int)floor(medianPoint) != lowerMedian->pointsSoFar))
            return lowerMedian->thisValue;
        else
            return (lowerMedian->thisValue + (lowerMedian+1)->thisValue) / 2.0;
    }

    /* Find the (location of the) minimum value of each row of an nxn matrix in
     * n log n time, given that the matrix is monotone (by the definition of
     * Grønlund et al.) and defined by a function M that takes row and column
     * as parameters. All boundary values are closed, [minRow, maxRow]
     * p. 197 of
     * AGGARWAL, Alok, et al. Geometric applications of a matrix-searching
     * algorithm. Algorithmica, 1987, 2.1-4: 195-208.
     */

    void monotoneMatrixIndices(std::function<KValue(int, int)>& M,
            int minRow, int maxRow, int minCol, int maxCol, 
            std::vector<int>& Tout,
            std::vector<KValue>& Dout)
    {
        if (maxRow == minRow)
            return;

        int currentRow = minRow + (maxRow-minRow)/2;

        /* Find the minimum column for this row. */

        KValue minValue = INT_MAX;
        int minHere = minCol;
        for (int i = minCol; i<=maxCol; i++)
        {
            KValue v = M(currentRow, i);
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

    /* If item is the first cdf value with count at
     * or above i, returns the cumulative sum up to the ith entry of the
     * underlying sorted points list.
     */

    KValue cumulativeAt(std::vector<struct CDF>::iterator item, int i)
    {
        KValue sumBelow = 0.0;
        int numPointsBelow = 0;

        if (item != cdf.begin())
        {
            sumBelow = (item-1)->sumToHere;
            numPointsBelow = (item-1)->pointsSoFar;
        }

        return sumBelow + item->thisValue*(i - numPointsBelow);
    }

    /* Grønlund et al.'s CC function for the K-median problem.
     *
     * CC(i,j) is the cost of grouping points_i...points_j into one cluster
     * with the optimal cluster point (the median point). By programming
     * convention, the interval is half-open and indexed from 0, unlike the
     * paper's convention of closed intervals indexed from 1.
     *
     * Note: i and j are indices onto weighted_cdf. So e.g. if i = 0, j = 2 and
     * weighted cdf is [[2, 0, 0], [4, 2, 1], [5, 2, 2]], then that is the cost
     * of clustering all points between the one described by the zeroth weighted
     * cdf entry, and up to (but not including) the last. In other words, it's
     * CC([0, 0, 1, 1], 0, 5).
     */

    KValue CC(int i, int j)
    {
        if (i >= j)
            return 0;

        int lowerCount = 0;
        if (i > 0)
            lowerCount = cdf[i-1].pointsSoFar;
        int upperCount = cdf[j-1].pointsSoFar;

        /* Note, this is not the index, but number of points start inclusive,
         * which is why the -1 has to be turned into +1. */

        KValue medianPoint = (lowerCount + upperCount + 1) / 2.0;
        
        auto lowerMedian = std::lower_bound(cdf.begin(), cdf.end(), (int)medianPoint,
            [&](const struct CDF& lhs, int rhs) {
                return lhs.pointsSoFar < rhs;
            }
        );

        KValue mu;
        if (((int)ceil(medianPoint) == (int)floor(medianPoint))
                || ((int)floor(medianPoint) != lowerMedian->pointsSoFar))
            mu = lowerMedian->thisValue;
        else
            mu = (lowerMedian->thisValue + (lowerMedian+1)->thisValue) / 2.0;

        /* Lower part: entry i to median point between i and j, median excluded. */

        int sumBelow = cumulativeAt(lowerMedian, (int)medianPoint);
        if (i > 0)
            sumBelow -= cdf[i-1].sumToHere;

        /* Upper part: everything from the median up, including the median if it's a
         * real point. */

        int sumAbove = cdf[j-1].sumToHere - cumulativeAt(lowerMedian, (int)medianPoint);

        return floor(medianPoint - lowerCount)*mu - sumBelow + sumAbove - ceil(upperCount - medianPoint)*mu;
    }

    /* D_previous is the D vector for (i-1) clusters, or empty if i < 2.
     * It's possible to do this even faster (and more incomprehensibly).
     * See Grønlund et al. for that. */

    /* Calculate C_i[m][j] given D_previous = D[i-1]. p. 4 */

    KValue C_i(int i, const std::vector<KValue>& D_previous, int m, int j)
    {
        KValue f;
        if (i == 1)
            f = CC(0, m);
        else
            f = D_previous[std::min(j, m)] + CC(j, m);
        return f;
    }

    /* Calculates the optimal cluster centres for the points. */

    std::vector<KValue> optimalKMedian(int numClusters)
    {
        std::vector<std::vector<int>> T(numClusters+1, std::vector<int>(uniquePoints+1, 0));
        std::vector<std::vector<KValue>> D(numClusters+1, std::vector<KValue>(uniquePoints+1, 0.0));

        for (int i=1; i<numClusters+1; i++)
        {
            /* Stop if we achieve optimal clustering with fewer clusters than the
             * user asked for. */
            
            std::vector<KValue>& lastD = D[i-1];
            if ((i != 1) && (lastD.back() == 0.0))
                continue;

            std::function<KValue(int, int)> M = [&](int m, int j) {
                return C_i(i, lastD, m, j);
            };

            monotoneMatrixIndices(M, i, uniquePoints+1, i-1, uniquePoints+1,
                T[i], D[i]);
        }

        /* Backtrack. The last cluster has to encompass everything, so the
         * rightmost boundary of the last cluster is the last point in the
         * array, hence given by the last position of T. Then the previous
         * cluster transition boundary is given by where that cluster starts,
         * and so on.
         */

        int currentClusteringRange = uniquePoints;
        std::vector<KValue> centres;

        for (int i=numClusters; i>0; i--)
        {
            int newClusteringRange = T[i][currentClusteringRange];

            /* Reconstruct the cluster that's the median point between
             * new_cluster_range and cur_clustering_range. */

            centres.push_back(medianPoint(newClusteringRange, currentClusteringRange));
            currentClusteringRange = newClusteringRange;
        }
    
        std::sort(centres.begin(), centres.end());
        return centres;
    }

};
    
static std::vector<KValue> getPointsFromFluxmap(const Fluxmap& fluxmap)
{
    FluxmapReader fmr(fluxmap);
    std::vector<KValue> points;
    while (!fmr.eof())
    {
        KValue point = fmr.findEvent(F_BIT_PULSE);
        points.push_back(point);
    }
    return points;
}

/* Analyses the fluxmap and determines the optimal cluster centres for it. The
 * number of clusters is taken from the size of the output array. */

std::vector<KValue> optimalKMedian(const std::vector<KValue>& points, int clusters)
{
    KCluster kcluster(points);
    return kcluster.optimalKMedian(clusters);
}

std::vector<unsigned> optimalKMedian(const Fluxmap& fluxmap, int clusters)
{
	 std::vector<unsigned> ticks;
     for (KValue t : optimalKMedian(getPointsFromFluxmap(fluxmap), clusters))
	 	ticks.push_back(t);
	return ticks;
}

