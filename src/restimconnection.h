#pragma once

#include "ithread.h"

class RestimConnection :public IThread
{
public:
	RestimConnection();
	virtual ~RestimConnection();

	virtual bool SendTCode(const std::string& tcode) = 0;

protected:

	virtual void Run(const ThreadParameters* threadparameters) = 0;

};