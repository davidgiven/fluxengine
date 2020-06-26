#include "globals.h"
#include <string.h>
#include "bytes.h"
#include "fluxmap.h"
#include "kmedian.h"
#include "fmt/format.h"
#include "betterassert.h"

static void testOptimalKMedian()
{
	std::vector<float> points = { 22, 36, 35, 22, 22, 23, 37, 22, 35, 47 };

	std::vector<float> clusters = optimalKMedian(points, 3);

	assertEquals(clusters,
		(std::vector<float>{
			22, 35.5, 47
		})
	);
}

int main(int argc, const char* argv[])
{
	testOptimalKMedian();
	return 0;
}

