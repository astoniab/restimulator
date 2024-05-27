#include "opencvfunctions.h"

#include <opencv2/imgproc.hpp>

void ResizeCropImage(const cv::Mat& inimg, cv::Mat& outimg, const int32_t size, int32_t& offsetx, int32_t& offsety, float& scale)
{
	if (inimg.empty() || size < 1)
	{
		return;
	}

	if (inimg.cols == size && inimg.rows == size)
	{
		outimg = inimg;
		return;
	}

	const float ar = static_cast<float>(inimg.cols) / static_cast<float>(inimg.rows);
	scale = (ar >= 1.0) ? static_cast<float>(inimg.rows) / static_cast<float>(size) : static_cast<float>(inimg.cols) / static_cast<float>(size);
	offsetx = (ar >= 1.0) ? (inimg.cols - inimg.rows) / 2 : 0;
	const int32_t ex = (ar >= 1.0) ? ((inimg.cols - inimg.rows) / 2) + inimg.rows : inimg.cols - 1;
	offsety = (ar >= 1.0) ? 0 : (inimg.rows - inimg.cols) / 2;
	const int32_t ey = (ar >= 1.0) ? inimg.rows - 1 : ((inimg.rows - inimg.cols) / 2) + inimg.cols;

	cv::resize(inimg(cv::Range(offsety, ey), cv::Range(offsetx, ex)), outimg, cv::Size(size, size));

}