#pragma once

#include <functional>
#include <memory>
#include <any>
#include <queue>
#include <optional>
#include <cstdlib>

namespace te
{
    class Pty
    {
    public:
        using InputFunc = std::function<void(std::shared_ptr<Pty> pty, std::any opaque, std::vector<std::byte> data)>;

        Pty();
        ~Pty();

        pid_t open(InputFunc fn, std::any opaque, uint16_t term_width, uint16_t term_height);
        void close();
        bool is_open();
        int get_fd();
        pid_t get_child();
        int dispatch();
        int write(std::vector<std::byte> data);
        int signal(int sig);
        int resize(uint16_t term_width, uint16_t term_height);

    private:
        static constexpr int pty_buffer_size = 16384;
        static constexpr std::byte pty_msg_failed{0};
        static constexpr std::byte pty_msg_setup{1};
        int fd_{-1};
        pid_t child_{0};
        std::vector<std::byte> in_buf_;
        std::queue<std::pair<std::vector<std::byte>, size_t>> out_buf_; // lock?

        InputFunc input_func_;
        std::any input_func_opaque_;

    private:
        int recv(int fd, std::byte &d);
        int recv(std::byte &d);
        int send(int fd, const std::byte &d);
        int send(const std::byte &d);
        int write();
        int read();
        int setup_child(int slave_fd, uint16_t term_width, uint16_t term_height);
        int init_child();
    };
}