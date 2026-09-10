#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <spdlog/spdlog.h>
#include "pty_process.h"
#include "misc.h"

namespace te
{
    Pty::Pty()
    {
        in_buf_.resize(pty_buffer_size);
    }

    Pty::~Pty()
    {
    }

    pid_t Pty::open(InputFunc fn, std::any opaque, uint16_t term_width, uint16_t term_height)
    {
        int fd = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK);
        CHECK_EXPR_RET_ERROR(fd < 0, -errno, "posix_openpt failed: {}", errno);

        int comm[2];
        int ret = pipe2(comm, O_CLOEXEC);
        CHECK_EXPR_RET_ERROR(ret != 0, -errno, "pipe2 failed: {}", errno);

        pid_t pid = fork();
        if (pid < 0) // error
        {
            pid = -errno;
            ::close(comm[0]);
            ::close(comm[1]);
            return pid;
        }
        else if (pid == 0) // child
        {
            int slave = init_child();
            CHECK_EXPR_EXIT_ERROR(slave < 0, 1, "init_child failed");

            ::close(comm[0]);
            ::close(fd);
            fd = -1;

            ret = setup_child(slave, term_width, term_height);
            CHECK_EXPR_EXIT_ERROR(ret < 0, 1, "setup_child failed");

            if (slave > 2)
                ::close(slave);

            send(comm[1], pty_msg_setup); // check ret?
            ::close(comm[1]);

            return pid;
        }
        else
        {
            fd_ = fd;
            child_ = pid;
            ::close(comm[1]);
            fd = -1;

            std::byte d;
            recv(comm[0], d); // check ret?
            ::close(comm[0]);
            CHECK_EXPR_RET_ERROR(d != pty_msg_setup, -EINVAL, "recv failed");

            return pid;
        }
    }

    void Pty::close()
    {
        if (fd_ < 0)
            return;
        ::close(fd_);
        fd_ = -1;
    }

    bool Pty::is_open()
    {
        return fd_ >= 0;
    }

    int Pty::get_fd()
    {
        return fd_ >= 0 ? fd_ : -EPIPE;
    }

    pid_t Pty::get_child()
    {
        return child_ > 0 ? child_ : -ECHILD;
    }

    int Pty::dispatch()
    {
        if (!is_open())
            return -ENODEV;

        int len = read();
        write();
        return len;
    }

    int Pty::write(std::vector<std::byte> data)
    {
        if (!is_open())
            return -ENODEV;

        out_buf_.push(std::pair{data, 0});

        return data.size();
    }

    int Pty::signal(int sig)
    {
        if (!is_open())
            return -ENODEV;

        int ret = ioctl(fd_, TIOCSIG, sig);
        CHECK_EXPR_RET_ERROR(ret < 0, -errno, "ioctl TIOCSIG failed: {}", errno);

        return 0;
    }

    int Pty::resize(uint16_t term_width, uint16_t term_height)
    {
        if (!is_open())
            return -ENODEV;

        winsize ws{.ws_row = term_height, .ws_col = term_width};
        int ret = ioctl(fd_, TIOCSWINSZ, &ws);
        CHECK_EXPR_RET_ERROR(ret < 0, -errno, "ioctl TIOCSWINSZ failed: {}", errno);

        return 0;
    }

    int Pty::recv(int fd, std::byte &d)
    {
        int ret;
        do
        {
            ret = ::read(fd, &d, 1);
        }
        while (ret < 0 && (errno == EINTR || errno == EAGAIN));

        return ret <= 0 ? -errno : 0;
    }

    int Pty::recv(std::byte &d)
    {
        return recv(fd_, d);
    }

    int Pty::send(int fd, const std::byte &d)
    {
        int ret;
        do
        {
            ret = ::write(fd, &d, 1);
        }
        while (ret < 0 && (errno == EINTR || errno == EAGAIN));

        return (ret != 1) ? -errno : 0;
    }

    int Pty::send(const std::byte &d)
    {
        return send(fd_, d);
    }

    int Pty::write()
    {
        for (int i = 0; i < 2; ++i)
        {
            if (out_buf_.empty())
                return 0;

            auto &front = out_buf_.front();
            auto &data = front.first;
            auto &offset = front.second;
            ssize_t len = ::write(fd_, data.data() + offset, data.size() - offset);
            if (len < 0)
            {
                // 现在写不进去了
                if (errno == EAGAIN)
                    return 0;
                // 中断了，重试吧
                if (errno == EINTR)
                    return -EAGAIN;
                // 其它情况
                return -errno;
            }
            else if (len == 0)
            {
                // 关闭了
                return -EPIPE;
            }
            else if (len > 0)
            {
                offset += len;
                if (offset == data.size())
                    out_buf_.pop();
            }
        }

        return out_buf_.empty() ? 0 : -EAGAIN;
    }

    int Pty::read()
    {
        for (int i = 0; i < 2; ++i)
        {
            ssize_t len = ::read(fd_, in_buf_.data(), in_buf_.size() - 1);
            if (len < 0)
            {
                // 读到报EAGAIN，那就是读完了
                if (errno == EAGAIN)
                    return 0;
                // 读到被中断，那就需要重试了
                if (errno == EINTR)
                    return -EAGAIN;
                // 其它情况
                return -errno;
            }
            else if (len == 0)
            {
                // 关闭了
                return -EPIPE;
            }
            else if (len > 0 && input_func_)
            {
                in_buf_[len] = std::byte{0}; // debugging safety
                // [TODO] input_func_()
            }
        }

        // 还没读完，需要再读
        return -EAGAIN;
    }

    int Pty::setup_child(int slave_fd, uint16_t term_width, uint16_t term_height)
    {
        termios attr;
        int ret = tcgetattr(slave_fd, &attr);
        CHECK_EXPR_RET_ERROR(ret < 0, -errno, "tcgetattr failed: {}", errno);

        winsize ws = {.ws_row = term_height, .ws_col = term_width};
        ret = ioctl(slave_fd, TIOCSWINSZ, &ws);
        CHECK_EXPR_RET_ERROR(ret < 0, -errno, "ioctl TIOCSWINSZ failed: {}", errno);

        ret = dup2(slave_fd, STDIN_FILENO) != STDIN_FILENO ||
              dup2(slave_fd, STDOUT_FILENO) != STDOUT_FILENO ||
              dup2(slave_fd, STDERR_FILENO) != STDERR_FILENO;
        CHECK_EXPR_RET_ERROR(ret != 0, -errno, "dup2 failed: {}", errno);

        return 0;
    }

    int Pty::init_child()
    {
        sigset_t sigset;
        sigemptyset(&sigset);

        int ret = sigprocmask(SIG_SETMASK, &sigset, nullptr);
        CHECK_EXPR_RET_ERROR(ret < 0, -errno, "sigprocmask failed: {}", errno);

        for (int i = 0; i < SIGSYS; ++i)
            ::signal(i, SIG_DFL);

        ret = grantpt(fd_);
        CHECK_EXPR_RET_ERROR(ret < 0, -errno, "grantpt failed: {}", errno);

        ret = unlockpt(fd_);
        CHECK_EXPR_RET_ERROR(ret < 0, -errno, "unlockpt failed: {}", errno);

        auto slave_name = ptsname(fd_);
        CHECK_EXPR_RET_ERROR(!slave_name, -errno, "ptsname failed: {}", errno);

        int slave = ::open(slave_name, O_RDWR | O_CLOEXEC | O_NOCTTY);
        CHECK_EXPR_RET_ERROR(slave < 0, -errno, "open failed: {}", errno);

        int pid = setsid();
        CHECK_EXPR_GOTO_ERROR(pid < 0, label1, "setsid failed: {}", errno);

        ret = ioctl(slave, TIOCSCTTY, 0);
        CHECK_EXPR_GOTO_ERROR(ret < 0, label1, "ioctl TIOCSCTTY failed: {}", errno);

        return slave;

    label1:
        ::close(slave);
        return -errno;
    }

}