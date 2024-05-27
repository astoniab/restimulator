#pragma once

#include "ithread.h"

#include <mutex>
#include <atomic>

#include "cameracapturethread.h"
#include "posekeypointdata.h"
#include "tcodegenerator.h"
#include "formmain.h"

#include <opencv2/core/mat.hpp>

#include <nana/gui/widgets/form.hpp>
#include <nana/gui/widgets/picture.hpp>

// has multithread nana example
// https://github.com/cnjinhao/nana-handbook/blob/master/Nana%20Handbook.md

class GUIThread :public IThread
{
public:
	GUIThread();
	virtual ~GUIThread();

	struct GUIThreadParameters :public IThread::ThreadParameters
	{
		int32_t m_camera = -1;
		std::function<void(const int32_t)> m_setcamera = nullptr;
		std::function<void()> m_stopcamera = nullptr;
		std::function<void()> m_startcamera = nullptr;
		std::function<CameraCaptureThread::CameraCaptureStatus()> m_camerastatus = nullptr;
		std::function<void(const PoseTrackingLocation)> m_setposetrackinglocation = nullptr;
	};

	void ReceivePose(const cv::Mat& im, const PoseDetection& posedetection);
	void ReceivePoseMovement(const PoseMovement& pm, const PoseTrackingLocation& track);

private:

	void Run(const IThread::ThreadParameters* threadparameters);

	void HandleButtonStartStopClick();
	void HandleComboCameraChanged();
	void HandleComboTrackChanged();

	std::mutex m_formmutex;
	std::atomic<bool> m_formcreated = false;
	std::atomic<bool> m_formrunning = false;
	std::atomic<FormMain*> m_form = nullptr;
	bool m_camerastarted = false;
	std::function<void(const int32_t)> m_setcamera = nullptr;
	std::function<void()> m_stopcamera = nullptr;
	std::function<void()> m_startcamera = nullptr;
	std::function<CameraCaptureThread::CameraCaptureStatus()> m_camerastatus = nullptr;
	std::function<void(const PoseTrackingLocation)> m_setposetrackinglocation = nullptr;

};
