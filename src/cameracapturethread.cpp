#include "cameracapturethread.h"

#include <opencv2/videoio.hpp>

//debug
#include <iostream>

CameraCaptureThread::CameraCaptureThread() :IThread(), m_status(CAMERA_CAPTURE_STOPPED), m_camera(0), m_newcamera(-1)
{

}

CameraCaptureThread::~CameraCaptureThread()
{

}

void CameraCaptureThread::Run(const IThread::ThreadParameters* threadparameters)
{
	try
	{
		const CameraCaptureThreadParameters params = *(dynamic_cast<const CameraCaptureThreadParameters*>(threadparameters));
		m_sendframe = params.m_sendframe;
		m_camera = params.m_camera;

		cv::VideoCapture vc;

		cv::Mat im;
		bool dosleep = false;

		while (!m_stop)
		{
			dosleep = false;

			if (m_status == CAMERA_CAPTURE_RUNNING)
			{
				const int32_t nc = m_newcamera.load();
				if (nc >= 0 && nc != m_camera)
				{
					if (vc.open(nc))
					{
						m_camera = nc;
					}
					else
					{
						m_status = CAMERA_CAPTURE_ERROR;
					}
					m_newcamera = -1;
				}

				if (!vc.isOpened() && m_camera >= 0)
				{
					if (!vc.open(m_camera))
					{
						m_status = CAMERA_CAPTURE_ERROR;
					}
				}

				if (vc.read(im) && !im.empty())
				{
					if (m_sendframe)
					{
						m_sendframe(im);
					}
				}
				else
				{
					dosleep = true;
				}
			}
			else if (vc.isOpened())
			{
				vc.release();
			}

			if ((!m_status == CAMERA_CAPTURE_RUNNING) || dosleep == true)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		std::cout << "CameraCaptureThread::Run Thread Complete" << std::endl;
	}
	catch (std::exception& e)
	{
		std::cout << "CameraCaptureThread::Run caught " << e.what() << std::endl;
	}

}

void CameraCaptureThread::SetCamera(const int32_t camera)
{
	m_newcamera = camera;
}

void CameraCaptureThread::StopCapture()
{
	m_status = CAMERA_CAPTURE_STOPPED;
}

void CameraCaptureThread::StartCapture()
{
	m_status = CAMERA_CAPTURE_RUNNING;
}

CameraCaptureThread::CameraCaptureStatus CameraCaptureThread::Status() const
{
	return m_status;
}
