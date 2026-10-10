#include <sdk/hooks/detail/callbacks.hpp>
#include <backends/imgui_impl_dwm.hpp>
#include <dxgi.h>

namespace sdk::hook_callbacks {
    HRESULT __fastcall present(
        void* self,
        void* chain,
        UINT flags,
        const dirty_rects& dirty,
        UINT interval,
        bool* submitted,
        bool legacy
    ) {
        bool local_submitted = false;
        auto* output = submitted ? submitted : &local_submitted;
        const auto result = present_original( self, chain, flags, dirty, interval, output, legacy );
        if( SUCCEEDED( result ) && *output && !( flags & DXGI_PRESENT_TEST ) ) {
            backends::dwm->presented( );
        }
        return result;
    }
}
