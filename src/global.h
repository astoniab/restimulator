#pragma once

#include <string>
#include <cstdint>
#include <atomic>

struct ProgramOptions
{
	std::string m_restimhost;
	int32_t m_restimport;

	bool m_directml;
	int32_t m_dmldevid;
	int32_t m_camera;

	std::string m_posemodel;

	int32_t m_posesamp;

	// debug
	float m_posediv;
	float m_poseadd;
};

namespace global
{
	extern std::atomic<bool> stop;

	void MultiByteToWideCharString(const std::string& mbstring, std::wstring& wcstring);

}	// namespace global