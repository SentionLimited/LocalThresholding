#ifndef LOCAL_THRESHOLDING_H
#define LOCAL_THRESHOLDING_H

#include <cmath>
#include <array>
#include <assert.h>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

#include <armadillo>

template <typename T>
class LocalThresholding
{
private:
  int mNumThresholds;
  T mTargetSum;
  T mGausStdev;

  std::array<T, 256 * 256> h_gaussianKernel;
  std::array<T, 256> h_gaussianSums;

  void precalculateGaussian();

public:
  LocalThresholding(const int _num_thresholds, const T _target_sum, const T _gaus_stdev);

  cv::Mat run(const cv::Mat &h_iMask, const cv::Mat &h_iMiniMask);
};

#endif // LOCAL_THRESHOLDING_H
