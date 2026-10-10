#include <sdk/hooks/methods/methods.hpp>
#include <sdk/dwm/dwm.hpp>
#include <backends/dwm/dwm.hpp>

namespace sdk::hook_methods {
    thread_local void* display_context = nullptr;

    namespace {
        struct display_pass {
            void* previous = display_context;

            display_pass( void* monitor, void* context ) {
                display_context = context;
                if( backends::dwm->needs_redraw( ) ) {
                    sdk::dwm->invalidate( monitor );
                }
            }

            ~display_pass( ) {
                display_context = previous;
            }
        };
    }

    HRESULT __fastcall legacy( void* monitor, void* context ) {
        display_pass scope( monitor, context );
        return legacy_original( monitor, context );
    }

    HRESULT __fastcall display( void* monitor, void* context ) {
        display_pass scope( monitor, context );
        return display_original( monitor, context );
    }
}
