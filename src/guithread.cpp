#include "guithread.h"

#include <atomic>
#include <thread>
#include <mutex>

#include "opencvfunctions.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

// debug
#include <iostream>

GUIThread::GUIThread() :IThread()
{

}

GUIThread::~GUIThread()
{

}

void GUIThread::Run(const IThread::ThreadParameters* threadparameters)
{
	try
	{
		const GUIThreadParameters params = *(dynamic_cast<const GUIThreadParameters*>(threadparameters));
		m_setcamera = params.m_setcamera;
		m_startcamera = params.m_startcamera;
		m_stopcamera = params.m_stopcamera;
		m_camerastatus = params.m_camerastatus;
		m_setposetrackinglocation = params.m_setposetrackinglocation;

		std::thread ft([this,&params]() {

			try
			{
				FormMain* fm = nullptr;
				{
					std::lock_guard<std::mutex> guard(m_formmutex);
					m_formrunning = true;
					fm = new FormMain(nullptr);

					fm->GetButtonStartStop()->events().click(std::bind(&GUIThread::HandleButtonStartStopClick, this));
					
					// MUST set initial selection before setting combo box change event - otherwise event will fire
					if (params.m_camera >= 0 && params.m_camera < fm->GetComboCamera()->the_number_of_options())
					{
						fm->GetComboCamera()->option(params.m_camera);
					}
					fm->GetComboCamera()->events().text_changed(std::bind(&GUIThread::HandleComboCameraChanged, this));
					fm->GetComboTrack()->events().text_changed(std::bind(&GUIThread::HandleComboTrackChanged, this));

					fm->show();
					m_form = fm;
					m_formcreated = true;
				}

				nana::exec();

				{
					std::lock_guard<std::mutex> guard(m_formmutex);
					m_form = nullptr;
					m_formrunning = false;
				}

				if (fm)
				{
					delete fm;
					fm = nullptr;
				}
			}
			catch (std::exception& e)
			{
				std::cout << "FormMain thread caught " << e.what() << std::endl;
			}

			});

		while (!m_stop)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			// TODO - update camera status, pose status labels
			if (m_camerastatus && m_form && m_formrunning)
			{
				
				CameraCaptureThread::CameraCaptureStatus status = m_camerastatus();
				switch (status)
				{
				case CameraCaptureThread::CAMERA_CAPTURE_RUNNING:
					m_form.load()->GetLabelStatus()->caption("Running");
					break;
				case CameraCaptureThread::CAMERA_CAPTURE_STOPPED:
					m_form.load()->GetLabelStatus()->caption("Stopped");
					break;
				case CameraCaptureThread::CAMERA_CAPTURE_ERROR:
					m_form.load()->GetLabelStatus()->caption("Error");
					break;
				default:
					m_form.load()->GetLabelStatus()->caption("Unknown");
					break;
				}
				
			}
		}

		{
			std::lock_guard<std::mutex> guard(m_formmutex);
			if (m_formrunning && m_form)
			{
				m_form.load()->close();
			}
		}

		ft.join();
	}
	catch (std::exception& e)
	{
		std::cout << "GUIThread::Run caught " << e.what() << std::endl;
	}
}

void GUIThread::ReceivePose(const cv::Mat& inim, const PoseDetection& posedetection)
{
	if(!inim.empty())
	{
		nana::size::value_type size = 0;

		{
			std::lock_guard<std::mutex> guard(m_formmutex);
			if (m_formcreated && m_formrunning && m_form)
			{
				size = m_form.load()->GetPictureCamera()->size().height;
			}
		}

		cv::Mat im = inim.clone();
		for (std::array<KeypointDetection, KeypointLocation::KEYPOINT_MAX>::const_iterator i = posedetection.m_keypoints.begin(); i != posedetection.m_keypoints.end(); i++)
		{
			if ((*i).m_presence == KeypointPresence::KEYPOINT_PRESENCE_PRESENT)
			{
				cv::circle(im, cv::Point((*i).m_pos.m_x, (*i).m_pos.m_y), 3, cv::Scalar(0.0, 255.0, 0.0), 1, 8, 0);
			}
		}

		cv::Mat displayim;
		if (size!=0 && ((im.rows != im.cols) || (im.rows != size)))
		{
			int32_t ox;
			int32_t oy;
			float scale;
			ResizeCropImage(im, displayim, size, ox, oy, scale);
		}
		else
		{
			displayim = im;
		}

		if (!displayim.empty())
		{
			std::vector<uint8_t> buff;
			cv::imencode(".bmp", displayim, buff, std::vector<int>());
			nana::paint::image i;
			i.open(buff.data(), buff.size());
			{
				std::lock_guard<std::mutex> guard(m_formmutex);
				if (m_formcreated && m_formrunning && m_form)
				{
					m_form.load()->GetPictureCamera()->load(i);
				}
			}
		}

	}
}

void GUIThread::HandleButtonStartStopClick()
{
	if (m_camerastarted == false && m_startcamera)
	{
		m_startcamera();
		m_camerastarted = true;
		std::cout << "Start camera" << std::endl;
	}
	else if (m_stopcamera)
	{
		m_stopcamera();
		m_camerastarted = false;
		std::cout << "Stop camera" << std::endl;
	}
}

void GUIThread::HandleComboCameraChanged()
{
	if (m_setcamera)
	{
		m_setcamera(m_form.load()->GetComboCamera()->option());
		std::cout << "Set camera " << m_form.load()->GetComboCamera()->option() << std::endl;
	}
}

void GUIThread::HandleComboTrackChanged()
{
	if (m_setposetrackinglocation)
	{
		m_setposetrackinglocation((PoseTrackingLocation)m_form.load()->GetComboTrack()->option());
		std::cout << "Set tracking location " << m_form.load()->GetComboTrack()->option() << std::endl;
	}
}

void GUIThread::ReceivePoseMovement(const PoseMovement& pm, const PoseTrackingLocation& track)
{
	
	if ((int)track < PoseTrackingLocationName.size())
	{
		std::string info("Tracking ");
		if ((int)track < PoseTrackingLocationName.size())
		{
			info += PoseTrackingLocationName[(int)track];
		}
		info += "\n";

		if (track != PoseTrackingLocation::POSE_TRACKING_NONE)
		{
			if (pm.m_posetracking[(int)track].m_presence == KeypointPresence::KEYPOINT_PRESENCE_PRESENT)
			{
				info += "Found\n";

				info += std::to_string(pm.m_posetracking[(int)track].m_current.m_x) + ", " + std::to_string(pm.m_posetracking[(int)track].m_current.m_y) + "\n";
			}
			else
			{
				info += "Not Found\n";
			}
		}

		std::lock_guard<std::mutex> guard(m_formmutex);
		if (m_formcreated && m_formrunning && m_form)
		{
			m_form.load()->GetLabelTrackStatus()->caption(info);

		}

	}

}
