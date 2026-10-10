# Async TCP Server using Linux epoll (C++17)

A single-threaded, non-blocking TCP echo server built directly on Linux `epoll` and POSIX sockets. It handles hundreds of concurrent connections on one thread without blocking on any single client.

## Features
- Non-blocking sockets with an `epoll` event loop
- Edge-triggered (`EPOLLET`) client sockets, each drained until `EAGAIN`
- Accepts all pending connections in a loop on every listener wake-up
- Handles partial writes so large reads are echoed completely
- `TCP_NODELAY` enabled to avoid Nagle delays on small messages
- Cleans up closed or errored connections (`EPOLL_CTL_DEL` + `close`)
- Graceful shutdown on `SIGINT`/`SIGTERM` with a summary of connections and bytes echoed

## How it works
1. Create a listening socket on `127.0.0.1:<port>`, set it non-blocking, and register it with `epoll`.
2. Loop on `epoll_wait`. If the listener is ready, `accept` until no more connections are pending, set each new socket non-blocking, and register it for `EPOLLIN | EPOLLET`.
3. If a client socket is ready, `read` in a loop until `EAGAIN` (required with edge-triggered mode, otherwise data can be missed) and echo the bytes back.
4. On EOF or a real error, remove the socket from `epoll` and close it.

## Project structure
```
epoll_server.cpp   # the server
load_test.py       # asyncio load tester that verifies every echo
Makefile
```

## Build and run
Requirements: Linux, g++ with C++17, Python 3 (for the load tester).

```bash
make
./epoll_server 9090
```

In a second terminal:

```bash
python3 load_test.py 500 200     # 500 concurrent clients x 200 messages each
```

On Windows, run this inside WSL.

## Test results
The load tester opens many concurrent connections, sends a unique message from each client, and checks that the echo matches exactly.

| Clients | Messages/client | Total round-trips | Mismatches | Throughput |
|---------|-----------------|-------------------|------------|------------|
| 500     | 200             | 100,000           | 0          | ~45K round-trips/s |

Measured on a single-core Linux sandbox with the client and server on the same machine, so numbers are indicative. Results will differ on your hardware.

## Limitations
- Echo only; no protocol parsing or framing
- Single-threaded (no multi-reactor / `SO_REUSEPORT` scaling)
- No `EPOLLOUT` write queue: if a socket send buffer is full, the write loop retries instead of buffering. Fine for small messages, not for slow clients
- Listens on loopback only

## Possible improvements
- Per-connection output buffers with `EPOLLOUT`
- One epoll loop per thread with `SO_REUSEPORT`
- A simple length-prefixed message protocol

## Author
Navneet Singh Yadav - [LinkedIn](https://www.linkedin.com/in/navneet-singh-yadav-439886281/)
