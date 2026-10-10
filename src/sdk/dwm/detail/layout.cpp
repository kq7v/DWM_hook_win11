#include <sdk/dwm/detail/layout.hpp>
#include <common/memory/pattern.hpp>
#include <common/diagnostics/log.hpp>
#include <libhat/scanner.hpp>
#include <algorithm>
#include <span>
#include <vector>

namespace sdk::detail {
    native_bindings bindings;

    namespace {
        std::span<const std::byte> image;
        std::vector<std::span<const std::byte>> code_sections;
    }

    const unsigned char* relative( const unsigned char* code, std::size_t displacement ) {
        return code + displacement + sizeof( std::int32_t ) + read<std::int32_t>( code, displacement );
    }

    bool contains( const void* address, std::size_t size ) {
        const auto offset =
            reinterpret_cast<std::uintptr_t>( address ) - reinterpret_cast<std::uintptr_t>( image.data( ) );
        return offset <= image.size( ) && size <= image.size( ) - offset;
    }

    bool matches( hat::signature_view signature, const void* address ) {
        return std::ranges::any_of( code_sections, [ & ]( const auto region ) {
            const auto offset =
                reinterpret_cast<std::uintptr_t>( address ) - reinterpret_cast<std::uintptr_t>( region.data( ) );
            return offset <= region.size( ) && signature.size( ) <= region.size( ) - offset &&
                   std::equal( signature.begin( ), signature.end( ), static_cast<const std::byte*>( address ) );
        } );
    }

    bool initialize_layout( HMODULE module ) {
        // Verified with x64 dwmcore 10.0.26100.9278; each required ABI is checked before hooking.
        using namespace hat::literals;
        bindings = { };
        const auto loaded = module ? hat::process::module_at( module ) : std::nullopt;
        if( !loaded ) {
            return false;
        }
        image = loaded->get_module_data( );
        code_sections.clear( );
        loaded->for_each_section( []( std::string_view, const auto region, hat::protection protection ) {
            if( ( protection & hat::protection::Execute ) == hat::protection::Execute ) {
                code_sections.push_back( region );
            }
            return true;
        } );
        native_bindings next;
        next.hooks.end_frame = common::find_pattern(
            module, "40 56 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 48 89 9C 24 ? ? ? ? 48 89 7C 24 ? 48 8B "
                    "F9 4C 89 64 24 ? 45 33 E4 41 8B F4 E8 ? ? ? ? 48 39 77 ? 75 ? 48"
        );
        next.hooks.compose = common::find_pattern(
            module, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? "
                    "? ? 48 33 C4 48 89 45 ? 33 F6"
        );
        next.hooks.present = common::find_pattern(
            module, "40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 "
                    "48 89 45 ? 4C 8B 65 ? 48 8B D9"
        );
        const auto force = common::find_pattern(
            module, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? C6 81 ? 02 00 00 ? 48 8B F9 48 8B 59 ? 48 8B 71 ? 48 3B "
                    "DE 74 ? 48 8B 0B 48 83 C1 ? 48 8B 01 48 8B 40 ? E8 ? ? ? ? 48 83 C3"
        );
        const auto schedule = common::find_pattern(
            module,
            "48 83 EC ? 8B C1 48 8B 0D ? ? ? ? 48 85 C9 74 ? 80 B9 ? 19 00 00 00 74 ? 44 8B C2 8B D0 E8 ? ? ? ? 48"
        );
        // CDeviceTextureTarget's constructor stores its native interface tables.
        const auto tables = common::find_pattern(
            module, "48 8D 05 ? ? ? ? 48 89 83 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 83 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 83 ? ? "
                    "? ? 48 8D 05 ? ? ? ? 48 89 83 ? ? ? ? 8A 84 24 ? ? ? ? 48 89 8B ? ? ? ? 88 83 ? ? ? ? 48 8B C3"
        );
        if( !next.hooks.end_frame || !next.hooks.compose || !next.hooks.present || !force || !schedule || !tables ) {
            diagnostics::log(
                "bindings failed: end=%p compose=%p present=%p force=%p schedule=%p tables=%p", next.hooks.end_frame,
                next.hooks.compose, next.hooks.present, force, schedule, tables
            );
            return false;
        }
        next.texture_table = reinterpret_cast<void**>( const_cast<unsigned char*>( relative( tables ) ) );
        next.target_table = reinterpret_cast<void**>( const_cast<unsigned char*>( relative( tables + 28 ) ) );
        if( !contains( next.texture_table, 16 * sizeof( void* ) ) ||
            !contains( next.target_table, 17 * sizeof( void* ) ) ) {
            return false;
        }
        // IDeviceTexture::GetTexture2D and IDeviceTarget::IsDisplayTarget, validated below.
        next.get_texture = reinterpret_cast<texture_function>( next.texture_table[ 15 ] );
        next.is_display = reinterpret_cast<display_function>( next.target_table[ 16 ] );
        next.texture_offset = read<std::int32_t>( tables, 38 ) - read<std::int32_t>( tables, 10 );
        next.drawing_target_offset = next.hooks.end_frame[ 56 ];
        const auto texture_code = reinterpret_cast<const unsigned char*>( next.get_texture );
        if( next.texture_offset <= 0 || next.texture_offset >= 256 || next.drawing_target_offset != 0x20 ||
            !matches( "48 81 E9 ? ? ? ? E9 ? ? ? ?"_sig, texture_code ) ||
            !matches( "80 B9 ? ? ? ? 00 74 06 48 8B 41 20 C3"_sig, relative( texture_code + 7, 1 ) ) ||
            !matches( "8A 41 21 C3"_sig, reinterpret_cast<const void*>( next.is_display ) ) ) {
            return false;
        }
        next.force_dirty = reinterpret_cast<invalidate_function>( const_cast<unsigned char*>( force ) );
        next.schedule = reinterpret_cast<schedule_function>( const_cast<unsigned char*>( schedule ) );
        bindings = next;
        diagnostics::log( "DWM native bindings validated" );
        return true;
    }
}
