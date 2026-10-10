#include <sdk/hooks/methods/methods.hpp>
#include <backends/dwm/dwm.hpp>
#include <dxgi.h>

namespace sdk::hook_methods {
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

        if( display_context && SUCCEEDED( result ) && *output && !( flags & DXGI_PRESENT_TEST ) ) {
            backends::dwm->presented( );
        }

        return result;
    }
}
