#pragma once

#include "stompconn/stomplay/frame.hpp"
#include "stompconn/stomplay/handler/id.hpp"

#include <functional>
#include <list>

namespace stompconn {
namespace stomplay {
namespace handler {

class receipt
{
    // сигнатура вызова
    using fn_type = std::function<void(frame)>;
    // данные для хранения
    using value_type = std::pair<std::string, fn_type>;
    // хранилище - просто список
    // подтвержения приходят в порядке их отправки
    using storage_type = std::list<value_type>;
    using iterator = storage_type::iterator;

    // последовательный номер подписки
    std::size_t receipt_seq_id_{};
    // хранилище обработчиков
    storage_type receipt_{};

    // вызов обработчика
    void exec(iterator i, frame p) noexcept;

public:
    receipt() = default;

    // установить собственный идентификтор подписки
    std::string& create(std::string_view id, fn_type fn);

    // использовать автоматический идентификатор подписки
    std::string& create(fn_type fn);
    
    bool call(std::string_view id, frame p) noexcept;

    void clear();
};

} // namespace handler
} // namspace stomplay
} // namespace stomptalk
