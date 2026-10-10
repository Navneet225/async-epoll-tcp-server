// Single-threaded asynchronous TCP echo server using Linux epoll (edge-triggered, non-blocking).
// Build: g++ -O2 -std=c++17 epoll_server.cpp -o epoll_server
// Run:   ./epoll_server 9090
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

static volatile sig_atomic_t g_stop = 0;
static void on_sig(int) { g_stop = 1; }

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl failed");
}

static void epoll_add(int ep, int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev) < 0) throw std::runtime_error("epoll_ctl failed");
}

int main(int argc, char** argv) {
    const int port = argc > 1 ? std::atoi(argv[1]) : 9090;
    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);
    if (bind(lfd, (sockaddr*)&addr, sizeof(addr)) < 0 || listen(lfd, 1024) < 0) {
        std::perror("bind/listen");
        return 1;
    }
    set_nonblocking(lfd);

    int ep = epoll_create1(0);
    epoll_add(ep, lfd, EPOLLIN);

    constexpr int kMaxEvents = 256;
    epoll_event events[kMaxEvents];
    char buf[4096];
    unsigned long long total_bytes = 0, total_conns = 0;

    std::printf("epoll echo server listening on 127.0.0.1:%d\n", port);
    std::fflush(stdout);

    while (!g_stop) {
        int n = epoll_wait(ep, events, kMaxEvents, 200);
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == lfd) {
                // Accept all pending connections (edge-case safe loop).
                while (true) {
                    int cfd = accept(lfd, nullptr, nullptr);
                    if (cfd < 0) break;  // EAGAIN -> no more pending
                    set_nonblocking(cfd);
                    int nd = 1;
                    setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &nd, sizeof(nd));
                    epoll_add(ep, cfd, EPOLLIN | EPOLLET);
                    ++total_conns;
                }
            } else {
                bool close_it = false;
                // Edge-triggered: must drain the socket until EAGAIN.
                while (true) {
                    ssize_t r = read(fd, buf, sizeof(buf));
                    if (r > 0) {
                        total_bytes += r;
                        ssize_t off = 0;
                        while (off < r) {
                            ssize_t w = write(fd, buf + off, r - off);
                            if (w < 0) {
                                if (errno == EAGAIN) continue;  // small msgs: retry
                                close_it = true;
                                break;
                            }
                            off += w;
                        }
                        if (close_it) break;
                    } else if (r == 0) {
                        close_it = true;
                        break;
                    } else {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) close_it = true;
                        break;
                    }
                }
                if (close_it) {
                    epoll_ctl(ep, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                }
            }
        }
    }
    std::printf("shutdown: %llu connections served, %llu bytes echoed\n", total_conns, total_bytes);
    close(ep);
    close(lfd);
    return 0;
}
