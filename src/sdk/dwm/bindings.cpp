#include <sdk/dwm/dwm.hpp>
#include <libhat/scanner.hpp>
#include <algorithm>
#include <cstring>
#include <vector>

namespace sdk {
    namespace {
        std::span<const std::byte> image;
        std::vector<std::span<const std::byte>> code;

        template <class T>
        T read( const void* address, std::ptrdiff_t offset = 0 ) {
            T result;
            std::memcpy( &result, static_cast<const unsigned char*>( address ) + offset, sizeof( result ) );
            return result;
        }

        bool contains( const void* address, std::size_t size ) {
            const auto offset =
                reinterpret_cast<std::uintptr_t>( address ) - reinterpret_cast<std::uintptr_t>( image.data( ) );
            return offset <= image.size( ) && size <= image.size( ) - offset;
        }

        const unsigned char* relative( const unsigned char* address, int displacement = 3 ) {
            return address + displacement + 4 + read<std::int32_t>( address, displacement );
        }

        bool matches( hat::signature_view signature, const void* address ) {
            return std::ranges::any_of( code, [ & ]( const auto region ) {
                const auto offset =
                    reinterpret_cast<std::uintptr_t>( address ) - reinterpret_cast<std::uintptr_t>( region.data( ) );
                return offset <= region.size( ) && signature.size( ) <= region.size( ) - offset &&
                       std::equal( signature.begin( ), signature.end( ), static_cast<const std::byte*>( address ) );
            } );
        }

        const unsigned char* find( std::string_view text ) {
            const auto signature = hat::parse_signature( text );
            const unsigned char* result = nullptr;
            if( !signature.has_value( ) ) {
                return nullptr;
            }
            for( const auto region : code ) {
                for( const auto match : hat::find_all_pattern( region, signature.value( ) ) ) {
                    if( result ) {
                        return nullptr;
                    }
                    result = reinterpret_cast<const unsigned char*>( match.get( ) );
                }
            }
            return result;
        }
    }

    bool c_dwm::initialize( ) {
        // Native x64 ABI: dwmcore 10.0.26100.9278. Reject missing or ambiguous signatures.
        using namespace hat::literals;
        const auto loaded = hat::process::module_at( GetModuleHandleW( L"dwmcore.dll" ) );
        if( !loaded ) {
            return false;
        }
        image = loaded->get_module_data( );
        code.clear( );
        loaded->for_each_section( []( std::string_view, auto region, hat::protection protection ) {
            if( ( protection & hat::protection::Execute ) == hat::protection::Execute ) {
                code.push_back( region );
            }
            return true;
        } );
        targets = {
            find(
                "40 56 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 48 89 9C 24 ? ? ? ? 48 89 7C 24 ? "
                "48 8B F9 4C 89 64 24 ? 45 33 E4 41 8B F4 E8 ? ? ? ? 48 39 77 ? 75 ? 48"
            ),
            find(
                "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? "
                "48 33 C4 48 89 45 ? 33 DB 48 89 55 ? 48 8B F9 4C 8B EA 48 8B 49 68"
            ),
            find(
                "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? "
                "48 33 C4 48 89 45 ? 4C 8B 79 70 33 DB 4C 8B EA 48 8B F9 8B F3 4D 85 FF"
            ),
            find(
                "40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? "
                "48 33 C4 48 89 45 ? 4C 8B 65 ? 48 8B D9"
            )
        };
        const auto timer = find(
            "48 83 EC ? 8B C1 48 8B 0D ? ? ? ? 48 85 C9 74 ? 80 B9 ? 19 00 00 00 74 ? "
            "44 8B C2 8B D0 E8 ? ? ? ? 48"
        );
        const auto tables = find(
            "48 8D 05 ? ? ? ? 48 89 83 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 83 ? ? ? ? "
            "48 8D 05 ? ? ? ? 48 89 83 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 83 ? ? ? ? "
            "8A 84 24 ? ? ? ? 48 89 8B ? ? ? ? 88 83 ? ? ? ? 48 8B C3"
        );
        if( std::ranges::find( targets, nullptr ) != targets.end( ) || !timer || !tables ) {
            return false;
        }
        texture_table = reinterpret_cast<void**>( const_cast<unsigned char*>( relative( tables ) ) );
        target_table = reinterpret_cast<void**>( const_cast<unsigned char*>( relative( tables + 28 ) ) );
        if( !contains( texture_table, 16 * sizeof( void* ) ) || !contains( target_table, sizeof( void* ) ) ||
            read<int>( tables, 38 ) - read<int>( tables, 10 ) != 16 || targets[ 0 ][ 56 ] != 0x20 ) {
            return false;
        }
        const auto getter = static_cast<const unsigned char*>( texture_table[ 15 ] );
        if( !matches( "48 81 E9 ? ? ? ? E9 ? ? ? ?"_sig, getter ) ||
            !matches( "80 B9 ? ? ? ? 00 74 06 48 8B 41 20 C3"_sig, relative( getter + 7, 1 ) ) ) {
            return false;
        }
        get_texture = reinterpret_cast<decltype( get_texture )>( texture_table[ 15 ] );
        schedule = reinterpret_cast<decltype( schedule )>( const_cast<unsigned char*>( timer ) );
        return true;
    }

}
