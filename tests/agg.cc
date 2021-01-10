#include "globals.h"
#include "bytes.h"
#include "dep/agg/include/agg2d.h"
#include <assert.h>

static void test_agg(void)
{
	const int WIDTH = 640;
	const int HEIGHT = 480;
	const int DEPTH = 3;
	const int STRIDE = WIDTH * DEPTH;

	std::vector<uint8_t> data(STRIDE*HEIGHT, 255);

	Agg2D painter;
	painter.attach(&data[0], WIDTH, HEIGHT, STRIDE);
	painter.line(0, 0, WIDTH, HEIGHT);
}

int main(int argc, const char* argv[])
{
	test_agg();
    return 0;
}

