#include "../include/syncevent.h"
#include <chrono>

bool SyncEvent::waitfor()
{
	return waitfor(-1);
}

bool SyncEvent::waitfor(const long timeout_ms)
{
	if (timeout_ms < 0)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this] { return m_signaled; });
		m_signaled = false;
	}
	else
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (!m_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return m_signaled; }))
			return false;
		m_signaled = false;
	}
	return true;
}

void SyncEvent::signal()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_signaled = true;
	}
	m_cv.notify_all();
}
