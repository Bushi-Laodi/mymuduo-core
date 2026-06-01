#pragma once
#include "mymuduo/Callbacks.h"
#include "mymuduo/Connector.h"
#include <memory>
#include <string>

namespace mymuduo {

class EventLoop;
class Timestamp;

class TcpClient {
public:
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, const Timestamp)>;    
    TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name);
    ~TcpClient();

    void connect();
    void disconnect();

    TcpConnectionPtr connection() const { return connection_; }

    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    std::shared_ptr<Connector> connector_;
    const std::string name_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    TcpConnectionPtr connection_;
    int nextConnId_;
};

}