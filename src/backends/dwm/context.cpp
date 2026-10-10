#include <backends/dwm/dwm.hpp>
#include <backends/dx11/renderer.hpp>
#include <core/input/input.hpp>
#include <sdk/dwm/dwm.hpp>
#include <imgui.h>
#include <cmath>

namespace backends {
    namespace {
        c_dwm instance;
    }

    c_dwm* const dwm = &instance;

    c_dwm::context_scope::context_scope( ) : previous( ImGui::GetCurrentContext( ) ) {
    }

    c_dwm::context_scope::~context_scope( ) {
        ImGui::SetCurrentContext( previous );
    }

    void c_dwm::initialize( void ( *callback )( ) ) {
        std::lock_guard lock( mutex );
        draw_ui = callback;
        active = true;
    }

    void c_dwm::shutdown( ) {
        active = false;
        pending = true;
        core::input->set_panel( { } );
        sdk::dwm->request_frame( );
    }

    void c_dwm::rescale( float value ) {
        if( std::abs( value - scale ) < 0.01f ) {
            return;
        }
        detail::renderer->invalidate( );
        ImGui::GetStyle( ) = ImGuiStyle( );
        ImGui::StyleColorsDark( );
        ImGui::GetStyle( ).ScaleAllSizes( value );
        auto& fonts = *ImGui::GetIO( ).Fonts;
        fonts.Clear( );
        ImFontConfig config;
        config.SizePixels = 16.0f * value;
        fonts.AddFontDefault( &config );
        scale = value;
    }

    void c_dwm::destroy_context( ) {
        if( context ) {
            context_scope scope;
            if( scope.previous == context ) {
                scope.previous = nullptr;
            }
            ImGui::SetCurrentContext( context );
            detail::renderer->shutdown( );
            ImGui::DestroyContext( context );
            context = nullptr;
        }
        scale = 0;
    }
}
