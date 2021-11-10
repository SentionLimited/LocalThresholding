#include "local_thresholding.h"

using T = float;

int main(int argc, char *argv[])
{

	// Set parameters
	const int num_thresholds = 100;
	const T target_sum = 0.5;
	const T gaus_stdev = 15;

	// Load input data
	cv::Mat img = cv::imread("test_input.png", 0);
	cv::Mat img_downsampled = cv::imread("test_input_downsampled.png", 0);

	// Run local thresholding filter
	auto localThresholding = LocalThresholding<T>(num_thresholds, target_sum, gaus_stdev);
	cv::Mat thresholded_img = localThresholding.run(img, img_downsampled);

	// Save output image
	cv::imwrite("build/output/thresholded_img.png", thresholded_img * 255);

	return 0;
}
