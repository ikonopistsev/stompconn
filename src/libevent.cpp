#include "stompconn/libevent.hpp"
#ifdef EVENT__HAVE_OPENSSL
#include "event2/bufferevent_ssl.h"
#endif

using namespace stompconn;

evbuffer* detail::mem_buffer::create()
{
    auto ptr = evbuffer_new();
    if (!ptr)
        throw std::runtime_error("evbuffer_new");
    return ptr;
}

void detail::mem_buffer::free(evbuffer* ptr) noexcept
{
    evbuffer_free(ptr);
}

bufferevent* bev::assert_handle() const noexcept
{
    auto hbev = handle();
    assert(hbev);
    return hbev;
}

evbuffer* bev::output_handle() const noexcept
{
    return bufferevent_get_output(assert_handle());
}

evbuffer* bev::input_handle() const noexcept
{
    return bufferevent_get_input(assert_handle());
}

bev::~bev() noexcept
{
    auto hbev = handle();
    if (nullptr != hbev)
        bufferevent_free(hbev);
}

bev::bev(bufferevent* hbev) noexcept
    : handle_(hbev)
{
    assert(hbev);
}

void bev::attach(bufferevent* hbev) noexcept
{
    handle_ = hbev;
}

void bev::create(event_base* queue, evutil_socket_t fd, int opt)
{
    assert(queue);
    auto hbev = bufferevent_socket_new(queue, fd, opt);
    detail::check_result("bufferevent_socket_new",
        hbev != nullptr ? 0 : -1);
    attach(hbev);
}

#if defined(EVENT__HAVE_OPENSSL) && defined(STOMPCONN_OPENSSL)
void bev::create(event_base* queue, evutil_socket_t fd, struct ssl_st *ssl, int opt)
{
    assert(ssl);
    assert(queue);
    auto hbev = bufferevent_openssl_socket_new(queue, 
        fd, ssl, BUFFEREVENT_SSL_CONNECTING, opt);
    detail::check_result("bufferevent_socket_new",
        hbev != nullptr ? 0 : -1);
    attach(hbev);
}
#endif

void bev::destroy() noexcept
{
    auto hbev = handle();
    if (nullptr != hbev)
    {
        bufferevent_free(hbev);
        handle_ = nullptr;
    }
}

event_base* bev::queue() const noexcept
{
    return bufferevent_get_base(assert_handle());
}

void bev::connect(const sockaddr* sa, ev_socklen_t len)
{
    assert(sa);
    detail::check_result("bufferevent_socket_connect",
        bufferevent_socket_connect(assert_handle(),
            const_cast<sockaddr*>(sa), static_cast<int>(len)));
}

void bev::connect(evdns_base* dns, int af, const std::string& hostname, int port)
{
    detail::check_result("bufferevent_socket_connect_hostname",
        bufferevent_socket_connect_hostname(assert_handle(), 
            dns, af, hostname.c_str(), port));
}

evutil_socket_t bev::fd() const noexcept
{
    return bufferevent_getfd(assert_handle());
}

void bev::set(bufferevent_data_cb rdfn, bufferevent_data_cb wrfn,
    bufferevent_event_cb evfn, void *arg) noexcept
{
    bufferevent_setcb(assert_handle(), rdfn, wrfn, evfn, arg);
}

void bev::set_timeout(timeval *timeout_read, timeval *timeout_write)
{
    bufferevent_set_timeouts(assert_handle(), timeout_read, timeout_write);
}

ev::ev(ev&& other) noexcept
{
    std::swap(handle_, other.handle_);
}

ev& ev::operator=(ev&& other) noexcept
{
    std::swap(handle_, other.handle_);
    return *this;
}

ev::~ev() noexcept
{
    if (handle_)
        deallocate(handle_);
}

void ev::deallocate(event* ptr) noexcept
{
    assert(ptr);
    event_del(ptr);
    event_free(ptr);
}

void ev::destroy() noexcept
{
    if (handle_)
    {
        deallocate(handle_);
        handle_ = nullptr;
    }
}

void ev::create(event_base* queue, evutil_socket_t fd, short ef,
    event_callback_fn fn, void *arg)
{
    auto handle = event_new(queue, fd, ef, fn, arg);
    if (!handle)
        throw std::runtime_error("event_new");
    handle_ = handle;
}

// !!! это не выполнить на следующем цикле очереди
// это добавить без таймаута
// допустим вечное ожидание EV_READ или сигнала
void ev::add(timeval* tv)
{
    detail::check_result("event_add", 
        event_add(assert_handle(), tv));
}

timeval stompconn::gettimeofday_cached(event_base* queue)
{
    timeval tv = {};
    detail::check_result("event_base_gettimeofday_cached", 
        event_base_gettimeofday_cached(queue, &tv));
    return tv;
}

void stompconn::make_once(event_base* queue, evutil_socket_t fd, 
    short ef, timeval tv, callback_type* fn)
{
    assert(fn);
    assert(queue);
    detail::check_result("event_base_once", 
        event_base_once(queue, fd, ef, 
            once<callback_type>::call, fn, &tv));
}
