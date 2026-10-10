#include <sdk/hooks/hooks.hpp>
#include <sdk/hooks/detail/callbacks.hpp>
#include <sdk/dwm/dwm.hpp>
#include <backends/imgui_impl_dwm.hpp>
#include <features/menu/menu.hpp>

namespace sdk::hook_callbacks {
    HRESULT __fastcall compose( void* self ) {
        if( backends::dwm->needs_redraw( hooks->enabled && features::menu->opened ) ) {
            sdk::dwm->invalidate( self );
        }
        return compose_original( self );
    }
}
