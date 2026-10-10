#pragma once
#include <d3d11.h>
#include <atomic>
#include <mutex>

struct ImGuiContext;

namespace backends {
    class c_dwm {
        std::mutex mutex;
        ImGuiContext* context = nullptr;
        void ( *draw_ui )( ) = nullptr;
        float scale = 0;
        std::atomic<bool> active { false }, visible { false }, pending { false };
        bool pending_visible = false;

        struct context_scope {
            ImGuiContext* previous;
            context_scope( );
            ~context_scope( );
        };

        void rescale( float value );
        void destroy_context( );

      public:

        void initialize( void ( *callback )( ) );
        void shutdown( );
        bool needs_redraw( ) const;
        void render( ID3D11Texture2D* texture );
        void presented( );
    };

    extern c_dwm* const dwm;
}
