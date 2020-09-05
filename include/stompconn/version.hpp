#pragma once

#include <string_view>

namespace stompconn {

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static constexpr std::string_view version = STR(STOMPCONN_VERSION);

#undef STR_HELPER
#undef STR

} // namespace stompconn