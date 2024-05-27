#include "posedetectorthread.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <onnxruntime_cxx_api.h>
#include <dml/dml_provider_factory.h>

#include "global.h"
#include "opencvfunctions.h"

#ifdef _WIN32
#include <Windows.h>
#include <timeapi.h>
#pragma comment(lib,"winmm")
#endif

//debug
#include <iostream>

/*

	Movenet pose detector info
	https://github.com/tensorflow/tfjs-models/blob/master/pose-detection/README.md

*/

const std::array<int32_t, 17> PoseDetectorThread::m_movenetkeypoints{ 
	KEYPOINT_NOSE,
	KEYPOINT_LEFT_EYE_CENTER,
	KEYPOINT_RIGHT_EYE_CENTER,
	KEYPOINT_LEFT_EAR,
	KEYPOINT_RIGHT_EAR,
	KEYPOINT_LEFT_SHOULDER,
	KEYPOINT_RIGHT_SHOULDER,
	KEYPOINT_LEFT_ELBOW,
	KEYPOINT_RIGHT_ELBOW,
	KEYPOINT_LEFT_WRIST,
	KEYPOINT_RIGHT_WRIST,
	KEYPOINT_LEFT_HIP,
	KEYPOINT_RIGHT_HIP,
	KEYPOINT_LEFT_KNEE,
	KEYPOINT_RIGHT_KNEE,
	KEYPOINT_LEFT_ANKLE,
	KEYPOINT_RIGHT_ANKLE };

PoseDetectorThread::PoseDetectorThread() :IThread()
{

}

PoseDetectorThread::~PoseDetectorThread()
{

}

void PoseDetectorThread::ReceiveFrame(const cv::Mat& img)
{
	if (!m_stop && !img.empty())
	{
		std::lock_guard<std::mutex> guard(m_framemutex);
		m_frame.push(img);
	}
}

void PoseDetectorThread::Run(const IThread::ThreadParameters* threadparameters)
{
	try
	{
		const PoseDetectorThreadParameters params = *(dynamic_cast<const PoseDetectorThreadParameters*>(threadparameters));
		m_sendpose = params.m_sendpose;
		m_senddetection = params.m_senddetection;
		int64_t inputsize = 256;

		//debug
		m_posediv = params.m_posediv;
		m_poseadd = params.m_poseadd;

		Ort::Env* env = new Ort::Env(ORT_LOGGING_LEVEL_ERROR, "posedetector");
		Ort::SessionOptions session_options;
		session_options.SetIntraOpNumThreads(1);
		session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
		Ort::AllocatorWithDefaultOptions* allocator = new Ort::AllocatorWithDefaultOptions();

		// DirectML
		if (params.m_usedirectml)
		{
			session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
			session_options.DisableMemPattern();

			OrtApi const& ortApi = Ort::GetApi();
			OrtDmlApi const* ortDmlApi = nullptr;
			ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<void const**>(&ortDmlApi));

			ortApi.AddFreeDimensionOverrideByName(session_options, "batch_size", 1); // If your model has this dimension

			ortDmlApi->SessionOptionsAppendExecutionProvider_DML(session_options, params.m_directmldevid);
		}

		std::wstring onnxmodelwc{ L"" };
		global::MultiByteToWideCharString(params.m_onnxmodel, onnxmodelwc);
		Ort::Session* session = new Ort::Session(*env, onnxmodelwc.c_str(), session_options);

		const size_t num_input_nodes = session->GetInputCount();
		std::vector<const char*> input_node_names(num_input_nodes);
		std::vector<std::string> input_node_strs(num_input_nodes);

		for (int i = 0; i < num_input_nodes; i++)
		{
			input_node_strs[i] = session->GetInputNameAllocated(i, *allocator).get();
			input_node_names[i] = input_node_strs[i].c_str();
		}

		ONNXTensorElementDataType inputtype = ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED;
		if (num_input_nodes > 0)
		{
			inputtype = session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetElementType();
			//std::cout << "Model input data type=" << (int)inputtype << std::endl;
			std::vector<int64_t> input_shape = session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
			if (input_shape.size() == 4 && input_shape[1] == input_shape[2] && input_shape[3] == 3)
			{
				inputsize = input_shape[2];
			}
			else
			{
				// TODO - onnx input not in expected format
				m_stop = true;
			}
		}
		else
		{
			// TODO - onnx model not in expected format
			m_stop = true;
		}

		const size_t num_output_nodes = session->GetOutputCount();
		std::vector<const char*> output_node_names(num_output_nodes);
		std::vector<std::string> output_node_strs(num_output_nodes);

		for (int i = 0; i < num_output_nodes; i++)
		{
			output_node_strs[i] = session->GetOutputNameAllocated(i, *allocator).get();
			output_node_names[i] = output_node_strs[i].c_str();
		}

		Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
		std::vector<int64_t> input_node_dim(4, 0);
		input_node_dim[0] = 1;			// batch size
		input_node_dim[1] = inputsize;	// width of image
		input_node_dim[2] = inputsize;	// height of image
		input_node_dim[3] = 3;			// channels

		std::vector<float> floatinput(inputsize * inputsize * 3LL, 0);
		std::vector<uint8_t> uint8input(inputsize * inputsize * 3LL, 0);
		cv::Mat imin;
		cv::Mat imout;
		std::chrono::high_resolution_clock::time_point timestamp;

#ifdef _WIN32
		timeBeginPeriod(1);
#endif

		while (!m_stop)
		{
			bool process = false;
			{
				std::lock_guard<std::mutex> guard(m_framemutex);
				while (m_frame.size() > 1)	// ignore any excess frames since we aren't keeping up with them
				{
					m_frame.pop();
				}
				if (!m_frame.empty())
				{
					imin = m_frame.front();
					m_frame.pop();
					process = true;
					timestamp = std::chrono::high_resolution_clock::now();
				}
			}
			if (process)
			{
				// debug
				// std::cout << "Processing " << imin.cols << " x " << imin.rows << std::endl;
				int32_t imageoffsetx = 0;
				int32_t imageoffsety = 0;
				float imagescale = 1.0;
				ResizeCropImage(imin, imout, inputsize, imageoffsetx, imageoffsety, imagescale);

				Ort::Value input_tensor(nullptr);
				if (inputtype == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
				{
					//std::cout << "Setting float tensor input" << std::endl;
					SetONNXFloatInput(imout, floatinput.data());
					input_tensor = Ort::Value::CreateTensor<float>(memory_info, floatinput.data(), floatinput.size(), input_node_dim.data(), input_node_dim.size());
				}
				else if (ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8)
				{
					//std::cout << "Setting uint8 tensor input" << std::endl;
					SetONNXUint8Input(imout, uint8input.data());
					input_tensor = Ort::Value::CreateTensor<uint8_t>(memory_info, uint8input.data(), uint8input.size(), input_node_dim.data(), input_node_dim.size());
				}

				//std::cout << "Running model" << std::endl;
				auto output_tensors = session->Run(Ort::RunOptions{ nullptr }, input_node_names.data(), &input_tensor, 1, output_node_names.data(), output_node_names.size());
				//std::cout << "After model" << std::endl;
				
				/*
				Hi!

			As for the model input we divide RGB image with [0, 255] values by 255.0 to get [0, 1] values. So the second formula.

			As for the outputs, you are interested in first two:

			(1, 155) = (1, 31 * 5) - 33 landmarks, represented as (x, y, z, visibility, presence) each, in a flatten format. presence means landmark is within the frame bound, visibility means landmark is present and not occluded by other objects or body parts.
			(1, 1) - Pose presence flag as x from sigmoid(x). Meaning if you apply sigmoid to it, you'll get value in [0, 1] range.
				*/

				/*
				# Tensors from mode inference of
				# "mediapipe/modules/pose_landmark/pose_landmark_lite|full|heavy.tflite".
				# (std::vector<Tensor>)
				# tensors[0]: landmarks
				# tensors[1]: pose flag
				# tensors[2]: segmentation
				# tensors[3]: heatmap
				# tensors[4]: world landmarks
			*/

			// TODO - check which model we're running and process output based on that

				PoseDetection posedetection;
				posedetection.m_timestamp = timestamp;
				float* output = output_tensors[0].GetTensorMutableData<float>();

				// TODO - beter detection of model type
				if (num_output_nodes == 1)
				{
					for (int i = 0; i < 17; i++)
					{
						//std::cout << i << " " << output[(i * 3) + 0] << "," << output[(i * 3) + 1] << "\t" << output[(i * 3) + 2] << std::endl;
						//cv::circle(imout, cv::Point(output[(i * 3) + 1] * static_cast<float>(inputsize), output[(i * 3) + 0] * static_cast<float>(inputsize)), 2, cv::Scalar(0.0, 255, 0, 0.0), 1, 8, 0);

						posedetection.m_keypoints[m_movenetkeypoints[i]].m_pos.m_x = (output[(i * 3) + 1] * static_cast<float>(inputsize) * imagescale) + imageoffsetx;
						posedetection.m_keypoints[m_movenetkeypoints[i]].m_pos.m_y = (output[(i * 3) + 0] * static_cast<float>(inputsize) * imagescale) + imageoffsety;
						if (output[(i * 3) + 2] > 0.5)
						{
							posedetection.m_keypoints[m_movenetkeypoints[i]].m_presence = KEYPOINT_PRESENCE_PRESENT;
						}
					}

				}


			/*
			float* output = output_tensors[0].GetTensorMutableData<float>();
			for (int i = 0; i < 33; i++)
			{
				std::cout << i << "\t" << output[(i * 5) + 0] << ", " << output[(i * 5) + 1] << ", " << output[(i * 5) + 2] << "\t" << output[(i * 5) + 3] << "\t" << output[(i * 5) + 4] << std::endl;
				if (output[(i * 5) + 4] > 0.5)
				{
					cv::circle(imout, cv::Point(output[(i * 5) + 0], output[(i * 5) + 1]), 2, cv::Scalar(0.0, 255.0, 0.0), 1, 8, 0);
				}
			}

			/*
			float* outputsegment = output_tensors[2].GetTensorMutableData<float>();
			int64_t pix = 0;
			for (int64_t y = 0; y < imout.rows; y++)
			{
				for (int64_t x = 0; x < imout.cols; x++, pix++)
				{
					if (outputsegment[pix] < 0.3)
					{
						imout.data[(pix * 3) + 0] = 0;
						imout.data[(pix * 3) + 1] = 0;
						imout.data[(pix * 3) + 2] = 0;
					}
				}
			}
			*/

				// send original image and detected landmarks downstream

				for (std::vector<std::function<void(const cv::Mat&, const PoseDetection&)>>::iterator i = m_sendpose.begin(); i != m_sendpose.end(); i++)
				{
					if ((*i))
					{
						(*i)(imin, posedetection);
					}
				}
				for (std::vector<std::function<void(const PoseDetection&)>>::iterator i = m_senddetection.begin(); i != m_senddetection.end(); i++)
				{
					if ((*i))
					{
						(*i)(posedetection);
					}
				}
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}

#ifdef _WIN32
		timeEndPeriod(1);
#endif

		if (allocator)
		{
			delete allocator;
			allocator = nullptr;
		}

		if (session)
		{
			delete session;
			session = nullptr;
		}

		if (env)
		{
			delete env;
			env = nullptr;
		}
	}
	catch (std::exception& e)
	{
		std::cout << "PoseDetectorThread::Run caught " << e.what() << std::endl;
	}

	std::cout << "PoseDetectorThread::Run Thread Complete" << std::endl;

}

void PoseDetectorThread::SetONNXFloatInput(const cv::Mat& img, float* data)
{
	// OpenCV stores as BGR by default

	// TODO - copy to data (swap r and b) first, then do AVX instructions for multiply and add

	const uint8_t* imgdata = img.data;

	for (int64_t pix = 0; pix < img.rows * img.cols; pix++)
	{

		data[(pix * 3) + 0] = (static_cast<float>(imgdata[(pix * 3) + 2]) / m_posediv) + m_poseadd;
		data[(pix * 3) + 1] = (static_cast<float>(imgdata[(pix * 3) + 1]) / m_posediv) + m_poseadd;
		data[(pix * 3) + 2] = (static_cast<float>(imgdata[(pix * 3) + 0]) / m_posediv) + m_poseadd;
		
	}

}

void PoseDetectorThread::SetONNXUint8Input(const cv::Mat& img, uint8_t* data)
{

	// OpenCV stores as BGR by default

	const uint8_t* imgdata = img.data;

	for (int64_t pix = 0; pix < img.rows * img.cols; pix++)
	{

		data[(pix * 3) + 0] = (static_cast<float>(imgdata[(pix * 3) + 2]) / m_posediv) + m_poseadd;
		data[(pix * 3) + 1] = (static_cast<float>(imgdata[(pix * 3) + 1]) / m_posediv) + m_poseadd;
		data[(pix * 3) + 2] = (static_cast<float>(imgdata[(pix * 3) + 0]) / m_posediv) + m_poseadd;

	}

}
