#include <backends/imgui_impl_dwm.hpp>
#include <core/input/input.hpp>
#include <backends/dwm/renderer.hpp>
#include <common/diagnostics/log.hpp>
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace backends {
    struct c_dwm::context_scope {
        c_dwm& owner;
        ImGuiContext* previous = ImGui::GetCurrentContext( );

        explicit context_scope( c_dwm& service ) : owner( service ) {
            if( !owner.context ) {
                IMGUI_CHECKVERSION( );
                owner.context = ImGui::CreateContext( );
                ImGui::SetCurrentContext( owner.context );
                auto& io = ImGui::GetIO( );
                io.IniFilename = io.LogFilename = nullptr;
                io.BackendPlatformName = "dwm_mouse";
                io.ConfigWindowsMoveFromTitleBarOnly = true;
            }

            ImGui::SetCurrentContext( owner.context );
        }

        ~context_scope( ) {
            ImGui::SetCurrentContext( previous );
        }
    };

    bool c_dwm::needs_redraw( bool open ) const noexcept {
        return ( open && active ) || visible || pending;
    }

    void c_dwm::render( ID3D11Texture2D* texture, bool open ) {
        std::lock_guard lock( mutex );
        pending_visible = false;
        pending = true;
        if( !open || !active || !draw_ui ) {
            if( !active ) {
                // Release GPU objects on DWM's rendering thread after native EndFrame.
                destroy_context( );
            }
            core::input->set_panel( { } );
            return;
        }

        context_scope scope( *this );
        const auto input = core::input->consume( );
        rescale( std::max( input.scale, 0.25f ) );
        if( !detail::renderer->new_frame( texture ) ) {
            diagnostics::log( "ImGui DWM renderer initialization failed" );
            return;
        }
        update_input( input );
        ImGui::NewFrame( );
        draw_ui( );
        ImGui::Render( );
        auto* data = ImGui::GetDrawData( );
        ImVec2 minimum = data->DisplaySize, maximum { 0, 0 };
        for( const auto* list : data->CmdLists ) {
            for( const auto& vertex : list->VtxBuffer ) {
                minimum.x = std::min( minimum.x, vertex.pos.x );
                minimum.y = std::min( minimum.y, vertex.pos.y );
                maximum.x = std::max( maximum.x, vertex.pos.x );
                maximum.y = std::max( maximum.y, vertex.pos.y );
            }
        }
        const RECT panel = data->TotalVtxCount ? RECT {
            static_cast<LONG>( std::floor( std::max( 0.0f, minimum.x ) ) ),
            static_cast<LONG>( std::floor( std::max( 0.0f, minimum.y ) ) ),
            static_cast<LONG>( std::ceil( std::min( data->DisplaySize.x, maximum.x ) ) ),
            static_cast<LONG>( std::ceil( std::min( data->DisplaySize.y, maximum.y ) ) )
        } : RECT { };
        core::input->set_panel( panel );
        detail::renderer->draw( data );
        pending_visible = data->TotalVtxCount != 0;
        static unsigned calls = 0;
        if( ++calls <= 3 ) {
            diagnostics::log( "ImGui frame rendered: vertices=%d", data->TotalVtxCount );
        }
    }

    void c_dwm::presented( ) noexcept {
        if( pending.exchange( false ) ) {
            visible = pending_visible;
        }
    }
}
