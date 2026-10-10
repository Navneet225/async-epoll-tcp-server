# Async TCP Server using Linux epoll (C++17)

Single-threaded, non-blocking echo server using edge-triggered `epoll`.
It accepts all pending connections, drains sockets until `EAGAIN`,
handles partial writes, and cleans up closed connections.

```bash
make && ./epoll_server 9090
# in another terminal:
python3 load_test.py 500 200     # 500 clients x 200 messages
```

Sample run: 100K round-trips, 0 mismatches, ~45K round-trips/s on one core.

Requirements: Linux, g++ with C++17, Python 3 (load tester).
