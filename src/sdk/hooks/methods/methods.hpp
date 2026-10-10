#pragma once
#include <windows.h>

namespace sdk::hook_methods {
    // DWM's release-mode std::vector<RECT> is passed by reference, including in Debug builds of this DLL.
    struct dirty_rects {
        const RECT* begin;
        const RECT* end;
        const RECT* capacity;
    };

    static_assert( sizeof( dirty_rects ) == 24 );
    using frame_function = HRESULT( __fastcall* )( void* );
    using monitor_function = HRESULT( __fastcall* )( void*, void* );
    using present_function = HRESULT( __fastcall* )( void*, void*, UINT, const dirty_rects&, UINT, bool*, bool );
    extern thread_local void* display_context;
    inline frame_function end_frame_original = nullptr;
    inline monitor_function legacy_original = nullptr, display_original = nullptr;
    inline present_function present_original = nullptr;

    HRESULT __fastcall end_frame( void* context );
    HRESULT __fastcall legacy( void* monitor, void* context );
    HRESULT __fastcall display( void* monitor, void* context );
    HRESULT __fastcall present( void*, void*, UINT, const dirty_rects&, UINT, bool*, bool );
}
