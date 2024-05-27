#pragma once

#include <array>
#include <chrono>
#include <string>

enum KeypointLocation
{
	KEYPOINT_NOSE = 1,
	KEYPOINT_RIGHT_EYE_CENTER,
	KEYPOINT_LEFT_EYE_CENTER,
	KEYPOINT_RIGHT_EAR,
	KEYPOINT_LEFT_EAR,
	KEYPOINT_MOUTH_CENTER,
	KEYPOINT_RIGHT_SHOULDER,
	KEYPOINT_LEFT_SHOULDER,
	KEYPOINT_RIGHT_ELBOW,
	KEYPOINT_LEFT_ELBOW,
	KEYPOINT_RIGHT_WRIST,
	KEYPOINT_LEFT_WRIST,
	KEYPOINT_RIGHT_HIP,
	KEYPOINT_LEFT_HIP,
	KEYPOINT_RIGHT_KNEE,
	KEYPOINT_LEFT_KNEE,
	KEYPOINT_RIGHT_ANKLE,
	KEYPOINT_LEFT_ANKLE,
	KEYPOINT_MAX
};

enum PoseTrackingLocation
{
	POSE_TRACKING_NONE = 0,
	POSE_TRACKING_HEAD,
	POSE_TRACKING_HIPS,
	POSE_TRACKING_LEFT_HAND,
	POSE_TRACKING_RIGHT_HAND,
	POSE_TRACKING_LEFT_FOOT,
	POSE_TRACKING_RIGHT_FOOT,
	POSE_TRACKING_MAX
};

extern std::array<std::string, PoseTrackingLocation::POSE_TRACKING_MAX> PoseTrackingLocationName;

enum KeypointPresence
{
	KEYPOINT_PRESENCE_UNKNOWN = 0,
	KEYPOINT_PRESENCE_PRESENT,
	KEYPOINT_PRESENCE_NOT_PRESENT
};

struct KeypointPosition
{
	float m_x = 0.0;
	float m_y = 0.0;
	float m_z = 0.0;

	bool operator==(const KeypointPosition& val) const;
	bool operator!=(const KeypointPosition& val) const;
	bool operator<(const KeypointPosition& val) const;
	bool operator>(const KeypointPosition& val) const;
	bool operator<=(const KeypointPosition& val) const;
	bool operator>=(const KeypointPosition& val) const;
	KeypointPosition& operator+=(const KeypointPosition& rhs);
	KeypointPosition operator+(const KeypointPosition& rhs) const;
	KeypointPosition& operator/=(const float val);
	KeypointPosition operator/(const float val) const;

	float Distance2(const KeypointPosition& rhs) const;
	float Distance(const KeypointPosition& rhs) const;
};

struct KeypointDetection
{
	KeypointPresence m_presence{ KEYPOINT_PRESENCE_UNKNOWN };
	KeypointPosition m_pos;
};

struct PoseDetection
{
	std::array<KeypointDetection, KeypointLocation::KEYPOINT_MAX> m_keypoints;
	std::chrono::high_resolution_clock::time_point m_timestamp;
};

struct PoseTrackingData
{
	KeypointPresence m_presence{ KEYPOINT_PRESENCE_UNKNOWN };
	KeypointPosition m_center;
	KeypointPosition m_min;
	KeypointPosition m_max;
	KeypointPosition m_current;
	float m_velocity;
};

struct PoseMovement
{
	std::array<PoseTrackingData, PoseTrackingLocation::POSE_TRACKING_MAX> m_posetracking;
	std::chrono::high_resolution_clock::time_point m_timestamp;
};

template<class InputIt>
KeypointDetection AverageKeypoint(InputIt begin, InputIt end)
{
	KeypointDetection avg;
	int64_t count = 0;
	for (; begin != end; ++begin)
	{
		if ((*begin).m_presence == KeypointPresence::KEYPOINT_PRESENCE_PRESENT)
		{
			avg.m_pos += (*begin).m_pos;
			count++;
		}
	}
	if (count > 0)
	{
		avg.m_presence = KeypointPresence::KEYPOINT_PRESENCE_PRESENT;
		avg.m_pos /= count;
	}
	else
	{
		avg.m_presence = KeypointPresence::KEYPOINT_PRESENCE_NOT_PRESENT;
	}
	return avg;
}
