#include "stompconn/version.hpp"

namespace stompconn {

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

std::string_view version() noexcept
{
    constexpr std::string_view rc = STR(STOMPCONN_VERSION);
    return rc;
}

#undef STR_HELPER
#undef STR

} // namespace stompconn
