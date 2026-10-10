CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra

epoll_server: epoll_server.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f epoll_server
