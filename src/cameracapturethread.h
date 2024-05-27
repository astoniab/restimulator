#pragma once

#include "ithread.h"

#include <functional>
#include <mutex>
#include <cstdint>

#include <opencv2/core/mat.hpp>

class CameraCaptureThread :public IThread
{
public:
	CameraCaptureThread();
	virtual ~CameraCaptureThread();

	enum CameraCaptureStatus
	{
		CAMERA_CAPTURE_STOPPED=0,
		CAMERA_CAPTURE_RUNNING,
		CAMERA_CAPTURE_ERROR
	};

	struct CameraCaptureThreadParameters :public IThread::ThreadParameters
	{
		std::function<void(const cv::Mat&)> m_sendframe;
		int32_t m_camera{ 0 };
	};

	void SetCamera(const int32_t camera);
	void StopCapture();
	void StartCapture();

	CameraCaptureStatus Status() const;

private:

	void Run(const IThread::ThreadParameters* threadparameters);

	std::function<void(const cv::Mat&)> m_sendframe;
	std::atomic<CameraCaptureStatus> m_status;
	std::atomic<int32_t> m_camera;
	std::atomic<int32_t> m_newcamera;

};
