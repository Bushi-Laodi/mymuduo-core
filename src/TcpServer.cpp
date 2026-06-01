#include "mymuduo/TcpServer.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/TcpConnection.h"
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace mymuduo {

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr,
                     const std::string& name, Option option)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(name),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name)),
      started_(0),
      nextConnId_(1) 
{
    acceptor_->setNewConnectionCallback([this](int sockfd, const InetAddress& peer) {
        newConnection(sockfd, peer);
    });
}

TcpServer::~TcpServer() 
{
    for (auto& item : connections_) {
        TcpConnectionPtr conn = item.second;
        item.second.reset();//shared_ptr 不再指向原来的 TcpConnection 对象。
        conn->getLoop()->runInLoop([conn] { conn->connectDestroyed(); });//销毁
    }
}

void TcpServer::setThreadNum(int numThreads) {
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {//线程池的初始化 + 监听loop中开始acceptor监听
    if (started_.fetch_add(1) == 0) {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop([this] { acceptor_->listen(); });
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_++);
    std::string connName = name_ + buf;

    sockaddr_in local;
    std::memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    ::getsockname(sockfd, reinterpret_cast<sockaddr*>(&local), &addrlen);
    InetAddress localAddr(local);

    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr& c) { removeConnection(c); });
    ioLoop->runInLoop([conn] { conn->connectEstablished(); });
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    loop_->runInLoop([this, conn] { removeConnectionInLoop(conn); });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    connections_.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop([conn] { conn->connectDestroyed(); });
}

}