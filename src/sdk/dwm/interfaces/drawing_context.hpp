#pragma once

#include <d3d11.h>
#include <cstddef>

namespace sdk {
    struct c_device_texture {
        static constexpr std::size_t get_texture_slot = 15;

        void* const* table;

        ID3D11Texture2D* get_texture( ) {
            using function = ID3D11Texture2D*(__fastcall*)( c_device_texture* );

            return reinterpret_cast<function>( table[ get_texture_slot ] )( this );
        }
    };

    struct c_device_target {
        void* const* table;

        c_device_texture* texture_interface( ) {
            return reinterpret_cast<c_device_texture*>( reinterpret_cast<std::byte*>( this ) - 0x10 );
        }
    };

    struct c_drawing_context {
        std::byte reserved[ 0x20 ];
        c_device_target* target;
    };

    static_assert( offsetof( c_drawing_context, target ) == 0x20 );
}
