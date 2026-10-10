#include <common/memory/pattern.hpp>
#include <libhat/scanner.hpp>

namespace common {
    const unsigned char* find_pattern(
        const HMODULE module,
        const std::string_view pattern,
        const std::function<bool( const unsigned char* )>& filter
    ) {
        const auto loaded = module ? hat::process::module_at( module ) : std::nullopt;
        const auto signature = hat::parse_signature( pattern );

        if( !loaded || !signature.has_value( ) ) {
            return nullptr;
        }

        const unsigned char* result = nullptr;
        bool unique = true;

        loaded->for_each_section(
            [ & ]( std::string_view, std::span<const std::byte> region, const hat::protection protection ) {
                if( ( protection & hat::protection::Execute ) != hat::protection::Execute ) {
                    return true;
                }

                for( const auto match : hat::find_all_pattern( region, signature.value( ) ) ) {
                    const auto candidate = reinterpret_cast<const unsigned char*>( match.get( ) );

                    if( filter && !filter( candidate ) ) {
                        continue;
                    }

                    if( result ) {
                        unique = false;

                        return false;
                    }

                    result = candidate;
                }

                return true;
            }
        );

        return unique ? result : nullptr;
    }
}
