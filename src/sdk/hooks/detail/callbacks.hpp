#pragma once
#include <windows.h>

namespace sdk::hook_callbacks {
    // A release-mode std::vector<RECT> is passed by reference by DWM.
    struct dirty_rects {
        const RECT* begin;
        const RECT* end;
        const RECT* capacity;
    };

    static_assert( sizeof( dirty_rects ) == 24 );
    using frame_function = HRESULT( __fastcall* )( void* );
    using present_function = HRESULT( __fastcall* )( void*, void*, UINT, const dirty_rects&, UINT, bool*, bool );
    inline frame_function compose_original = nullptr, end_frame_original = nullptr;
    inline present_function present_original = nullptr;
    HRESULT __fastcall compose( void* self );
    HRESULT __fastcall end_frame( void* self );
    HRESULT __fastcall present( void*, void*, UINT, const dirty_rects&, UINT, bool*, bool );
}
