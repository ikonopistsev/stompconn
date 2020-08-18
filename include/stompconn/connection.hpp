#pragma once

#include "stompconn/stomplay.hpp"
#include "btpro/tcp/bev.hpp"
#include <map>

namespace stompconn {

class connection
{
public:
    typedef std::function<void()> on_connect_type;
    typedef std::function<void(short)> on_event_type;

private:
    btpro::queue_ref queue_{};
    btpro::tcp::bev bev_{};

    on_event_type event_fun_{};
    on_connect_type on_connect_fun_{};

    stomplay stomplay_{};

    template<class A>
    struct proxy
    {
        static inline void evcb(bufferevent *, short what, void *self) noexcept
        {
            assert(self);
            static_cast<A*>(self)->do_evcb(what);
        }

        static inline void recvcb(bufferevent *hbev, void *self) noexcept
        {
            assert(self);
            btpro::buffer_ref input(bufferevent_get_input(hbev));
            static_cast<A*>(self)->do_recv(std::move(input));
        }
    };

    void do_evcb(short what) noexcept;

    void do_recv(btpro::buffer_ref input) noexcept;

    void create();

    void exec_subscribe(const stomplay::fun_type& fn, packet p);

    void exec_unsubscribe(const stomplay::fun_type& fn,
                          const std::string& id, packet p);

public:
    connection(btpro::queue_ref queue,
               on_event_type evfn, on_connect_type connfn) noexcept
        : queue_(std::move(queue))
        , event_fun_(evfn)
        , on_connect_fun_(connfn)
    {
        assert(evfn);
        assert(connfn);
    }

    void connect(const btpro::ip::addr& addr);

    template<class Rep, class Period>
    void connect(const btpro::ip::addr& addr,
                 std::chrono::duration<Rep, Period> timeout)
    {
        connect(addr);

        auto tv = btpro::make_timeval(timeout);
        bev_.set_timeout(nullptr, &tv);
    }

    void connect(btpro::dns_ref dns, const std::string& host, int port);

    template<class Rep, class Period>
    void connect(btpro::dns_ref dns, const std::string& host, int port,
                 std::chrono::duration<Rep, Period> timeout)
    {
        connect(dns, host, port);

        auto tv = btpro::make_timeval(timeout);
        bev_.set_timeout(nullptr, &tv);
    }

    // stomp CONNECT
    void logon(stompconn::logon frame, stomplay::fun_type fn);

    void subscribe(stompconn::subscribe frame, stomplay::fun_type fn);

    void unsubscribe(std::string_view id, stomplay::fun_type fn);

    // stomp DISCONNECT
    void logout(stomplay::fun_type fn);

    void ack(stompconn::ack frame, stomplay::fun_type fn);

    void ack(stompconn::ack frame)
    {
        frame.write(bev_);
    }

    void ack(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);

        ack(stompconn::ack(p), std::move(fn));
    }

    void ack(const packet& p)
    {
        ack(stompconn::ack(p));
    }

    void nack(stompconn::nack frame, stomplay::fun_type fn);

    void nack(stompconn::nack frame)
    {
        frame.write(bev_);
    }

    void nack(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);

        nack(stompconn::nack(p), std::move(fn));
    }

    void nack(const packet& p)
    {
        nack(stompconn::nack(p));
    }

    void begin(std::string_view transaction_id, stomplay::fun_type fn);

    void begin(stompconn::begin frame, stomplay::fun_type fn);

    void begin(std::string_view transaction_id);

    void begin(stompconn::begin frame)
    {
        frame.write(bev_);
    }

    void commit(std::string_view transaction_id, stomplay::fun_type fn);

    void commit(stompconn::commit frame, stomplay::fun_type fn);

    void commit(std::string_view transaction_id);

    void commit(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);

        commit(stompconn::commit(p), std::move(fn));
    }

    void commit(const packet& p)
    {
        commit(stompconn::commit(p));
    }

    void commit(stompconn::commit frame)
    {
        frame.write(bev_);
    }

    void abort(std::string_view transaction_id, stomplay::fun_type fn);

    void abort(stompconn::abort frame, stomplay::fun_type fn);

    void abort(std::string_view transaction_id);

    void abort(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);

        abort(stompconn::abort(p), std::move(fn));
    }

    void abort(const packet& p)
    {
        abort(stompconn::abort(p));
    }

    void abort(stompconn::abort frame)
    {
        frame.write(bev_);
    }

    void send(stompconn::logon frame, stomplay::fun_type fn);

    void send(stompconn::subscribe frame, stomplay::fun_type fn);

    void send(stompconn::ack frame, stomplay::fun_type fn);

    void send(stompconn::ack frame);

    void send(stompconn::nack frame, stomplay::fun_type fn);

    void send(stompconn::nack frame);

    void send(stompconn::begin frame, stomplay::fun_type fn);

    void send(stompconn::begin frame);

    void send(stompconn::commit frame, stomplay::fun_type fn);

    void send(stompconn::commit frame);

    void send(stompconn::abort frame, stomplay::fun_type fn);

    void send(stompconn::abort frame);

    void send(stompconn::send frame, stomplay::fun_type fn);

    void send(stompconn::send frame)
    {
        frame.write(bev_);
    }

    void on_error(stomplay::fun_type fn);
};

} // namespace stomptalk

