#include <sdk/hooks/methods/methods.hpp>
#include <backends/dwm/dwm.hpp>

namespace sdk::hook_methods {
    thread_local c_drawing_context* display_context = nullptr;

    namespace {
        struct display_pass {
            c_drawing_context* previous = display_context;

            display_pass( c_render_target* monitor, c_drawing_context* context ) {
                display_context = context;

                if( backends::dwm->needs_redraw( ) ) {
                    monitor->force_full_dirty( );
                }
            }

            ~display_pass( ) {
                display_context = previous;
            }
        };
    }

    HRESULT __fastcall legacy( c_render_target* monitor, c_drawing_context* context ) {
        display_pass scope( monitor, context );

        return legacy_original( monitor, context );
    }

    HRESULT __fastcall display( c_render_target* monitor, c_drawing_context* context ) {
        display_pass scope( monitor, context );

        return display_original( monitor, context );
    }
}
