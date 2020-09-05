#pragma once

#include <string_view>

namespace stompconn {

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#undef STR_HELPER
#undef STR
std::string_view version() noexcept;

} // namespace stompconn
