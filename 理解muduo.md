# 理解muduo

## 1、EchoServer的服务的初始化

**代码示例：**

```cpp
int main(){
    mymuduo::EventLoop loop;
    mymuduo::InetAddress listenAddr(8000);
    mymuduo::TcpServer server(&loop, listenAddr, "EchoServer");

    server.setConnectionCallback([](const mymuduo::TcpConnectionPtr& conn) {
        std::cout << conn->name()
                  << (conn->connected() ? " connected" : " disconnected")
                  << std::endl;
    });

    server.setMessageCallback([](const mymuduo::TcpConnectionPtr& conn,
                                 mymuduo::Buffer* buffer,
                                 const Timestamp) {
        std::string message = buffer->retrieveAllAsString();
        conn->send(message);
    });

    server.start();
    loop.loop();
    return 0;
}
```

### **线路分析**

#### **初始化线路**

```
main线程：

base_loop（mymuduo::EventLoop loop）: 主事件循环（main线程中）
	|
	V
server（mymuduo::TcpServer server(&loop, listenAddr, "EchoServer") : 服务端
	|
    |-->|-->Acceptor	
    |	|	|-->acceptor_socket ：创建并持有listenfd
    |   |	|-->accpetor_chanel	：绑定listenfd的读事件，
    |	|						  设置readCallBack= Acceptor::handleRead		
    |	|-->IOThreadPool ：管理多个IO线程，每个线程都有一个subLoop（IOLoop）
	|				
	V
server::start()
	|
	|-->|-->IOthread::start	-->启动loopthread线程
	|	|					-->每个线程IOLoop初始化并开启事件循环
	|	|-->acceptor::listen-->acceptor_socket::listen
	|						-->acceptor_channel::enbleReading 
	|						-->acceptor_channel::update
	|						-->base_loop::updateChannel
	|						-->EPollPoller::updateChannel
	|						-->epoll_ctl  
	|
	V
base_loop::loop
	|
	|-->|-->EPollPoller::poll	
	|	|		|-->epoll_wait
		|-->acceptor_channel::handleEvent{ readCallBack();//处理事件，选择}
                |-->acceptor::handleRead{ newConnectionCallBack();}
                |-->TCPServer::newConnection
                |-->NewConnectionPtr
```

#### **新连接线路**

```
base_loop::loop
	|
	V
EPollPoller::poll{ epoll_wait();}
	|		
	V
acceptor_channel::handleEvent{ readCallBack();//处理事件，选择}
	|
    V
acceptor::handleRead{ connfd = accept();
	|				newConnectionCallBack();}
    |
    V
TCPServer::newConnection{
	|						ioLoop = threadpool->getNext();
	|						TcpConnectionPtr conn(new TcpConnection(...));
	|						//构造函数创建connSocket 和 connChannel
	|						connections_[connName] = conn;//将连接注册到记录表中
	|						conn绑定回调函数
    |                   }
    |
    V
IOLoop::runInLoop{ conn->connectEstablished();}
	|
	|
	V
TcpConnection::connectEstablished{	//要判断是否再IOLoop的线程中
									channel_->tie(shared_from_this());
									//将Channel与TcpConnection连接，确保回调函数的生命周期
    								channel_->enableReading();
    								//这样这个连接就可以在epoll中轮询}
```

**简化版**

```
新连接流程：

base_loop epoll_wait 监听 listenfd
    ↓
listenfd 可读
    ↓
acceptorChannel_::handleEvent
    ↓
Acceptor::handleRead
    ↓
accept() 得到 connfd
    ↓
TcpServer::newConnection
    ↓
选择一个 IOLoop
    ↓
创建 TcpConnectionPtr conn
    ↓
conn 内部创建 socket_ 和 channel_
    ↓
IOLoop::runInLoop(conn->connectEstablished)
    ↓
conn->channel_->enableReading()
    ↓
connfd 注册到 IOLoop 的 epoll
```

```
base_loop::epoll_wait监听listenfd
	|
	|
	V
listenfd::可读状态
	|
	|
	V
Channel::handleReadEvent
	|
	|
	V
Acceptor::handleRead
	|
	|
	V
acceptor() 得到 connfd 并调用newConnectionCallback
	|
	|
	V
TCPServer::newCnnection{
    |					获取一个IOLoop
    |					创建TCPNeconnection conn
    |					conn构造函数创建connSocket、connChannel
    |                   绑定回调函数
    |					IOLoop->runInLoop({conn->connectEstablished()})}
	|						
	V                    
TCPConnection::connectEstablished(){
    								确保在IO线程
    								更改Channe的状态
    								将Channel绑定TCPConnection，生命周期绑定，回调函数确保
    								Channel->enableRead();//让IOLoop的epoll监听connfd
									}                        
```

#### **信息处理路线**

```
客户端发送 hello
    |
    V
connfd 可读
    |
    V
IOLoop::loop
    |
    V
EPollPoller::poll(){epoll_wait();}
	|
	V
Channel::handleEvent()
	|
	V
Channel::handleEventWithGuard()::调用回调函数，根据 revents_ 判断当前发生了什么事件
	|
	V
TCPConnection::handRead(){inputBuffer_.readFd();//读区字数，如果有信息就调用message回调
	| 						//message回调由用户来写，没有就断开连接调用关闭回调，有错误就错误回调}
	|
	V
EchoServer::onMessage()
{
    从 inputBuffer_ 中取出 hello;
    conn->send(hello);
}
```

### 理解盘问

#### 第 1 题：

> `EventLoop`、`Poller`、`Channel` 三者分别负责什么？它们之间是什么关系？

我的回答：

```
EventLoop:是负责时间循环和任务分发的，在loop函数中进行事件的接收和处理，先用poller的poll函数获取新的时间的channel，在处理各个channel的handleEvent函数来处理事件，处理完成后就进行dopendingFunc函数，来处理添加到当前线程中执行的任务。
Poller:是对epoll的封装，poller本身也维持了一个感兴趣的fd列表，并对外部暴露修改感兴趣列表的接口，这个poller可以进行fd的添加和更新，并且监听目标fd，外部的可以通过调用poll函数来获取当前已经触发的感兴趣的事件。
Channel:是fd的封装但是他不持有fd，他持有对fd的增删改查的内容，并且维持fd的各种事件的回调函数
```

正确答案：

```
EventLoop 是 Reactor 的核心事件循环，负责在所属线程中不断调用 Poller::poll() 等待 IO 事件，并把返回的活跃 Channel 逐个分发处理，同时也会执行 pendingFunctors_ 中跨线程提交过来的任务。

Poller 是对 IO 多路复用机制的封装，在 Linux 下具体实现通常是 EPollPoller。它负责通过 epoll_ctl 注册、修改、删除 fd 事件，通过 epoll_wait 等待事件发生，并把发生事件的 fd 转换成对应的 Channel 返回给 EventLoop。

Channel 是对 fd 事件的封装。它不拥有 fd，只保存 fd、感兴趣事件 events_、实际发生事件 revents_，以及读、写、关闭、错误等回调函数。当 EventLoop 调用 Channel::handleEvent() 时，Channel 会根据 revents_ 调用对应回调。
```

