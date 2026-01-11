#ifndef SERVER_LOG_QUEUE_H
#define SERVER_LOG_QUEUE_H

#include <queue>
#include <string>
#include <mutex>

class log_queue
{
public:
    void push(const std::string& value)
    {
        std::lock_guard lock(m_mutex);
        m_logQueue.push(value);
        m_cond.notify_one();
    }

    bool pop_with_timeout(std::string& out, uint32_t timeout_ms)
    {
        std::unique_lock lock(m_mutex);
        if (!m_cond.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]
        {
            return !m_logQueue.empty();
        }))
        {
            return false;
        }

        out = m_logQueue.front();
        m_logQueue.pop();
        return true;
    }
private:
    std::condition_variable m_cond;
    std::queue<std::string> m_logQueue;
    std::mutex m_mutex;
};

#endif //SERVER_LOG_QUEUE_H