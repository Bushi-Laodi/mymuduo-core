#pragma once
#include "mymuduo/Buffer.h"
#include "mymuduo/Callbacks.h"
#include "mymuduo/InetAddress.h"
#include <atomic>
#include <memory>
#include <string>
#include "Timestamp.h"

namespace mymuduo{

class Channel;
class EventLoop;
class Socket;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, const Timestamp)>;    

    TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                  const InetAddress& local, const InetAddress& peer);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }

    void send(const std::string& message);
    void shutdown();

    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }
    void setCloseCallback(CloseCallback cb) { closeCallback_ = std::move(cb); }
    void setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t highWaterMark) {//待发送数据的缓存警戒线
        highWaterMarkCallback_ = std::move(cb);
        highWaterMark_ = highWaterMark;
    }

    void connectEstablished();
    void connectDestroyed();

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void setState(StateE s) { state_ = s; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();

    EventLoop* loop_;
    const std::string name_;
    std::atomic<int> state_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;//连接状态发生变化时的回调
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};
}
