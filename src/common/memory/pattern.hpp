#pragma once

#include <windows.h>
#include <functional>
#include <string_view>

namespace common {
    const unsigned char* find_pattern(
        HMODULE module,
        std::string_view pattern,
        const std::function<bool( const unsigned char* )>& filter = { }
    );
}
