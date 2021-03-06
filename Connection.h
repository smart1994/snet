#ifndef CONNECTION_H
#define CONNECTION_H

#include "Buffer.h"
#include "EventLoop.h"
#include "SocketOps.h"
#include <functional>
#include <memory>
#include <queue>

namespace snet
{

enum class SendE : int
{
    OK = 0,
    Error = -1,
};

enum class RecvE : int
{
    PeerClosed = 0,
    NoAvailData = -1,
    Error = -2,
};

enum class ShutdownT
{
    Read,
    Write,
    Both
};

class Connection final
{
public:
    using OnSendComplete = std::function<void ()>;
    using OnReceivable = std::function<void ()>;
    using OnError = std::function<void ()>;

    Connection(int fd, EventLoop *loop);
    ~Connection();

    Connection(const Connection &) = delete;
    void operator = (const Connection &) = delete;

    int Send(std::unique_ptr<Buffer> buffer);
    int Recv(Buffer *buffer);
    void Shutdown(ShutdownT type);
    void Close();
    void SetTcpKeepAlive();
    void SetTcpNoDelay();
    bool GetPeerAddress(struct sockaddr_in *inet);

    void SetOnError(const OnError &oe);
    void SetOnReceivable(const OnReceivable &onr);
    void SetOnSendComplete(const OnSendComplete &osc);
    void ChangeEventLoop(EventLoop *loop);

private:
    class ConnectionEventHandler final : public EventHandler
    {
    public:
        explicit ConnectionEventHandler(Connection *connection)
            : events_(0),
              enabled_events_(0),
              connection_(connection)
        {
            events_ |= static_cast<int>(Event::Read);
            events_ |= static_cast<int>(Event::Write);
            EnableRead();
        }

        ConnectionEventHandler(const ConnectionEventHandler &) = delete;
        void operator = (const ConnectionEventHandler &) = delete;

        void EnableRead()
        {
            enabled_events_ |= static_cast<int>(Event::Read);
        }

        void EnableWrite()
        {
            enabled_events_ |= static_cast<int>(Event::Write);
        }

        void DisableRead()
        {
            enabled_events_ &= ~static_cast<int>(Event::Read);
        }

        void DisableWrite()
        {
            enabled_events_ &= ~static_cast<int>(Event::Write);
        }

        virtual int Fd() const override
        {
            return connection_->fd_;
        }

        virtual Event Events() const override
        {
            return static_cast<Event>(events_);
        }

        virtual Event EnabledEvents() const override
        {
            return static_cast<Event>(enabled_events_);
        }

        virtual void HandleRead() override
        {
            connection_->HandleRead();
        }

        virtual void HandleWrite() override
        {
            connection_->HandleWrite();
        }

    private:
        int events_;
        int enabled_events_;
        Connection *connection_;
    };

    using BufferQueue = std::queue<std::unique_ptr<Buffer>>;

    int WriteBuffer(const std::unique_ptr<Buffer> &buffer);
    void HandleRead();
    void HandleWrite();

    int fd_;
    EventLoop *loop_;
    OnError on_error_;
    OnReceivable on_recv_;
    OnSendComplete on_send_complete_;
    BufferQueue send_queue_;
    ConnectionEventHandler eh_;
};

} // namespace snet

#endif // CONNECTION_H
