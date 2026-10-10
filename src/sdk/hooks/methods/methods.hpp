#pragma once
#include <windows.h>
#include <sdk/dwm/interfaces/drawing_context.hpp>
#include <sdk/dwm/interfaces/render_target.hpp>

namespace sdk::hook_methods {
    // DWM's release-mode std::vector<RECT> is passed by reference, including in Debug builds of this DLL.
    struct dirty_rects {
        const RECT* begin;
        const RECT* end;
        const RECT* capacity;
    };

    static_assert( sizeof( dirty_rects ) == 24 );
    using frame_function = HRESULT( __fastcall* )( c_drawing_context* );
    using monitor_function = HRESULT( __fastcall* )( c_render_target*, c_drawing_context* );
    using present_function = HRESULT( __fastcall* )( void*, void*, UINT, const dirty_rects&, UINT, bool*, bool );
    extern thread_local c_drawing_context* display_context;
    inline frame_function end_frame_original = nullptr;
    inline monitor_function legacy_original = nullptr, display_original = nullptr;
    inline present_function present_original = nullptr;

    HRESULT __fastcall end_frame( c_drawing_context* context );
    HRESULT __fastcall legacy( c_render_target* monitor, c_drawing_context* context );
    HRESULT __fastcall display( c_render_target* monitor, c_drawing_context* context );
    HRESULT __fastcall present( void*, void*, UINT, const dirty_rects&, UINT, bool*, bool );
}
