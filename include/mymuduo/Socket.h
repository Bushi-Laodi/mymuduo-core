#pragma once
#include <netinet/in.h>

namespace mymuduo{

class InetAddress;

class Socket{
public:
    explicit Socket(int fd) : fd_(fd){};
    ~Socket();

    int fd(){return fd_;};

    void bindAddress(const InetAddress& local);
    void listen();
    int accept(InetAddress* peer);
    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

    static int createNonBlockingSocket();
private:
    int fd_;
};


}
