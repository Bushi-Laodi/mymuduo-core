#include "mymuduo/Socket.h"
#include "mymuduo/InetAddress.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/tcp.h>
#include <stdexcept>//runtime_error()
#include <sys/socket.h>
#include <unistd.h>

namespace mymuduo
{

int Socket::createNonBlockingSocket(){
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(fd < 0){
        throw std::runtime_error("createNonBlockingSocket() err");
    }
    return fd;
}

void Socket::bindAddress(const InetAddress& local){
    if(bind(fd_, reinterpret_cast<const sockaddr*>(local.getSockAddr()), sizeof(sockaddr_in)) < 0){
        throw std::runtime_error("socket bind error");
    }
}
Socket::~Socket(){
    if(fd_ > 0) close(fd_);//RAII
}
void Socket::listen(){
    if(::listen(fd_, SOMAXCONN) < 0){
        throw std::runtime_error("socket listen error");
    }
}
int Socket::accept(InetAddress* peer){
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    std::memset(&addr, 0, len);
    int connfd = accept4(fd_, reinterpret_cast<sockaddr*>(&addr), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd > 0){
        peer->setSockaddr(addr);
    }
    return connfd;
}
void Socket::shutdownWrite(){
    ::shutdown(fd_, SHUT_WR);
}

void Socket::setTcpNoDelay(bool on){
    int op = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &op, sizeof(op));
}
void Socket::setReuseAddr(bool on){
    int op = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
}
void Socket::setReusePort(bool on){
    int op = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &op, sizeof(op));
}
void Socket::setKeepAlive(bool on){
    int op = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &op, sizeof(op));
}


} // namespace mymuduo
