#pragma once

#include <windows.h>
#include <cstddef>

namespace sdk {
    struct c_dwm_addresses {
        const std::byte* end_frame = nullptr;
        const std::byte* legacy_render_and_present = nullptr;
        const std::byte* display_render_and_present = nullptr;
        const std::byte* overlay_present = nullptr;

        void* const* texture_table = nullptr;
        void* const* target_table = nullptr;

        void( __fastcall* schedule_composition_pass )( UINT, UINT ) = nullptr;

        bool initialize( );
    };
}
