#include "stompconn/version.hpp"

namespace stompconn {

constexpr std::string_view version() noexcept
{
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
    constexpr std::string_view rc = STR(STOMPCONN_VERSION);
#undef STR_HELPER
#undef STR
    return rc;
}

} // namespace stompconn
