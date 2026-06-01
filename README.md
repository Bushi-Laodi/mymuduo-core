# mymuduo-core

`mymuduo-core` 是一个基于 C++11 复现 muduo 网络库核心功能的学习型项目。项目主要用于理解 Reactor 网络模型、非阻塞 IO、事件循环、连接管理、线程模型和定时器机制等网络库底层原理。

本项目不是对 muduo 的完整复制，而是围绕 muduo 的核心思想进行精简复现，重点关注网络库主流程的实现和源码结构的学习。

## 项目目标

通过复现 muduo 网络库核心模块，掌握 C++ 高性能网络编程中的关键设计思想，包括：

* Reactor 事件驱动模型
* `EventLoop` 事件循环机制
* `Channel` 对 fd 事件的封装
* `Poller` / `EPollPoller` 对 IO 多路复用的封装
* `Acceptor` 新连接接收流程
* `TcpConnection` 连接生命周期管理
* `TcpServer` 服务器封装
* `TcpClient` / `Connector` 客户端连接管理
* `Buffer` 应用层缓冲区设计
* `Timer` / `TimerQueue` 定时器机制
* one loop per thread 线程模型

## 项目结构

```text
mymuduo
├── include/mymuduo/        # 头文件目录
│   ├── Acceptor.h
│   ├── Buffer.h
│   ├── Channel.h
│   ├── Connector.h
│   ├── EventLoop.h
│   ├── EventLoopThread.h
│   ├── EventLoopThreadPool.h
│   ├── InetAddress.h
│   ├── Poller.h
│   ├── Socket.h
│   ├── TcpClient.h
│   ├── TcpConnection.h
│   ├── TcpServer.h
│   ├── Timer.h
│   ├── TimerQueue.h
│   └── Timestamp.h
│
├── src/                    # 源文件目录
│   ├── Acceptor.cpp
│   ├── Buffer.cpp
│   ├── Channel.cpp
│   ├── Connector.cpp
│   ├── EPollPoller.cpp
│   ├── EventLoop.cpp
│   ├── EventLoopThread.cpp
│   ├── EventLoopThreadPool.cpp
│   ├── InetAddress.cpp
│   ├── Poller.cpp
│   ├── Socket.cpp
│   ├── TcpClient.cpp
│   ├── TcpConnection.cpp
│   ├── TcpServer.cpp
│   ├── TimerQueue.cpp
│   └── Timestamp.cpp
│
└── CMakeLists.txt
```

## 核心模块说明

### EventLoop

`EventLoop` 是整个网络库的核心，负责事件循环。每个线程最多拥有一个 `EventLoop`，通过 `loop()` 不断等待并处理 IO 事件、定时器事件和跨线程任务。

### Channel

`Channel` 是对文件描述符及其事件的封装。它不拥有 fd，只负责保存 fd 关心的事件、实际发生的事件以及对应的回调函数。

### Poller / EPollPoller

`Poller` 是 IO 多路复用的抽象基类，`EPollPoller` 是基于 Linux `epoll` 的具体实现，用于监听多个 fd 的读写事件。

### TcpConnection

`TcpConnection` 表示一条 TCP 连接，负责连接状态管理、读写事件处理、发送缓冲区、关闭连接和回调触发。

### TcpServer

`TcpServer` 是服务端封装，负责监听端口、接收新连接、创建 `TcpConnection`，并配合 `EventLoopThreadPool` 支持多线程网络模型。

### TcpClient / Connector

`TcpClient` 用于客户端连接管理，`Connector` 负责非阻塞 `connect` 流程。当连接成功后，将 socket fd 交给 `TcpConnection` 管理。

### TimerQueue

`TimerQueue` 基于 `timerfd` 实现定时器功能，并接入 `EventLoop`。它支持延迟任务和周期任务，可用于心跳检测、连接超时、定时清理等场景。

## 技术要点

* C++11
* Linux Socket API
* non-blocking socket
* epoll
* eventfd
* timerfd
* RAII
* smart pointer
* callback
* one loop per thread
* Reactor 模型

## 构建方式

进入项目根目录：

```bash
cd ~/myproject/mymuduo
```

创建构建目录：

```bash
mkdir -p build
cd build
```

执行 CMake：

```bash
cmake ..
```

编译项目：

```bash
make
```

## 学习收获

通过本项目，可以系统理解 muduo 网络库的核心设计，包括：

1. 为什么网络库需要 `EventLoop`
2. `Channel` 如何把 fd 事件和回调函数绑定起来
3. `epoll` 如何被封装到 `Poller` 中
4. `TcpConnection` 如何管理连接生命周期
5. 发送缓冲区如何处理非阻塞写不完的问题
6. 定时器如何通过 `timerfd` 接入事件循环
7. 多线程模型中为什么要保证一个 `EventLoop` 属于一个线程

## 后续计划

* 完善日志模块
* 增加 EchoServer 示例
* 增加 TcpClient 测试示例
* 完善连接超时和心跳检测
* 增加更完整的错误处理
* 结合 RPC 框架进一步实践网络库能力

## 项目定位

本项目主要用于学习和复现 muduo 网络库核心思想，适合作为 C++ 网络编程、Linux 高性能服务器开发、RPC 框架底层网络模块学习的基础项目。
