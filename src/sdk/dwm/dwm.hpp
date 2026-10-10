#pragma once

#include <sdk/dwm/addresses.hpp>
#include <sdk/dwm/interfaces/drawing_context.hpp>

namespace sdk {
    class c_dwm {
      public:

        c_dwm_addresses addresses;

        bool initialize( );
        HRESULT frame_texture( c_drawing_context* context, ID3D11Texture2D** texture ) const;
        void request_frame( ) const;
    };

    extern c_dwm* const dwm;
}
