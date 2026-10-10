#pragma once

namespace sdk {
    struct c_render_target {
        void* const* table;

        void force_full_dirty( ) {
            using function = void( __fastcall* )( c_render_target* );
            reinterpret_cast<function>( table[ 6 ] )( this );
        }
    };
}
