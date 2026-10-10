#include <sdk/dwm/dwm.hpp>

namespace sdk {
    namespace {
        c_dwm instance;
    }

    c_dwm* const dwm = &instance;

    bool c_dwm::initialize( ) {
        return addresses.initialize( );
    }

    HRESULT c_dwm::frame_texture( c_drawing_context* context, ID3D11Texture2D** texture ) const {
        *texture = nullptr;

        const auto target = context->target;
        if( !target || target->table != addresses.target_table ) {
            return E_NOINTERFACE;
        }

        const auto source = target->texture_interface( );
        if( source->table != addresses.texture_table ) {
            return E_NOINTERFACE;
        }

        const auto borrowed = source->get_texture( );

        return borrowed ? borrowed->QueryInterface( IID_PPV_ARGS( texture ) ) : E_POINTER;
    }

    void c_dwm::request_frame( ) const {
        addresses.schedule_composition_pass( 0, 0 );
    }
}
