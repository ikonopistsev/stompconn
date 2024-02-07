#pragma once

#include "stompconn/stomplay/packet.hpp"

#include <functional>
#include <list>

namespace stompconn {
namespace stomplay {

class receipt_handler
{
    // сигнатура вызова
    using fn_type = std::function<void(packet)>;
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
    void exec(iterator i, packet p) noexcept;

public:
    receipt_handler() = default;

    // установить собственный идентификтор подписки
    std::string_view create(std::string_view id, fn_type fn);

    // использовать автоматический идентификатор подписки
    std::string_view create(fn_type fn);
    
    bool call(std::string_view id, packet p) noexcept;

    void clear();
};

} // namspace stomplay
} // namespace stomptalk
