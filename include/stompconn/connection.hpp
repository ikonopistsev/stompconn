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

    std::size_t connection_id_{};
    std::size_t message_seq_id_{};

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

    void exec_subscribe(const stomplay::fun_type& fn, packet p) noexcept;

    void exec_unsubscribe(const stomplay::fun_type& fn,
                          const std::string& id, packet p) noexcept;

    std::string message_seq_id() noexcept;
    std::string connection_seq_id() noexcept;
    std::string create_id(char L) noexcept;

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

    const std::string& session() const noexcept
    {
        return stomplay_.session();
    }

    // stomp CONNECT
    [[deprecated("replaced by send logon frame")]]
    void logon(stompconn::logon frame, stomplay::fun_type fn);

    [[deprecated("replaced by send subscribe frame")]]
    void subscribe(stompconn::subscribe frame, stomplay::fun_type fn);

    void unsubscribe(std::string_view id, stomplay::fun_type fn);

    // stomp DISCONNECT
    void logout(stomplay::fun_type fn);

    [[deprecated("replaced by send ack frame")]]
    void ack(stompconn::ack frame, stomplay::fun_type fn);

    [[deprecated("Replaced by send ack frame")]]
    void ack(stompconn::ack frame)
    {
        send(std::move(frame));
    }

    void ack(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        send(stompconn::ack(p), std::move(fn));
    }

    void ack(const packet& p)
    {
        send(stompconn::ack(p));
    }

    [[deprecated("Replaced by send nack frame")]]
    void nack(stompconn::nack frame, stomplay::fun_type fn);

    [[deprecated("Replaced by send nack frame")]]
    void nack(stompconn::nack frame)
    {
        send(std::move(frame));
    }

    void nack(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        send(stompconn::nack(p), std::move(fn));
    }

    void nack(const packet& p)
    {
        send(stompconn::nack(p));
    }

    [[deprecated("replaced by send begin frame")]]
    void begin(stompconn::begin frame, stomplay::fun_type fn);

    [[deprecated("replaced by send begin frame")]]
    void begin(stompconn::begin frame)
    {
        send(std::move(frame));
    }

    void begin(std::string_view transaction_id, stomplay::fun_type fn)
    {
        assert(fn);
        send(stompconn::begin(transaction_id), std::move(fn));
    }

    void begin(std::string_view transaction_id)
    {
        send(stompconn::begin(transaction_id));
    }

    [[deprecated("replaced by send commit frame")]]
    void commit(stompconn::commit frame, stomplay::fun_type fn);

    [[deprecated("replaced by send commit frame")]]
    void commit(stompconn::commit frame)
    {
        send(std::move(frame));
    }

    void commit(std::string_view transaction_id, stomplay::fun_type fn);

    void commit(std::string_view transaction_id);

    void commit(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        send(stompconn::commit(p), std::move(fn));
    }

    void commit(const packet& p)
    {
        send(stompconn::commit(p));
    }

    [[deprecated("replaced by send abort frame")]]
    void abort(stompconn::abort frame, stomplay::fun_type fn);

    [[deprecated("replaced by send abort frame")]]
    void abort(stompconn::abort frame)
    {
        send(std::move(frame));
    }

    void abort(std::string_view transaction_id, stomplay::fun_type fn);


    void abort(std::string_view transaction_id);

    void abort(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        send(stompconn::abort(p), std::move(fn));
    }

    void abort(const packet& p)
    {
        send(stompconn::abort(p));
    }

    void send(stompconn::logon frame, stomplay::fun_type fn);

    void send(stompconn::subscribe frame, stomplay::fun_type fn);

    void send(stompconn::ack frame, stomplay::fun_type fn);

    void send(stompconn::nack frame, stomplay::fun_type fn);

    void send(stompconn::begin frame, stomplay::fun_type fn);

    void send(stompconn::commit frame, stomplay::fun_type fn);

    void send(stompconn::abort frame, stomplay::fun_type fn);

    void send(stompconn::send frame, stomplay::fun_type fn);

    void send(stompconn::frame frame)
    {
        frame.write(bev_);
    }

    void on_error(stomplay::fun_type fn);

    std::string create_subs_id() noexcept;

    std::string create_receipt_id() noexcept;

    std::string create_message_id() noexcept;
};

} // namespace stomptalk

