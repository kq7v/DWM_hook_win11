#include <sdk/dwm/dwm.hpp>

namespace sdk {
    namespace {
        c_dwm instance;
    }

    c_dwm* const dwm = &instance;

    HRESULT c_dwm::frame_texture( void* context, ID3D11Texture2D** texture ) const {
        *texture = nullptr;
        const auto target = *reinterpret_cast<void**>( static_cast<char*>( context ) + 0x20 );
        if( !target || *static_cast<void***>( target ) != target_table ) {
            return E_NOINTERFACE;
        }
        const auto source = static_cast<char*>( target ) - 16;
        if( *reinterpret_cast<void***>( source ) != texture_table ) {
            return E_NOINTERFACE;
        }
        const auto borrowed = get_texture( source );
        return borrowed ? borrowed->QueryInterface( IID_PPV_ARGS( texture ) ) : E_POINTER;
    }

    void c_dwm::invalidate( void* monitor ) const {
        // IRenderTarget::ForceFullDirty, with the same interface pointer used by DWM.
        reinterpret_cast<void( __fastcall* )( void* )>( ( *static_cast<void***>( monitor ) )[ 6 ] )( monitor );
    }

    void c_dwm::request_frame( ) const {
        schedule( 0, 0 );
    }
}
