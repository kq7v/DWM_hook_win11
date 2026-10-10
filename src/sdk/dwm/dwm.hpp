#pragma once
#include <d3d11.h>
#include <array>

namespace sdk {
    class c_dwm {
        void** texture_table = nullptr;
        void** target_table = nullptr;
        ID3D11Texture2D*( __fastcall* get_texture )( void* ) = nullptr;
        void( __fastcall* schedule )( UINT, UINT ) = nullptr;

      public:

        std::array<const unsigned char*, 4> targets { };

        bool initialize( );
        HRESULT frame_texture( void* context, ID3D11Texture2D** texture ) const;
        void invalidate( void* monitor ) const;
        void request_frame( ) const;
    };

    extern c_dwm* const dwm;
}
