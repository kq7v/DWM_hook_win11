#include <sdk/hooks/hooks.hpp>
#include <sdk/hooks/detail/callbacks.hpp>
#include <sdk/dwm/dwm.hpp>
#include <backends/imgui_impl_dwm.hpp>
#include <features/menu/menu.hpp>
#include <wrl/client.h>

namespace sdk::hook_callbacks {
    HRESULT __fastcall end_frame( void* self ) {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        sdk::dwm->frame_texture( self, &texture );
        const auto result = end_frame_original( self );
        if( SUCCEEDED( result ) && texture ) {
            try {
                backends::dwm->render( texture.Get( ), hooks->enabled && features::menu->opened );
            } catch( ... ) {
                hooks->enabled = false;
            }
        }
        return result;
    }
}
