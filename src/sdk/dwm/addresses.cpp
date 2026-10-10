#include <sdk/dwm/addresses.hpp>
#include <sdk/dwm/interfaces/drawing_context.hpp>
#include <core/memory/module.hpp>

namespace sdk {
    namespace signatures {
        using namespace hat::literals;

        constexpr auto end_frame =
            "40 56 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 48 89 9C 24 ? ? ? ? 48 89 7C 24 ? "
            "48 8B F9 4C 89 64 24 ? 45 33 E4 41 8B F4 E8 ? ? ? ? 48 39 77 20 75 ? 48"_sig;

        constexpr auto legacy_render_and_present =
            "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? "
            "48 33 C4 48 89 45 ? 33 DB 48 89 55 ? 48 8B F9 4C 8B EA 48 8B 49 68"_sig;

        constexpr auto display_render_and_present =
            "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? "
            "48 33 C4 48 89 45 ? 4C 8B 79 70 33 DB 4C 8B EA 48 8B F9 8B F3 4D 85 FF"_sig;

        constexpr auto overlay_present =
            "40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? "
            "48 33 C4 48 89 45 ? 4C 8B 65 ? 48 8B D9"_sig;

        constexpr auto schedule_composition_pass =
            "48 83 EC ? 8B C1 48 8B 0D ? ? ? ? 48 85 C9 74 ? 80 B9 ? 19 00 00 00 74 ? "
            "44 8B C2 8B D0 E8 ? ? ? ? 48"_sig;

        constexpr auto texture_target = "48 8D 05 ? ? ? ? 48 89 83 F0 00 00 00 48 8D 05 ? ? ? ? 48 89 83 ? ? ? ? "
                                        "48 8D 05 ? ? ? ? 48 89 83 00 01 00 00 48 8D 05 ? ? ? ? 48 89 83 ? ? ? ? "
                                        "8A 84 24 ? ? ? ? 48 89 8B ? ? ? ? 88 83 ? ? ? ? 48 8B C3"_sig;

        constexpr auto texture_getter = "80 B9 ? ? ? ? 00 74 06 48 8B 41 20 C3"_sig;
        constexpr auto texture_getter_thunk = "48 81 E9 ? ? ? ? E9 ? ? ? ?"_sig;
    }

    bool c_dwm_addresses::initialize( ) {
        const core::c_module module( L"dwmcore.dll" );

        end_frame = module.find( signatures::end_frame ).get( );
        legacy_render_and_present = module.find( signatures::legacy_render_and_present ).get( );
        display_render_and_present = module.find( signatures::display_render_and_present ).get( );
        overlay_present = module.find( signatures::overlay_present ).get( );

        schedule_composition_pass = reinterpret_cast<decltype( schedule_composition_pass )>(
            const_cast<std::byte*>( module.find( signatures::schedule_composition_pass ).get( ) )
        );
        const auto constructor = module.find( signatures::texture_target );

        if( !end_frame || !legacy_render_and_present || !display_render_and_present || !overlay_present ||
            !schedule_composition_pass || !constructor.has_result( ) ) {
            return false;
        }

        texture_table = reinterpret_cast<void* const*>( constructor.rel( 3 ) );
        target_table = reinterpret_cast<void* const*>( constructor.rel( 31 ) );

        if( !module.contains( texture_table, ( c_device_texture::get_texture_slot + 1 ) * sizeof( void* ) ) ||
            !module.contains( target_table, sizeof( void* ) ) ) {
            return false;
        }

        const auto getter = module.find( signatures::texture_getter );
        const auto thunk =
            module.match( texture_table[ c_device_texture::get_texture_slot ], signatures::texture_getter_thunk );

        return getter.has_result( ) && thunk.rel( 8 ) == getter.get( );
    }
}
