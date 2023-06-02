#include <condition_variable>
#include <mutex>

class SyncEvent {
public:
	// Waits for the event for a certain amount of time
	bool waitfor();
	bool waitfor(const long timeout_mss);
	void signal();

private:
	std::condition_variable m_cv;
	std::mutex m_mutex;
	bool m_signaled = false;
};