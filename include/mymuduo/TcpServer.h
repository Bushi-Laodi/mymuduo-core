#pragma once 
#include "mymuduo/Acceptor.h"
#include "mymuduo/Callbacks.h"
#include "mymuduo/EventLoopThreadPool.h"
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

namespace mymuduo {
// 功能：
// 1、线程池的初始化 
// 2、监听loop中开始acceptor监听 
// 3、TcpConnection的生成回调函数来进行监听Loop和ioLoop的交互
class TcpServer {public:
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, const Timestamp)>;   
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    enum Option { kNoReusePort, kReusePort };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr,
              const std::string& name, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadNum(int numThreads);
    void setThreadInitCallback(ThreadInitCallback cb) { threadInitCallback_ = std::move(cb); }
    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }
    void start();

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_;
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;
    std::atomic<int> started_;
    int nextConnId_;
    ConnectionMap connections_;
};

}