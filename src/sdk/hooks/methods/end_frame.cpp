#include <sdk/hooks/methods/methods.hpp>
#include <sdk/dwm/dwm.hpp>
#include <backends/dwm/dwm.hpp>
#include <wrl/client.h>

namespace sdk::hook_methods {
    HRESULT __fastcall end_frame( c_drawing_context* context ) {
        if( !display_context || display_context != context ) {
            return end_frame_original( context );
        }

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        dwm->frame_texture( context, &texture );
        const auto result = end_frame_original( context );

        if( SUCCEEDED( result ) && texture ) {
            backends::dwm->render( texture.Get( ) );
        }

        return result;
    }
}
