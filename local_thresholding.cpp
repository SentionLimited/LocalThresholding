#include "local_thresholding.h"

using arma::Col;

//===================================================================

///
/// Constructor
///
template <typename T>
LocalThresholding<T>::LocalThresholding(
    const int _num_thresholds,
    const T _target_sum,
    const T _gaus_stdev)
{
    mNumThresholds = _num_thresholds;
    mTargetSum = _target_sum;
    mGausStdev = _gaus_stdev;

    precalculateGaussian();
}

//===================================================================

///
/// Precalculate Gaussian kernel
///
template <typename T>
void LocalThresholding<T>::precalculateGaussian()
{

    //
    // Populate h_gaussianKernel and h_gaussianSums (vector of sums of each gaussianKernel row)
    //
    for (int j = 0; j < 256; ++j)
    {

        T sum = 0.f;

        for (int i = 0; i < 256; ++i)
        {

            int coef = j * 256 + i;
            int dist_from_mean = abs(i - j);
            T res = std::exp(-0.5 * std::pow(dist_from_mean / mGausStdev, 2));
            h_gaussianKernel[coef] = res;
            sum += res;
        }

        h_gaussianSums[j] = sum;
    }
}

//===================================================================

///
/// Run local thresholding filter
///
template <typename T>
cv::Mat LocalThresholding<T>::run(
    const cv::Mat &h_iMask,
    const cv::Mat &h_iMiniMask)
{

    // Calculate scaling factor from inputs
    const int scaling_factor = h_iMask.cols / h_iMiniMask.cols;
    assert(scaling_factor * h_iMiniMask.cols == h_iMask.cols && "Downsampling scaling factor must be an integer.");
    assert(scaling_factor * h_iMiniMask.rows == h_iMask.rows && "Aspect ratio must be retained in downsampled image.");

    cv::Mat tmpMask = cv::Mat::zeros(h_iMask.size(), h_iMask.type());

    //
    // Loop over pixels in the downsampled mask
    //
    for (int c = 1; c < h_iMiniMask.cols - 1; ++c)
    {
        for (int r = 1; r < h_iMiniMask.rows - 1; ++r)
        {

            // Set vector of zeros for histogram
            Col<T> smoothed_hist(256, arma::fill::zeros);

            //
            // Calculate smoothed histogram by aggregating rows from d_gaussianKernel
            //
            for (int x = -1; x <= 1; ++x)
            {
                for (int y = -1; y <= 1; ++y)
                {

                    int pixVal = static_cast<int>(h_iMiniMask.at<uint8_t>(r + y, c + x));

                    for (int i = 0; i < 256; ++i)
                    {

                        int coef = pixVal * 256 + i;

                        smoothed_hist(i) += h_gaussianKernel[coef];
                    }
                }
            }

            // Normalize smoothed histogram such that sum equals one
            T smoothed_sum = arma::accu(smoothed_hist);
            smoothed_hist /= smoothed_sum;

            Col<T> cappedSum(mNumThresholds);

            T max_thresh = arma::max(smoothed_hist);
            T min_thresh = 0.f;
            T local_thresh = (max_thresh - min_thresh) / 2.f;

            //
            // Iterate over thresholds using bisection method
            //
            for (int iter = 0; iter < mNumThresholds; ++iter)
            {

                //
                // Cap smoothed histogram
                //
                Col<T> capped_hist = arma::clamp(smoothed_hist, 0, local_thresh);
                T aggregate = arma::sum(capped_hist);

                cappedSum(iter) = aggregate;

                //
                // Update the threshold based on the cappedSum and targetSum results
                //
                if (fabs(cappedSum(iter) - mTargetSum) < 1e-3)
                {

                    break;
                }
                else if (cappedSum(iter) > mTargetSum)
                {

                    max_thresh = local_thresh;
                    local_thresh = (min_thresh + local_thresh) / 2;
                }
                else
                {

                    min_thresh = local_thresh;
                    local_thresh = (max_thresh + local_thresh) / 2;
                }

                assert(iter < mNumThresholds - 1 && "Error: Target sum was not reached in the preset number of iterations.");
            }

            //
            // Apply local threshold to corresponding block in the original image
            //

            // Determine applicable bounds in original image
            int x_min = c * scaling_factor;
            int x_max = (c + 1) * scaling_factor;
            int y_min = r * scaling_factor;
            int y_max = (r + 1) * scaling_factor;

            for (int cc = x_min; cc < x_max; ++cc)
            {
                for (int rr = y_min; rr < y_max; ++rr)
                {
                    T g = smoothed_hist(static_cast<int>(h_iMask.at<uint8_t>(rr, cc)));
                    tmpMask.at<uint8_t>(rr, cc) = (g > local_thresh) ? 0 : 1;
                }
            }
        }
    }

    // Apply morphological close operation
    cv::Mat h_oMask;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(tmpMask, h_oMask, cv::MORPH_CLOSE, kernel);

    return h_oMask;
}

//===================================================================

////
//// Explicit instantiation for the template class
////
template class LocalThresholding<float>;