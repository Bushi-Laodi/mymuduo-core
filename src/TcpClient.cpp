#include "mymuduo/TcpClient.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/Socket.h"
#include "mymuduo/TcpConnection.h"
#include <cstdio>
#include <cstring>
#include <sys/socket.h>

namespace mymuduo {

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr)),
      name_(name),
      nextConnId_(1) 
{
    connector_->setNewConnectionCallback([this](int sockfd) { newConnection(sockfd); });
}

TcpClient::~TcpClient() {
    TcpConnectionPtr conn = connection_;
    if (conn) {
        conn->getLoop()->runInLoop([conn] { conn->connectDestroyed(); });
    }
}

void TcpClient::connect() {
    connector_->start();
}

void TcpClient::disconnect() {
    if (connection_) connection_->shutdown();
}

void TcpClient::newConnection(int sockfd) {
    loop_->assertInLoopThread();

    sockaddr_in peer;
    std::memset(&peer, 0, sizeof(peer));
    socklen_t len = sizeof(peer);
    ::getpeername(sockfd, reinterpret_cast<sockaddr*>(&peer), &len);
    InetAddress peerAddr(peer);

    sockaddr_in local;
    std::memset(&local, 0, sizeof(local));
    len = sizeof(local);
    ::getsockname(sockfd, reinterpret_cast<sockaddr*>(&local), &len);
    InetAddress localAddr(local);

    char buf[64];
    snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_++);
    std::string connName = name_ + buf;

    TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr& c) { removeConnection(c); });
    connection_ = conn;
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    connection_.reset();
    loop_->queueInLoop([conn] { conn->connectDestroyed(); });
}

}