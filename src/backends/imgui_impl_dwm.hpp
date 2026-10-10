#pragma once

#include <d3d11.h>
#include <atomic>
#include <mutex>

struct ImGuiContext;

namespace core {
    struct input_frame;
}

namespace backends {
    // The compositor supplies the texture; this backend owns only ImGui state.
    class c_dwm {
        std::mutex mutex;
        ImGuiContext* context = nullptr;
        void ( *draw_ui )( ) = nullptr;
        float current_scale = 0;
        std::atomic<bool> visible { false }, pending { false };
        std::atomic<bool> active { false };
        bool pending_visible = false;
        struct context_scope;
        void rescale( float requested );
        void update_input( const core::input_frame& frame );
        void destroy_context( );

      public:

        void initialize( void ( *callback )( ) );
        bool needs_redraw( bool open ) const noexcept;
        void render( ID3D11Texture2D* texture, bool open );
        void presented( ) noexcept;
        void shutdown( );
    };

    extern c_dwm* const dwm;
}
