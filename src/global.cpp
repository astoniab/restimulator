#include "global.h"

#include <vector>

namespace global
{

	std::atomic<bool> stop = false;

	void MultiByteToWideCharString(const std::string& mbstring, std::wstring& wcstring)
	{
		if (mbstring.empty())
		{
			wcstring = L"";
			return;
		}
		std::vector<wchar_t> wcbuff(1 + (mbstring.size() * 4), 0);
		size_t len = mbstowcs(&wcbuff[0], mbstring.data(), wcbuff.size());
		wcstring = std::wstring(wcbuff.begin(), wcbuff.begin() + len);
	}

}	// namespace global