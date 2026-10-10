#include <backends/imgui_impl_dwm.hpp>
#include <core/input/input.hpp>
#include <backends/dwm/renderer.hpp>
#include <imgui.h>
#include <cmath>
#include <span>

namespace backends {
    namespace {
        c_dwm instance;
    }

    c_dwm* const dwm = &instance;

    void c_dwm::rescale( float requested ) {
        if( std::abs( requested - current_scale ) < 0.01f ) {
            return;
        }

        if( ImGui::GetIO( ).BackendRendererUserData ) {
            detail::renderer->invalidate( );
        }

        ImGui::GetStyle( ) = ImGuiStyle( );
        ImGui::StyleColorsDark( );
        ImGui::GetStyle( ).ScaleAllSizes( requested );
        auto& fonts = *ImGui::GetIO( ).Fonts;
        fonts.Clear( );
        ImFontConfig config;
        config.SizePixels = 16.0f * requested;
        fonts.AddFontDefault( &config );
        current_scale = requested;
    }

    void c_dwm::update_input( const core::input_frame& frame ) {
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
    }

    void c_dwm::initialize( void ( *callback )( ) ) {
        std::lock_guard lock( mutex );
        draw_ui = callback;
        active = true;
    }

    void c_dwm::shutdown( ) {
        active = false;
        core::input->set_panel( { } );
    }

    void c_dwm::destroy_context( ) {
        if( context ) {
            const auto previous = ImGui::GetCurrentContext( );
            ImGui::SetCurrentContext( context );
            detail::renderer->shutdown( );
            ImGui::DestroyContext( context );
            ImGui::SetCurrentContext( previous == context ? nullptr : previous );
            context = nullptr;
        }
        current_scale = 0;
    }
}
