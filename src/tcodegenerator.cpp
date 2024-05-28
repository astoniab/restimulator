#include "tcodegenerator.h"

#ifdef _WIN32
#include <Windows.h>
#include <timeapi.h>
#pragma comment(lib,"winmm")
#endif

#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

TCodeGenerator::TCodeGenerator() :IThread(), m_posetrackinglocation(POSE_TRACKING_NONE), m_poselowvolume(true)
{
	m_posekeypointmapping[PoseTrackingLocation::POSE_TRACKING_HEAD].push_back(KeypointLocation::KEYPOINT_NOSE);
	m_posekeypointmapping[PoseTrackingLocation::POSE_TRACKING_LEFT_HAND].push_back(KeypointLocation::KEYPOINT_LEFT_WRIST);
	m_posekeypointmapping[PoseTrackingLocation::POSE_TRACKING_RIGHT_HAND].push_back(KeypointLocation::KEYPOINT_RIGHT_WRIST);
	m_posekeypointmapping[PoseTrackingLocation::POSE_TRACKING_HIPS].push_back(KeypointLocation::KEYPOINT_LEFT_HIP);
	m_posekeypointmapping[PoseTrackingLocation::POSE_TRACKING_HIPS].push_back(KeypointLocation::KEYPOINT_RIGHT_HIP);
	m_posekeypointmapping[PoseTrackingLocation::POSE_TRACKING_LEFT_FOOT].push_back(KeypointLocation::KEYPOINT_LEFT_ANKLE);
	m_posekeypointmapping[PoseTrackingLocation::POSE_TRACKING_RIGHT_FOOT].push_back(KeypointLocation::KEYPOINT_RIGHT_ANKLE);
}

TCodeGenerator::~TCodeGenerator()
{
	// TODO - set volume to 0 send
}

void TCodeGenerator::Run(const ThreadParameters* threadparameters)
{

	const TCodeGeneratorParameters params = *(dynamic_cast<const TCodeGeneratorParameters*>(threadparameters));
	m_sendtcode = params.m_sendtcode;
	m_sendposemovement = params.m_sendposemovement;
	m_posesamp = params.m_posesamp;

#ifdef _WIN32
	timeBeginPeriod(1);
#endif

	std::chrono::high_resolution_clock::time_point lasttime = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point thistime = lasttime;

	// debug
	m_circlerad = 0;
	//m_generatormode = TCODE_MODE_CIRCLE;
	m_generatormode = TCODE_MODE_POSE;

	while (!m_stop)
	{
		thistime = std::chrono::high_resolution_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(thistime - lasttime).count() >= 10)
		{
			// generate and send TCode
			switch (m_generatormode)
			{
			case TCODE_MODE_CIRCLE:
				UpdateRestimCirclePosition(lasttime, thistime);
				break;
			case TCODE_MODE_POSE:
				UpdateRestimPosePosition(lasttime, thistime);
				break;
			case TCODE_MODE_NONE:
			default:
				break;
			}

			lasttime = thistime;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

#ifdef _WIN32
	timeEndPeriod(1);
#endif

}

void TCodeGenerator::ReceivePose(const PoseDetection& pose)
{
	//std::cout << "TCodeGenerator::ReceivePose " << std::endl;
	const std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

	std::lock_guard<std::mutex> guard(m_posemutex);
	m_keypoints.push_back(pose);

	// calculate the average pose locations
	PoseDetection pd;
	pd.m_timestamp = pose.m_timestamp;
	int32_t cnt = 0;
	std::array<std::list<KeypointDetection>, KeypointLocation::KEYPOINT_MAX> tempkeypoints;
	
	std::list<PoseDetection>::reverse_iterator revendpos = m_keypoints.rbegin();
	for (revendpos = m_keypoints.rbegin(); revendpos != m_keypoints.rend() && cnt < m_posesamp; revendpos++, cnt++)
	{
	}

	for (std::list<PoseDetection>::reverse_iterator i = m_keypoints.rbegin(); i != revendpos; i++)
	{
		for (size_t j = 0; j < KeypointLocation::KEYPOINT_MAX; j++)
		{
			tempkeypoints[j].push_back((*i).m_keypoints[j]);
		}
	}

	for (size_t i = 0; i < pd.m_keypoints.size(); i++)
	{
		pd.m_keypoints[i] = AverageKeypoint(tempkeypoints[i].begin(), tempkeypoints[i].end());
	}

	m_avgkeypoints.push_back(pd);


	// calculate pose movement (30 seconds of data for center/min/max locations)
	PoseMovement pm;
	ConsolidateKeypointsToPoses(m_avgkeypoints, pose.m_timestamp, pm);
	m_posemovement.push_back(pm);

	if (m_sendposemovement)
	{
		m_sendposemovement(pm, m_posetrackinglocation);
	}

	// clear out pose keypoints older than 1 minute
	while (!m_keypoints.empty() && m_keypoints.front().m_timestamp < (now - std::chrono::minutes(1)))
	{
		m_keypoints.pop_front();
	}
	while (!m_avgkeypoints.empty() && m_avgkeypoints.front().m_timestamp < (now - std::chrono::minutes(1)))
	{
		m_avgkeypoints.pop_front();
	}
	while (!m_posemovement.empty() && m_posemovement.front().m_timestamp < (now - std::chrono::minutes(1)))
	{
		m_posemovement.pop_front();
	}
}

void TCodeGenerator::SetPoseTrackingLocation(const PoseTrackingLocation location)
{
	m_posetrackinglocation = location;
}

void TCodeGenerator::ConsolidateKeypointsToPoses(const std::list<PoseDetection>& keypoints, const std::chrono::high_resolution_clock::time_point& timestamp, PoseMovement& pm)
{
	pm.m_timestamp = timestamp;
	std::array<std::pair<std::vector<KeypointDetection>,std::vector<std::chrono::high_resolution_clock::time_point>>, PoseTrackingLocation::POSE_TRACKING_MAX> m_posekeypoints;
	for (size_t i = 0; i < pm.m_posetracking.size(); i++)
	{
		for (std::list<PoseDetection>::const_reverse_iterator j = keypoints.rbegin(); j != keypoints.rend() && (*j).m_timestamp > (std::chrono::high_resolution_clock::now() - std::chrono::seconds(30)); j++)
		{
			int64_t count = 0;
			KeypointDetection avg;
			avg.m_presence = KeypointPresence::KEYPOINT_PRESENCE_PRESENT;
			for (size_t k = 0; k < m_posekeypointmapping[(PoseTrackingLocation)i].size(); k++)
			{
				// all keypoints for tracking location need to be present
				if ((*j).m_keypoints[m_posekeypointmapping[(PoseTrackingLocation)i][k]].m_presence == KeypointPresence::KEYPOINT_PRESENCE_PRESENT)
				{
					 avg.m_pos += (*j).m_keypoints[m_posekeypointmapping[(PoseTrackingLocation)i][k]].m_pos;
					 count++;
				}
				else
				{
					avg.m_presence = KeypointPresence::KEYPOINT_PRESENCE_NOT_PRESENT;
				}
			}

			if (count > 0)
			{
				avg.m_pos /= count;
			}
			
			m_posekeypoints[i].first.push_back(avg);
			m_posekeypoints[i].second.push_back((*j).m_timestamp);
		}

		KeypointDetection avgkd = AverageKeypoint(m_posekeypoints[i].first.begin(), m_posekeypoints[i].first.end());

		pm.m_posetracking[i].m_center = avgkd.m_pos;
		pm.m_posetracking[i].m_current = (m_posekeypoints[i].first.size() > 0 ? m_posekeypoints[i].first[0].m_pos : pm.m_posetracking[i].m_current);
		pm.m_posetracking[i].m_presence = (m_posekeypoints[i].first.size() > 0 ? m_posekeypoints[i].first[0].m_presence : pm.m_posetracking[i].m_presence);

		KeypointPosition minloc = avgkd.m_pos;
		KeypointPosition maxloc = avgkd.m_pos;
		float mindist2 = 0;
		float maxdist2 = 0;
		for (size_t j = 0; j < m_posekeypoints[i].first.size(); j++)
		{
			if (m_posekeypoints[i].first[j].m_presence == KeypointPresence::KEYPOINT_PRESENCE_PRESENT)
			{
				float d2 = m_posekeypoints[i].first[j].m_pos.Distance2(pm.m_posetracking[i].m_center);

				if (d2 > mindist2 && (m_posekeypoints[i].first[j].m_pos <= pm.m_posetracking[i].m_center))
				{
					mindist2 = d2;
					minloc = m_posekeypoints[i].first[j].m_pos;
				}
				else if (d2 > maxdist2 && (m_posekeypoints[i].first[j].m_pos > pm.m_posetracking[i].m_center))
				{
					maxdist2 = d2;
					maxloc = m_posekeypoints[i].first[j].m_pos;
				}
			}
		}

		pm.m_posetracking[i].m_min = minloc;
		pm.m_posetracking[i].m_max = maxloc;

		// velocity - only use keypoints that are present
		
		size_t cur = 0;
		size_t prev = 1;
		if (m_posekeypoints[i].first.size() > 1)
		{
			if (m_posekeypoints[i].first[cur].m_presence == KeypointPresence::KEYPOINT_PRESENCE_PRESENT)
			{			
				while (m_posekeypoints[i].first[prev].m_presence != KeypointPresence::KEYPOINT_PRESENCE_PRESENT && ((prev + 1) < m_posekeypoints[i].first.size()))
				{
					prev++;
				}
				
				if (m_posekeypoints[i].first[prev].m_presence == KeypointPresence::KEYPOINT_PRESENCE_PRESENT)
				{
					const float pd = m_posekeypoints[i].first[prev].m_pos.Distance(m_posekeypoints[i].first[cur].m_pos);
					const float ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_posekeypoints[i].second[cur] - m_posekeypoints[i].second[prev]).count();
					const float diameter = pm.m_posetracking[i].m_min.Distance(pm.m_posetracking[i].m_max);
					
					if (diameter != 0.0 && ms != 0.0)
					{
						// allow 100 ms to traverse diameter of circle
						float vel = (pd / diameter) * 100.0 / ms;
						
						if (vel > 1)
						{
							vel = 1;
						}
						if (m_posekeypoints[i].first[cur].m_pos < m_posekeypoints[i].first[prev].m_pos)
						{
							vel = -vel;
						}

						pm.m_posetracking[i].m_velocity = vel;
						
					}
					
				}

			}
		}

	}
}

void TCodeGenerator::UpdateRestimCirclePosition(const std::chrono::high_resolution_clock::time_point& lasttimestamp, const std::chrono::high_resolution_clock::time_point& timestamp)
{
	std::ostringstream ostr;
	// TODO - generate position
	// debug
	m_circlerad += (0.01) * static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - lasttimestamp).count());
	while (m_circlerad > 2.0 * M_PI)
	{
		m_circlerad -= (2.0 * M_PI);
	}

	float minstroke = 0.5;
	float maxstroke = 0.9;
	float radius = (maxstroke - minstroke) / 2.0;

	float centerx = minstroke + radius;
	float centery = 0.5;

	// debug
	//int32_t alpha = (cosf(rad) + 1.0) * 9999.0 / 2.0;			//0-2 to range 0-9999
	//int32_t beta = (sinf(rad) + 1.0) * 9999.0 / 2.0;			//0-2 to range 0-9999

	int32_t alpha = ((cosf(m_circlerad) * (radius * 2.0)) + (centerx * 2.0)) * 9999.0 / 2.0;
	int32_t beta = ((sinf(m_circlerad) * (radius * 2.0)) + (centery * 2.0)) * 9999.0 / 2.0;

	ostr << "L0" << std::setw(4) << std::setfill('0') << alpha << "I5";
	ostr << " ";
	ostr << "L1" << std::setw(4) << std::setfill('0') << beta << "I5";

	//debug
	//std::cout << "Sending " << ostr.str() << " rad=" << rad << " cosf=" << cosf(rad) << " deltat=" << std::chrono::duration_cast<std::chrono::milliseconds>(thistime - lasttime).count() << std::endl;

	if (m_sendtcode)
	{
		m_sendtcode(ostr.str());
	}
}

void TCodeGenerator::UpdateRestimPosePosition(const std::chrono::high_resolution_clock::time_point& lasttimestamp, const std::chrono::high_resolution_clock::time_point& timestamp)
{
	//std::cout << "TCodeGenerator::UpdateRestimPosePosition" << std::endl;
	std::ostringstream ostr;
	std::unique_lock<std::mutex> guard(m_posemutex);

	if (!m_posemovement.empty())
	{
		const PoseMovement pm = m_posemovement.back();
		const PoseTrackingData pd = pm.m_posetracking[m_posetrackinglocation];
		
		guard.unlock();

		if (pm.m_timestamp > m_poselastupdate)	// don't send the same update multiple times
		{
			if (pd.m_presence == KeypointPresence::KEYPOINT_PRESENCE_PRESENT)
			{
				if (pd.m_current != pd.m_min && pd.m_current != pd.m_max)
				{
					const float totaldist = pd.m_min.Distance(pd.m_max);		// distance between min and max positions

					// since current pos may not be on same line as min <-> max, find angle between and get component that is on line
					float ang = atan2(pd.m_current.m_y - pd.m_min.m_y, pd.m_current.m_x - pd.m_min.m_x) - atan2(pd.m_max.m_y - pd.m_min.m_y, pd.m_max.m_x - pd.m_min.m_x);
					if (ang < 0)
					{
						ang += (M_PI * 2.0);
					}
					float dist = cos(ang) * pd.m_min.Distance(pd.m_current);

					//std::cout << pd.m_current.m_x << "," << pd.m_current.m_y << " " << pd.m_min.m_x << "," << pd.m_min.m_y << " " << pd.m_max.m_x << "," << pd.m_max.m_y <<"   mmdist=" << pd.m_min.Distance(pd.m_max) << "  d=" << dist << "  a=" << ang << std::endl;

					int32_t alphapos = (1.0 - (dist / totaldist)) * 9999.0;	// top of camera frame is y=0 and "bottom" in restim in y=0 - so we reverse position
					int32_t betapos = ((pd.m_velocity / 2.0) + 0.5) * 9999.0;

					/*
					if (m_poselowvolume == true)
					{
						ostr << "A09I5000 ";		// change volume to 100% over 5 seconds
						m_poselowvolume = false;
					}
					*/

					ostr << "L0" << std::setw(4) << std::setfill('0') << alphapos << "I5";
					ostr << " ";
					ostr << "L1" << std::setw(4) << std::setfill('0') << betapos << "I5";

					//debug
					//std::cout << "Sending " << ostr.str() << " rad=" << rad << " cosf=" << cosf(rad) << " deltat=" << std::chrono::duration_cast<std::chrono::milliseconds>(thistime - lasttime).count() << std::endl;

					if (m_sendtcode)
					{
						m_sendtcode(ostr.str());
					}

				}
			}
			else
			{
				// pose not present

				/*
				ostr << "A05I5000";		// change audio volume to 0.5 over 5 seconds

				if (m_poselowvolume == false && m_sendtcode)
				{
					m_sendtcode(ostr.str());
					m_poselowvolume = true;
				}
				*/
			}

			m_poselastupdate = pm.m_timestamp;
		}
	}

}
