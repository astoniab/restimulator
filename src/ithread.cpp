#include "ithread.h"

#include <thread>

IThread::ThreadParameters::ThreadParameters()
{

}

IThread::ThreadParameters::~ThreadParameters()
{

}

IThread::IThread()
{

}

IThread::~IThread()
{
	if (m_running)
	{
		Stop();
	}
	while (m_running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void IThread::Start(const ThreadParameters* threadparameters)
{
	std::lock_guard<std::mutex> guard(m_startmutex);
	if (!m_running)
	{
		m_running = true;
		std::thread t(&IThread::RunInternal, this, threadparameters);
		t.detach();
	}
}

void IThread::Stop()
{
	m_stop = true;
}

bool IThread::IsRunning() const
{
	return m_running;
}

void IThread::RunInternal(const ThreadParameters* threadparameters)
{
	Run(threadparameters);
	m_running = false;
}
