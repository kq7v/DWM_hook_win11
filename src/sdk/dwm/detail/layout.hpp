#pragma once
#include <sdk/dwm/dwm.hpp>
#include <libhat/signature.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sdk::detail {
    using texture_function = ID3D11Texture2D*(__fastcall*)( void* );
    using display_function = bool( __fastcall* )( void* );
    using invalidate_function = void( __fastcall* )( void* );
    using schedule_function = void( __fastcall* )( UINT, UINT );

    struct native_bindings {
        hook_targets hooks;
        void** texture_table = nullptr;
        void** target_table = nullptr;
        texture_function get_texture = nullptr;
        display_function is_display = nullptr;
        std::ptrdiff_t texture_offset = 0;
        std::ptrdiff_t drawing_target_offset = 0;
        invalidate_function force_dirty = nullptr;
        schedule_function schedule = nullptr;
    };

    extern native_bindings bindings;
    bool initialize_layout( HMODULE module );
    bool contains( const void* address, std::size_t size );
    bool matches( hat::signature_view signature, const void* address );
    const unsigned char* relative( const unsigned char* code, std::size_t displacement = 3 );

    template <class T>
    T read( const void* address, std::ptrdiff_t offset = 0 ) {
        T value;
        std::memcpy( &value, static_cast<const unsigned char*>( address ) + offset, sizeof( value ) );
        return value;
    }

    inline void** vtable( void* object ) {
        return *static_cast<void***>( object );
    }
}
