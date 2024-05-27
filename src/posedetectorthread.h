#pragma once

#include "ithread.h"

#include <queue>
#include <mutex>
#include <functional>
#include <string>
#include <cstdint>
#include <vector>

#include <opencv2/core/mat.hpp>

#include "posekeypointdata.h"

class PoseDetectorThread :public IThread
{
public:
	PoseDetectorThread();
	virtual ~PoseDetectorThread();

	struct PoseDetectorThreadParameters :public IThread::ThreadParameters
	{
		std::vector<std::function<void(const cv::Mat&, const PoseDetection&)>> m_sendpose;
		std::vector<std::function<void(const PoseDetection&)>> m_senddetection;
		std::string m_onnxmodel{ "" };
		bool m_usedirectml{ false };
		int32_t m_directmldevid{ 0 };

		//debug
		float m_posediv;
		float m_poseadd;
	};

	void ReceiveFrame(const cv::Mat& img);

private:

	void Run(const IThread::ThreadParameters* threadparameters);

	void SetONNXFloatInput(const cv::Mat& img, float* data);
	void SetONNXUint8Input(const cv::Mat& img, uint8_t* data);

	std::vector<std::function<void(const cv::Mat&, const PoseDetection&)>> m_sendpose;
	std::vector<std::function<void(const PoseDetection&)>> m_senddetection;
	std::mutex m_framemutex;
	std::queue<cv::Mat> m_frame;

	static const std::array<int32_t, 17> m_movenetkeypoints;		// movenet keypoint index to our keypoint list

	//debug
	float m_posediv;
	float m_poseadd;

};
