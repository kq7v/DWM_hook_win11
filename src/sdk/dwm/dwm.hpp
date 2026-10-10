#pragma once
#include <d3d11.h>

namespace sdk {
    struct hook_targets {
        const unsigned char* end_frame = nullptr;
        const unsigned char* compose = nullptr;
        const unsigned char* present = nullptr;
    };

    class c_dwm {
      public:

        bool initialize( );
        hook_targets targets( ) const noexcept;
        // DWM owns its objects; the returned COM texture has an additional reference.
        HRESULT frame_texture( void* drawing_context, ID3D11Texture2D** texture ) const noexcept;
        void invalidate( void* manager ) const noexcept;
        void request_frame( ) const noexcept;
    };

    extern c_dwm* const dwm;
}
