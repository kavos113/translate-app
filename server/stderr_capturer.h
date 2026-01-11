#ifndef SERVER_STDERR_CAPTURER_H
#define SERVER_STDERR_CAPTURER_H

#include <io.h>
#include <fcntl.h>
#include <functional>
#include <thread>
#include <iostream>
#include <string>

class stderr_capturer
{
public:
    void set_callback(const std::function<void(const std::string&)>& callback)
    {
        m_callback = callback;
    }

    void start()
    {
        if (_pipe(m_pipeFds, 4096, O_TEXT) != 0)
        {
            std::cout << "error create pipe" << std::endl;
            return;
        }

        original_stderr = _dup(_fileno(stderr));

        // write descriptor: stderr -> m_pipeFds
        if (_dup2(m_pipeFds[1], _fileno(stderr)) != 0)
        {
            std::cout << "error in dup2" << std::endl;
            return;
        }

        setvbuf(stderr, nullptr, _IONBF, 0);

        m_capturing = true;

        worker = std::thread(&stderr_capturer::readData, this);
    }

    void stop()
    {
        if (!m_capturing)
        {
            return;
        }

        m_capturing = false;

        if (original_stderr != -1)
        {
            _dup2(original_stderr, _fileno(stderr));
            _close(original_stderr);
            original_stderr = -1;
        }

        if (m_pipeFds[1] != -1)
        {
            _close(m_pipeFds[1]);
            m_pipeFds[1] = -1;
        }

        if (worker.joinable())
        {
            worker.join();
        }

        if (m_pipeFds[0] != -1)
        {
            _close(m_pipeFds[0]);
            m_pipeFds[0] = -1;
        }
    }
private:
    void readData() const
    {
        char buf[1024];

        while (m_capturing)
        {
            int bytes_read = _read(m_pipeFds[0], buf, sizeof(buf) - 1);

            if (bytes_read > 0)
            {
                buf[bytes_read] = 0;
                std::string str(buf);

                if (m_callback)
                {
                    m_callback(str);
                }
            }
            else
            {
                break;
            }
        }
    }

    bool m_capturing = false;

    int m_pipeFds[2] = {-1, -1};
    int original_stderr = -1;

    std::thread worker;
    std::function<void(const std::string&)> m_callback;
};


#endif //SERVER_STDERR_CAPTURER_H