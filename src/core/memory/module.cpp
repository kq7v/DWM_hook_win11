#include <core/memory/module.hpp>
#include <array>

namespace core {
    c_module::c_module( const wchar_t* name ) {
        const auto* base = reinterpret_cast<const std::byte*>( GetModuleHandleW( name ) );

        if( !base ) {
            return;
        }

        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>( base );
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>( base + dos->e_lfanew );

        image = { base, nt->OptionalHeader.SizeOfImage };
        sections = { IMAGE_FIRST_SECTION( nt ), nt->FileHeader.NumberOfSections };
    }

    hat::const_scan_result c_module::find( hat::signature_view signature ) const {
        hat::const_scan_result result;

        for( const auto& section : sections ) {
            if( !( section.Characteristics & IMAGE_SCN_MEM_EXECUTE ) ) {
                continue;
            }

            const auto region = image.subspan( section.VirtualAddress, section.Misc.VirtualSize );
            std::array<hat::const_scan_result, 2> matches;
            const auto end = hat::find_all_pattern( region, matches, signature ).second;

            if( end == matches.begin( ) ) {
                continue;
            }

            if( result.has_result( ) || end == matches.end( ) ) {
                return nullptr;
            }

            result = matches.front( );
        }

        return result;
    }

    hat::const_scan_result c_module::match( const void* address, hat::signature_view signature ) const {
        for( const auto& section : sections ) {
            if( !( section.Characteristics & IMAGE_SCN_MEM_EXECUTE ) ) {
                continue;
            }

            const auto region = image.subspan( section.VirtualAddress, section.Misc.VirtualSize );
            const auto offset =
                reinterpret_cast<std::uintptr_t>( address ) - reinterpret_cast<std::uintptr_t>( region.data( ) );

            if( offset <= region.size( ) && signature.size( ) <= region.size( ) - offset &&
                std::equal( signature.begin( ), signature.end( ), static_cast<const std::byte*>( address ) ) ) {
                return static_cast<const std::byte*>( address );
            }
        }

        return nullptr;
    }

    bool c_module::contains( const void* address, std::size_t size ) const {
        const auto offset =
            reinterpret_cast<std::uintptr_t>( address ) - reinterpret_cast<std::uintptr_t>( image.data( ) );

        return offset <= image.size( ) && size <= image.size( ) - offset;
    }
}
