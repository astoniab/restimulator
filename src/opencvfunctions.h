#pragma once

#include <cstdint>

#include <opencv2/core/mat.hpp>

void ResizeCropImage(const cv::Mat& inimg, cv::Mat& outimg, const int32_t size, int32_t& offsetx, int32_t& offsety, float& scale);
