#include <core/memory/module.hpp>
#include <windows.h>

namespace core {
    c_module::c_module( const wchar_t* name ) {
        const auto loaded = hat::process::module_at( GetModuleHandleW( name ) );

        if( !loaded ) {
            return;
        }

        image = loaded->get_module_data( );
        loaded->for_each_section( [ this ]( std::string_view, auto region, hat::protection protection ) {
            if( ( protection & hat::protection::Execute ) == hat::protection::Execute ) {
                code.push_back( region );
            }

            return true;
        } );
    }

    hat::const_scan_result c_module::find( hat::signature_view signature ) const {
        hat::const_scan_result result;

        for( const auto region : code ) {
            for( const auto match : hat::find_all_pattern( region, signature ) ) {
                if( result.has_result( ) ) {
                    return nullptr;
                }

                result = match;
            }
        }

        return result;
    }

    hat::const_scan_result c_module::match( const void* address, hat::signature_view signature ) const {
        for( const auto region : code ) {
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
