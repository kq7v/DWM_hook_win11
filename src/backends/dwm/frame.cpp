#include <backends/dwm/dwm.hpp>
#include <backends/dx11/renderer.hpp>
#include <core/input/input.hpp>
#include <sdk/hooks/hooks.hpp>
#include <features/menu/menu.hpp>
#include <imgui.h>
#include <algorithm>
#include <span>

namespace backends {
    bool c_dwm::needs_redraw( ) const {
        return ( active && sdk::hooks->enabled && features::menu->opened ) || visible || pending;
    }

    void c_dwm::render( ID3D11Texture2D* texture ) {
        std::lock_guard lock( mutex );

        pending_visible = false;
        pending = true;

        if( !active || !sdk::hooks->enabled || !features::menu->opened ) {
            if( !active ) {
                destroy_context( );
            }

            core::input->set_panel( { } );
            return;
        }

        context_scope scope;

        if( !context ) {
            IMGUI_CHECKVERSION( );
            context = ImGui::CreateContext( );
            ImGui::GetIO( ).IniFilename = ImGui::GetIO( ).LogFilename = nullptr;
            ImGui::GetIO( ).BackendPlatformName = "dwm_input";
            ImGui::GetIO( ).ConfigWindowsMoveFromTitleBarOnly = true;
        }

        ImGui::SetCurrentContext( context );
        const auto frame = core::input->consume( );
        rescale( std::max( frame.scale, 0.25f ) );

        if( !detail::renderer->new_frame( texture ) ) {
            core::input->set_panel( { } );
            return;
        }

        auto& io = ImGui::GetIO( );

        if( frame.reset ) {
            for( int i = 0; i < 5; ++i ) {
                io.AddMouseButtonEvent( i, false );
            }
        }

        for( const auto& event : std::span( frame.events ).first( frame.count ) ) {
            io.AddMousePosEvent( static_cast<float>( event.position.x ), static_cast<float>( event.position.y ) );
            io.AddMouseButtonEvent( event.button, event.down );
        }

        io.AddMousePosEvent( static_cast<float>( frame.position.x ), static_cast<float>( frame.position.y ) );
        io.AddMouseWheelEvent( 0, frame.wheel );

        ImGui::NewFrame( );
        draw_ui( );
        ImGui::Render( );

        detail::renderer->draw( ImGui::GetDrawData( ) );
        pending_visible = ImGui::GetDrawData( )->TotalVtxCount != 0;
    }

    void c_dwm::presented( ) {
        if( pending.exchange( false ) ) {
            visible = pending_visible;
        }
    }
}
