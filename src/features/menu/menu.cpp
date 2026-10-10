#include <features/menu/menu.hpp>
#include <imgui.h>
#include <core/input/input.hpp>
#include <algorithm>

namespace features {
    namespace {
        c_menu instance;
    }

    c_menu* const menu = &instance;

    void c_menu::draw( ) {
        bool open = opened;
        const float scale = ImGui::GetFontSize( ) / 16.0f;
        const auto display = ImGui::GetIO( ).DisplaySize;
        const float margin = 8 * scale;
        const ImVec2 available { std::max( 1.0f, display.x - margin * 2 ), std::max( 1.0f, display.y - margin * 2 ) };
        ImGui::SetNextWindowSizeConstraints(
            { std::min( 300 * scale, available.x ), std::min( 190 * scale, available.y ) }, available
        );

        if( ImGui::Begin( "Hello world", &open, ImGuiWindowFlags_NoCollapse ) ) {
            ImGui::TextUnformatted( "Hello world!" );

            if( ImGui::Button( "Click me" ) ) {
                ++clicks;
            }

            ImGui::SameLine( );
            ImGui::Text( "Clicks: %u", clicks );
            ImGui::Checkbox( "Checkbox", &checked );
            ImGui::SliderFloat( "Value", &value, 0, 1 );
            ImGui::TextWrapped( "Insert: show/hide. End: stop hook." );
        }

        const auto position = ImGui::GetWindowPos( ), size = ImGui::GetWindowSize( );
        core::input->set_panel(
            { static_cast<LONG>( position.x ), static_cast<LONG>( position.y ),
              static_cast<LONG>( position.x + size.x ), static_cast<LONG>( position.y + size.y ) }
        );
        ImGui::End( );
        if( !open ) {
            opened = false;
        }
    }

}
