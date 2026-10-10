#include <sdk/dwm/dwm.hpp>
#include <sdk/dwm/detail/layout.hpp>

namespace sdk {
    HRESULT c_dwm::frame_texture( void* drawing_context, ID3D11Texture2D** texture ) const noexcept {
        if( !texture ) {
            return E_POINTER;
        }
        *texture = nullptr;
        const auto& native = detail::bindings;
        if( !drawing_context || !native.target_table ) {
            return E_NOINTERFACE;
        }
        auto* target = detail::read<void*>( drawing_context, native.drawing_target_offset );
        if( !target || detail::vtable( target ) != native.target_table ) {
            return E_NOINTERFACE;
        }
        if( !native.is_display( target ) ) {
            return S_FALSE;
        }
        auto* source = static_cast<char*>( target ) - native.texture_offset;
        if( detail::vtable( source ) != native.texture_table ) {
            return E_NOINTERFACE;
        }
        auto* borrowed = native.get_texture( source );
        return borrowed ? borrowed->QueryInterface( IID_PPV_ARGS( texture ) ) : E_POINTER;
    }
}
