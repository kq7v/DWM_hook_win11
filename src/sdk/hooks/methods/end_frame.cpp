#include <sdk/hooks/hooks.hpp>
#include <sdk/hooks/methods/methods.hpp>
#include <sdk/dwm/dwm.hpp>
#include <backends/dwm/dwm.hpp>
#include <wrl/client.h>

namespace sdk::hook_methods {
    HRESULT __fastcall end_frame( void* context ) {
        // IsDisplayTarget() is also true for CDDASwapChain's capture buffer.
        // Only physical monitor passes may render the overlay into their native texture.
        if( !display_context || display_context != context ) {
            return end_frame_original( context );
        }
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        sdk::dwm->frame_texture( context, &texture );
        const auto result = end_frame_original( context );
        if( SUCCEEDED( result ) && texture ) {
            try {
                backends::dwm->render( texture.Get( ) );
            } catch( ... ) {
                hooks->enabled = false;
                backends::dwm->shutdown( );
            }
        }
        return result;
    }
}
