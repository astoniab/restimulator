#include <iostream>
#include <string>
#include <csignal>

#include "cxxopts.hpp"
#include "global.h"

#include <opencv2/core/utils/logger.hpp>

#include <vector>
#include <cstdint>

#include "cameracapturethread.h"
#include "posedetectorthread.h"
#include "guithread.h"
#include "tcodegenerator.h"
#include "restimtcpconnection.h"

std::vector <float> sigmoid(const std::vector <float>& m1) {

	/*  Returns the value of the sigmoid function f(x) = 1/(1 + e^-x).
		Input: m1, a vector.
		Output: 1/(1 + e^-x) for every element of the input matrix m1.
	*/

	const unsigned long VECTOR_SIZE = m1.size();
	std::vector <float> output(VECTOR_SIZE);


	for (unsigned i = 0; i != VECTOR_SIZE; ++i) {
		output[i] = 1 / (1 + exp(-m1[i]));
	}

	return output;
}

void sighandler(int signo)
{
	global::stop = true;
}

int main(int argc, char* argv[])
{
	signal(SIGINT, sighandler);
	signal(SIGABRT, sighandler);
	signal(SIGBREAK, sighandler);

	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);

	cxxopts::Options options("restimulator", "Restim Controller");
	options.add_options()
		("h,help", "Help", cxxopts::value<bool>(), "Print help")
		("directml", "Use DirectML", cxxopts::value<bool>()->default_value("false"), "Use DirectML on GPU for running ONNX models")
		("dmldevid", "DirectML Device ID", cxxopts::value<int>()->default_value("0"), "ID (Index) of GPU device to use for DirectML")
		("camera", "Camera ID", cxxopts::value<int>()->default_value("0"), "ID (Index) of camera device to use for input")
		;

	options.add_options("restim")
		("restimhost", "Restim Host", cxxopts::value<std::string>()->default_value("127.0.0.1"), "Restim host name/address")
		("restimport", "Restim Port", cxxopts::value<int>()->default_value("12347"), "Restim listening port for TCP connections")
		;

	options.add_options("pose")
		("posemodel", "Pose Model", cxxopts::value<std::string>()->default_value("movenet_lightning.onnx"), "Full or relative path to the pose detection ONNX model to use")
		("posesamp", "Pose Samples", cxxopts::value<int>()->default_value("3"), "Combine this many pose samples together to get an average value to smooth keypoint jitter")
		// debug
		("posediv", "Channel Divide", cxxopts::value<float>()->default_value("1.0"), "Value to divide image channel value for normalization")
		("poseadd", "Channel Add", cxxopts::value<float>()->default_value("0"), "Value to add to image channel value after division for normalization")
		;

	cxxopts::ParseResult pr = options.parse(argc, argv);

	if (pr.count("h") > 0 || pr.count("?") > 0 || pr.unmatched().size() > 0)
	{
		std::cout << options.help();
		return 0;
	}

	ProgramOptions opts;

	opts.m_restimhost = pr["restimhost"].as<std::string>();
	opts.m_restimport = pr["restimport"].as<int>();
	opts.m_directml = pr["directml"].as<bool>();
	opts.m_dmldevid = pr["dmldevid"].as<int>();
	opts.m_camera = pr["camera"].as<int>();
	opts.m_posemodel = pr["posemodel"].as<std::string>();
	opts.m_posesamp = pr["posesamp"].as<int>();
	//debug
	opts.m_posediv = pr["posediv"].as<float>();
	opts.m_poseadd = pr["poseadd"].as<float>();
	//debug
	/*
	{
		std::cout << "Min / Max Normalized Image Channel Value " << ((0.) / opts.m_posediv) + opts.m_poseadd << " - " << ((255.) / opts.m_posediv) + opts.m_poseadd << std::endl;
	}
	*/

	CameraCaptureThread cct;
	PoseDetectorThread pdt;
	GUIThread guit;
	TCodeGenerator tcgt;
	RestimTCPConnection rtct;

	CameraCaptureThread::CameraCaptureThreadParameters cctp;
	cctp.m_camera = opts.m_camera;
	cctp.m_sendframe = std::bind(&PoseDetectorThread::ReceiveFrame, &pdt, std::placeholders::_1);

	PoseDetectorThread::PoseDetectorThreadParameters pdtp;
	pdtp.m_onnxmodel = opts.m_posemodel;
	pdtp.m_usedirectml = opts.m_directml;
	pdtp.m_directmldevid = opts.m_dmldevid;
	//debug
	pdtp.m_posediv = opts.m_posediv;
	pdtp.m_poseadd = opts.m_poseadd;
	pdtp.m_sendpose.push_back(std::bind(&GUIThread::ReceivePose, &guit, std::placeholders::_1, std::placeholders::_2));
	pdtp.m_senddetection.push_back(std::bind(&TCodeGenerator::ReceivePose, &tcgt, std::placeholders::_1));

	GUIThread::GUIThreadParameters guitp;
	guitp.m_camera = opts.m_camera;
	guitp.m_setcamera = std::bind(&CameraCaptureThread::SetCamera, &cct, std::placeholders::_1);
	guitp.m_startcamera = std::bind(&CameraCaptureThread::StartCapture, &cct);
	guitp.m_stopcamera = std::bind(&CameraCaptureThread::StopCapture, &cct);
	guitp.m_camerastatus = std::bind(&CameraCaptureThread::Status, &cct);
	guitp.m_setposetrackinglocation = std::bind(&TCodeGenerator::SetPoseTrackingLocation, &tcgt, std::placeholders::_1);

	TCodeGenerator::TCodeGeneratorParameters tcgtp;
	tcgtp.m_sendtcode = std::bind(&RestimTCPConnection::SendTCode, &rtct, std::placeholders::_1);
	tcgtp.m_sendposemovement = std::bind(&GUIThread::ReceivePoseMovement, &guit, std::placeholders::_1, std::placeholders::_2);
	tcgtp.m_posesamp = opts.m_posesamp;

	RestimTCPConnection::RestimTCPConnectionParameters rtctp;
	rtctp.m_host = opts.m_restimhost;
	rtctp.m_port = opts.m_restimport;

	cct.Start(&cctp);
	pdt.Start(&pdtp);
	guit.Start(&guitp);
	tcgt.Start(&tcgtp);
	rtct.Start(&rtctp);


	while (!global::stop)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	};

	std::cout << "Stopping" << std::endl;

	pdt.Stop();
	cct.Stop();
	guit.Stop();
	tcgt.Stop();

	while (pdt.IsRunning() || cct.IsRunning() || guit.IsRunning() /* || tcgt.IsRunning() || rtct.IsRunning()*/)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// TCP connection should be shut down AFTER TCodeGenerator in case it needs to send final T-Code commands
	rtct.Stop();
	while (rtct.IsRunning())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::cout << "Shutdown" << std::endl;

	return 0;
}
