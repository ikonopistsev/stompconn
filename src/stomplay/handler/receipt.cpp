#include "stompconn/stomplay/handler/receipt.hpp"

namespace stompconn {
namespace stomplay {
namespace handler {

void receipt::exec(iterator i, frame p) noexcept
{
    try
    {
        auto& fn = std::get<1>(*i);
        assert(fn);
        fn(std::move(p));
    }
    catch (...)
    {   }
}

std::string_view receipt::create(std::string_view id, fn_type fn)
{
    auto& i = receipt_.emplace_front(id, std::move(fn));
    return { std::get<0>(i) };
}

std::string_view receipt::create(fn_type fn)
{
    auto receipt_id = std::to_string(++receipt_seq_id_);
    return create(std::string_view{receipt_id}, std::move(fn));
}

bool receipt::call(std::string_view id, frame p) noexcept
{
    auto i = receipt_.begin();
    auto e = receipt_.end();
    while (i != e)
    {
        auto& receipt_id = std::get<0>(*i);
        if (receipt_id == id)
        {
            // если нашли, то вызываем обработчик
            exec(i, std::move(p));
            // удаляем
            receipt_.erase(i);
            // выходим
            return true;
        }

        ++i;
    }

    return false;
}

void receipt::clear()
{
    receipt_.clear();
}

} // namespace handler
} // namespace stomplay
} // namespace stompconn