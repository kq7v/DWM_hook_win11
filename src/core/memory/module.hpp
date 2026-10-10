#pragma once

#include <libhat/scanner.hpp>
#include <windows.h>

namespace core {
    class c_module {
        std::span<const std::byte> image;
        std::span<const IMAGE_SECTION_HEADER> sections;

      public:

        explicit c_module( const wchar_t* name );
        hat::const_scan_result find( hat::signature_view signature ) const;
        hat::const_scan_result match( const void* address, hat::signature_view signature ) const;
        bool contains( const void* address, std::size_t size ) const;
    };
}
