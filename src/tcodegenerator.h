#pragma once

#include "ithread.h"

#include <array>
#include <string>
#include <cstdint>
#include <functional>
#include <list>
#include <map>

#include "posekeypointdata.h"

struct TCodeAxisMetadata
{
	std::string m_axistype{ "" };
	int8_t m_channel{ -1 };
	float m_maxchange{ 1.0 };			// maximum value change per sample
};

struct TCodeAxis
{
	TCodeAxisMetadata m_metadata;
	float m_currentvalue;
	float m_expectedvalue;
};

enum RestimAxis
{
	RESTIM_AXIS_ALPHA = 1,
	RESTIM_AXIS_BETA = 2,
	RESTIM_AXIS_VOLUME = 3,
	RESTIM_AXIS_MAX
};

class TCodeGenerator :public IThread
{
public:
	TCodeGenerator();
	~TCodeGenerator();

	enum TCodeGeneratorMode
	{
		TCODE_MODE_NONE=0,
		TCODE_MODE_CIRCLE=1,
		TCODE_MODE_PROGRAM=2,
		TCODE_MODE_POSE=3
	};

	struct TCodeGeneratorParameters :public IThread::ThreadParameters
	{
		std::function<bool(const std::string&)> m_sendtcode = nullptr;
		std::function<void(const PoseMovement& pm, const PoseTrackingLocation& track)> m_sendposemovement = nullptr;
		int32_t m_posesamp = 1;
	};

	void ReceivePose(const PoseDetection& pose);
	void SetPoseTrackingLocation(const PoseTrackingLocation location);

private:

	void Run(const IThread::ThreadParameters *threadparameters);

	std::array<TCodeAxisMetadata, RESTIM_AXIS_MAX> m_axisdata;
	PoseTrackingLocation m_posetrackinglocation;
	TCodeGeneratorMode m_generatormode;
	std::function<bool(const std::string&)> m_sendtcode;
	std::function<void(const PoseMovement& pm, const PoseTrackingLocation& track)> m_sendposemovement = nullptr;
	int32_t m_posesamp;
	std::mutex m_posemutex;
	std::list<PoseDetection> m_keypoints;
	std::list<PoseDetection> m_avgkeypoints;

	// debug
	float m_circlerad;

	std::list<PoseMovement> m_posemovement;
	std::map<PoseTrackingLocation, std::vector<KeypointLocation>> m_posekeypointmapping;

	void ConsolidateKeypointsToPoses(const std::list<PoseDetection>& keypoints, const std::chrono::high_resolution_clock::time_point &timestamp, PoseMovement& pm);

	void UpdateRestimCirclePosition(const std::chrono::high_resolution_clock::time_point &lasttimestamp, const std::chrono::high_resolution_clock::time_point &timestamp);
	void UpdateRestimPosePosition(const std::chrono::high_resolution_clock::time_point& lasttimestamp, const std::chrono::high_resolution_clock::time_point& timestamp);
};
