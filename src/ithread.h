#pragma once

#include <atomic>
#include <mutex>

class IThread
{
public:
	IThread();
	virtual ~IThread();

	struct ThreadParameters
	{
	public:
		ThreadParameters();
		virtual ~ThreadParameters();
	};

	void Start(const ThreadParameters* threadparameters);

	void Stop();
	bool IsRunning() const;

protected:

	std::atomic<bool> m_running;
	std::atomic<bool> m_stop;

	virtual void Run(const ThreadParameters* threadparameters) = 0;

private:

	std::mutex m_startmutex;

	void RunInternal(const ThreadParameters* threadparameters);

};
