#pragma once

#include "stompconn/stomplay.hpp"
#include "stomptalk/basic_text.hpp"
#include "btpro/tcp/bev.hpp"
#include "btpro/evcore.hpp"
#include <map>

namespace stompconn {

class connection
{
public:
    using on_connect_type = std::function<void()>;
    using on_event_type = std::function<void(short)>;
    using text_id_type = stomptalk::basic_text<char, 64>;
    using hex_text_type = stomptalk::basic_text<char, 20>;

private:
    btpro::queue_ref queue_{};
    btpro::tcp::bev bev_{};
    btpro::evs timeout_{};
    std::size_t write_timeout_{};
    std::size_t read_timeout_{};

    on_event_type event_fun_{};
    on_connect_type on_connect_fun_{};

    stomplay stomplay_{};

    std::size_t connection_seq_id_{};
    text_id_type connection_id_{};

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

        static inline void heart_beat(evutil_socket_t, short, void* self)
        {
            assert(self);
            static_cast<A*>(self)->send_heart_beat();
        }
    };

    void do_evcb(short what) noexcept;

    void setup_write_timeout();

    void setup_read_timeout();

    void do_recv(btpro::buffer_ref input) noexcept;

    void create();

    void exec_logon(const stomplay::fun_type& fn, packet p) noexcept;

    void exec_unsubscribe(const stomplay::fun_type& fn,
                          const std::string& id, packet p) noexcept;

    void exec_event_fun(short ef) noexcept;

    void update_connection_id() noexcept;

    hex_text_type message_seq_id() noexcept;

    text_id_type create_id(char ch) noexcept;

    void setup_heart_beat(const packet& logon);

    void send_heart_beat() noexcept;

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

    ~connection();

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

    void disconnect() noexcept;

    const std::string& session() const noexcept
    {
        return stomplay_.session();
    }

    void unsubscribe(std::string_view id, stomplay::fun_type fn);

    // stomp DISCONNECT
    void logout(stomplay::fun_type fn);

    void ack(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        auto transaction_id = p.get(stomptalk::header::tag::transaction());
        send(stompconn::ack(transaction_id), std::move(fn));
    }

    void ack(const packet& p)
    {
        auto transaction_id = p.get(stomptalk::header::tag::transaction());
        send(stompconn::ack(transaction_id));
    }

    void nack(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        auto id = p.get(stomptalk::header::tag::receipt());
        send(stompconn::nack(id), std::move(fn));
    }

    void nack(const packet& p)
    {
        auto transaction_id = p.get(stomptalk::header::tag::transaction());
        send(stompconn::nack(transaction_id));
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

    void commit(std::string_view transaction_id, stomplay::fun_type fn);

    void commit(std::string_view transaction_id);

    void commit(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        auto transaction_id = p.get(stomptalk::header::tag::transaction());
        send(stompconn::commit(transaction_id), std::move(fn));
    }

    void commit(const packet& p)
    {
        auto transaction_id = p.get(stomptalk::header::tag::transaction());
        send(stompconn::commit(transaction_id));
    }

    void abort(std::string_view transaction_id, stomplay::fun_type fn);


    void abort(std::string_view transaction_id);

    void abort(const packet& p, stomplay::fun_type fn)
    {
        assert(fn);
        auto transaction_id = p.get(stomptalk::header::tag::transaction());
        send(stompconn::abort(transaction_id), std::move(fn));
    }

    void abort(const packet& p)
    {
        auto transaction_id = p.get(stomptalk::header::tag::transaction());
        send(stompconn::abort(transaction_id));
    }

    void send(stompconn::logon frame, stomplay::fun_type fn);

    void send(stompconn::subscribe frame, stomplay::fun_type fn);

    void send(stompconn::ack frame, stomplay::fun_type fn);

    void send(stompconn::nack frame, stomplay::fun_type fn);

    void send(stompconn::begin frame, stomplay::fun_type fn);

    void send(stompconn::commit frame, stomplay::fun_type fn);

    void send(stompconn::abort frame, stomplay::fun_type fn);

    void send(stompconn::send frame, stomplay::fun_type fn);

    template<class F>
    void send(F frame)
    {
        setup_write_timeout();

        frame.write(bev_);
    }

    void on_error(stomplay::fun_type fn);

    text_id_type create_message_id() noexcept;
};

} // namespace stomptalk

