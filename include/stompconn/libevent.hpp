#pragma once

#include <memory>
#include <chrono>
#include <string>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <type_traits>

#include "event2/dns.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"

#ifdef STOMPCONN_OPENSSL
#ifdef EVENT__HAVE_OPENSSL
#include "event2/bufferevent_ssl.h"
#endif
#endif

namespace stompconn {
namespace detail {

struct ref_buffer {

constexpr static inline evbuffer* create() noexcept
{
    return nullptr;
}

constexpr static inline void free(evbuffer*) noexcept
{   }

};

struct mem_buffer {

static evbuffer* create();

static void free(evbuffer* ptr) noexcept;

};

static inline void check_result(const char* what, int result)
{
    assert(what);

    if (-1 == result)
        throw std::runtime_error(what);
}

template<class T>
std::size_t check_size(const char* what, T result)
{
    assert(what);

    if (result == static_cast<T>(-1))
        throw std::runtime_error(what);

    return static_cast<std::size_t>(result);
}

template<class Rep, class Period>
static timeval make_timeval(std::chrono::duration<Rep, Period> timeout) noexcept
{
    const auto sec = std::chrono::duration_cast<
        std::chrono::seconds>(timeout);
    const auto usec = std::chrono::duration_cast<
        std::chrono::microseconds>(timeout) - sec;

    // FIX: decltype fix for clang macosx
    return {
        static_cast<decltype(timeval::tv_sec)>(sec.count()),
        static_cast<decltype(timeval::tv_usec)>(usec.count())
    };
}

} // namespace detail

template<class D>
class basic_buffer;

using buffer = basic_buffer<detail::mem_buffer>;
using buffer_ref = basic_buffer<detail::ref_buffer>;

template<class D>
class basic_buffer
{
    evbuffer* handle_{ D::create() };

    auto assert_handle() const noexcept
    {
        auto hbuf = handle();
        assert(hbuf);
        return hbuf;
    }

public:
    using this_type = basic_buffer<D>;

    evbuffer* handle() const noexcept
    {
        return handle_;
    }

    operator evbuffer*() const noexcept
    {
        return handle();
    }

    operator buffer_ref() const noexcept
    {
        return ref();
    }

    buffer_ref ref() const noexcept
    {
        return buffer_ref(handle());
    }

    basic_buffer() = default;

    ~basic_buffer()
    {
        D::free(handle());
    }

    // only for ref
    // this delete copy ctor for buffer&
    basic_buffer(const basic_buffer& other) noexcept
        : handle_(other.handle())
    {
        // copy only for refs
        static_assert(std::is_same<this_type, buffer_ref>::value);
    }

    // only for ref
    // this delete copy ctor for buffer&
    basic_buffer& operator=(const basic_buffer& other) noexcept
    {
        // copy only for refs
        static_assert(std::is_same<this_type, buffer_ref>::value);
        handle_ = other.handle();
        return *this;
    }

    basic_buffer(basic_buffer&& other) noexcept
    {
        std::swap(handle_, other.handle_);
    }

    basic_buffer& operator=(basic_buffer&& other) noexcept
    {
        std::swap(handle_, other.handle_);
        return *this;
    }

    explicit basic_buffer(evbuffer* ptr) noexcept
        : handle_(ptr)
    {   
        static_assert(std::is_same<this_type, buffer_ref>::value);
        assert(ptr);
    }

    template<class T>
    void append(const T& str_buf)
    {
        append(str_buf.data(), str_buf.size());
    }

    template<class T>
    void append(basic_buffer<T> buf)
    {
        detail::check_result("evbuffer_add_buffer",
            evbuffer_add_buffer(assert_handle(), buf));
    }

    void append(const void *data, std::size_t len)
    {
        assert(data && len);
        detail::check_result("evbuffer_add",
            evbuffer_add(assert_handle(), data, len));
    }

    void append_ref(const void *data, std::size_t len,
        evbuffer_ref_cleanup_cb cleanupfn, void *cleanupfn_arg)
    {
        assert(data && len);
        detail::check_result("evbuffer_add_reference",
            evbuffer_add_reference(assert_handle(), data, len,
                cleanupfn, cleanupfn_arg));
    }

    void append_ref(const void *data, std::size_t len)
    {
        append_ref(data, len, nullptr, nullptr);
    }

    template<class T>
    void append_ref(const T& str_buf)
    {
        append_ref(str_buf.data(), str_buf.size());
    }

    template<class T>
    void append(std::reference_wrapper<T> str_ref)
    {
        append_ref(str_ref.get());
    }

    // Prepends data to the beginning of the evbuffer
    void prepend(const void *data, std::size_t len)
    {
        assert((nullptr != data) && len);
        detail::check_result("evbuffer_prepend",
            evbuffer_prepend(assert_handle(), data, len));
    }

    // Prepends data to the beginning of the evbuffer
    template<class T>
    void prepend(const T& str_buf)
    {
        prepend(str_buf.data(), str_buf.size());
    }

    // Remove a specified number of bytes data from the beginning of an evbuffer.
    std::size_t drain(std::size_t len)
    {
        detail::check_result("evbuffer_drain",
            evbuffer_drain(assert_handle(), len));
        return size();
    }

    // Remove a specified number of bytes data from the beginning of an evbuffer.
    std::size_t drain(std::string& text, std::size_t max_len)
    {
        std::size_t total = (std::min)(size(), max_len);
        if (total)
        {
            text.resize(total);
            copyout(text.data(), total);
            drain(total);
        }
        return size();
    }

    // Expands the available space in the evbuffer to at least datlen,
    // so that appending datlen additional bytes
    // will not require any new allocations
    void expand(std::size_t size)
    {
        detail::check_result("evbuffer_expand",
            evbuffer_expand(assert_handle(), size));
    }

    std::size_t drain(std::string& text)
    {
        return drain(text, size());
    }

    // Read data from an evbuffer, and leave the buffer unchanged.
    std::size_t copyout(void *out, std::size_t len) const
    {
        assert(out && len);
        return detail::check_size("evbuffer_copyout",
            evbuffer_copyout(assert_handle(), out, len));
    }

    std::size_t copyout(buffer& other)
    {
        return detail::check_size("evbuffer_add_buffer",
            evbuffer_add_buffer(other, assert_handle()));
    }

    // Makes the data at the beginning of an evbuffer contiguous.
    unsigned char* pullup(ev_ssize_t len)
    {
        // @return a pointer to the contiguous memory array, or NULL
        // if param size requested more data than is present in the buffer.
        assert(len <= static_cast<ev_ssize_t>(size()));

        unsigned char *result = evbuffer_pullup(assert_handle(), len);
        if (!result)
        {
            if (empty())
            {
                static unsigned char nil;
                result = &nil;
            }
            else
                throw std::runtime_error("evbuffer_pullup");
        }
        return result;
    }

    // Returns the total number of bytes stored in the evbuffer
    std::size_t size() const noexcept
    {
        return evbuffer_get_length(assert_handle());
    }

    // Returns the number of contiguous available bytes in the first buffer chain.
    std::size_t contiguous_space() const noexcept
    {
        return evbuffer_get_contiguous_space(assert_handle());
    }

    std::string str() const
    {
        std::string rc;
        std::size_t len = size();
        if (len)
        {
            rc.resize(len);
            copyout(rc.data(), len);
        }
        return rc;
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    // Read data from an evbuffer and drain the bytes read.
    std::size_t remove(void *out, std::size_t len) const
    {
        assert(out && len);
        return detail::check_size("evbuffer_remove",
            evbuffer_remove(assert_handle(), out, len));
    }

    int write(evutil_socket_t fd)
    {
        return evbuffer_write(assert_handle(), fd);
    }

    template<class Ref>
    int write(basic_buffer<Ref> buf)
    {
        auto size = buf.size();
        buf.write(std::move(*this));
        return size;
    } 

    void reset(buffer other)
    {
        *this = std::move(other);
    } 
};

class bev
{
private:
    bufferevent* handle_{ nullptr };

    bufferevent* assert_handle() const noexcept;

    evbuffer* output_handle() const noexcept;

    evbuffer* input_handle() const noexcept;

public:
    bev() = default;
    ~bev() noexcept;

    bev(const bev&) = delete;
    bev& operator=(const bev&) = delete;

    bev(bev&& other) noexcept
    {
        std::swap(handle_, other.handle_);
    }

    bev& operator=(bev&& other) noexcept
    {
        std::swap(handle_, other.handle_);
        return *this;
    }

    bev(bufferevent* hbev) noexcept;

    bufferevent* handle() const noexcept
    {
        return handle_;
    }

    void attach(bufferevent* hbev) noexcept;

    void create(event_base* queue, evutil_socket_t fd, 
        int opt = BEV_OPT_CLOSE_ON_FREE);

#ifdef STOMPCONN_OPENSSL
#ifdef EVENT__HAVE_OPENSSL
    void create(event_base* queue, evutil_socket_t fd, struct ssl_st *ssl,
        int opt = BEV_OPT_CLOSE_ON_FREE);
#endif
#endif

    void destroy() noexcept;

    buffer_ref input() const noexcept
    {
        return buffer_ref(input_handle());
    }

    buffer_ref output() const noexcept
    {
        return buffer_ref(output_handle());
    }

    event_base* queue() const noexcept;

    void connect(const sockaddr* sa, ev_socklen_t len);

    void connect(evdns_base* dns, int af, const std::string& hostname, int port);

    void connect(evdns_base* dns, const std::string& hostname, int port)
    {
        connect(dns, AF_UNSPEC, hostname, port);
    }

    operator bufferevent*() const noexcept
    {
        return handle();
    }

    evutil_socket_t fd() const noexcept;

    void set(bufferevent_data_cb rdfn, bufferevent_data_cb wrfn,
        bufferevent_event_cb evfn, void *arg) noexcept;

    void set_timeout(timeval *timeout_read, timeval *timeout_write);

    void write(const void *data, std::size_t size)
    {
        detail::check_result("bufferevent_write",
            bufferevent_write(assert_handle(), data, size));
    }

    void write(evbuffer* handle)
    {
        assert(handle);
        detail::check_result("bufferevent_write_buffer",
            bufferevent_write_buffer(assert_handle(), handle));
    }

    template<class Ref>
    void write(basic_buffer<Ref> buf)
    {
        write(buf.handle());
    }

    buffer read()
    {
        buffer result;
        detail::check_result("bufferevent_read_buffer",
            bufferevent_read_buffer(assert_handle(), result));
        return result;
    } 

    void enable(short event)
    {
        detail::check_result("bufferevent_enable",
            bufferevent_enable(assert_handle(), event));
    }

    void disable(short event)
    {
        detail::check_result("bufferevent_disable",
            bufferevent_disable(assert_handle(), event));
    } 
};

// хэндл динамического эвента
class ev
{
private:
    static void deallocate(event* ptr) noexcept;

    event* handle_{ nullptr };

    auto assert_handle() const noexcept
    {
        assert(!empty());
        return handle();
    }

public:
    ev() = default;
    ev(ev&& other) noexcept;
    ev& operator=(ev&& other) noexcept;
    ~ev() noexcept;

    ev(const ev&) = delete;
    ev& operator=(const ev&) = delete;

    void destroy() noexcept;

    void create(event_base* queue, evutil_socket_t fd, short ef,
        event_callback_fn fn, void *arg);

    void create(event_base* queue, short ef, event_callback_fn fn, void *arg)
    {
        create(queue, -1, ef, fn, arg);
    }

    event* handle() const noexcept
    {
        return handle_;
    }

    operator event*() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return handle() == nullptr;
    }

    // !!! это не выполнить на следующем цикле очереди
    // это добавить без таймаута
    // допустим вечное ожидание EV_READ или сигнала
    void add(timeval* tv = nullptr);

    void add(timeval tv)
    {
        add(&tv);
    }

    template<class Rep, class Period>
    void add(std::chrono::duration<Rep, Period> timeout)
    {
        add(detail::make_timeval(timeout));
    }
};

timeval gettimeofday_cached(event_base* queue);

template<class T>
struct once
{
    static inline void call(evutil_socket_t, short, void* arg)
    {
        assert(arg);
        auto fn = static_cast<T*>(arg);
        try
        {
            (*fn)();
        }
        catch (...)
        {   }

        delete fn;
    }
};

using callback_type = std::function<void()>;

void make_once(event_base* queue, evutil_socket_t fd, 
    short ef, timeval tv, callback_type* fn);

} // namespace stompconn
