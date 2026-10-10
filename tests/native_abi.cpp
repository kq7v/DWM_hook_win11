#include <sdk/dwm/dwm.hpp>
#include <sdk/dwm/detail/layout.hpp>
#include <wrl/client.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void require( bool value, const char* message ) {
    if( !value ) {
        std::fprintf( stderr, "FAIL: %s\n", message );
        std::exit( 1 );
    }
}

int main( ) {
    wchar_t path[ 32768 ] { };
    if( !GetEnvironmentVariableW( L"DWM_REFERENCE_DLL", path, 32768 ) ) {
        std::puts( "SKIP: set DWM_REFERENCE_DLL to an offline dwmcore.dll" );
        return 77;
    }
    const auto module = LoadLibraryExW( path, nullptr, DONT_RESOLVE_DLL_REFERENCES );
    require( module && sdk::dwm->initialize( ), "resolve all native bindings without executing DWM" );
    const auto& native = sdk::detail::bindings;
    const auto base = reinterpret_cast<const unsigned char*>( module );
    require(
        native.hooks.end_frame - base == 0xc9c00 && native.hooks.compose - base == 0xd82a0 &&
            native.hooks.present - base == 0x22dd10,
        "guest function entry points"
    );
    require( native.drawing_target_offset == 0x20 && native.texture_offset == 16, "verified interface offsets" );

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    require(
        SUCCEEDED( D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context
        ) ),
        "create test device"
    );
    D3D11_TEXTURE2D_DESC desc { };
    desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    require( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, &texture ) ), "create test texture" );
    // Exercise the actual native getter and interface adjustor using the verified guest layout.
    alignas( void* ) std::array<unsigned char, 512> target { };
    alignas( void* ) std::array<unsigned char, 64> drawing { };
    auto put = []( auto& bytes, size_t offset, auto value ) {
        std::memcpy( bytes.data( ) + offset, &value, sizeof( value ) );
    };
    put( target, 0xf0, native.texture_table );
    put( target, 0x100, native.target_table );
    put( target, 0x68, texture.Get( ) );
    target[ 0x121 ] = 1;
    target[ 0x122 ] = 1;
    put( drawing, 0x20, target.data( ) + 0x100 );
    Microsoft::WRL::ComPtr<ID3D11Texture2D> found;
    require(
        SUCCEEDED( sdk::dwm->frame_texture( drawing.data( ), &found ) ) && found.Get( ) == texture.Get( ),
        "borrow the existing native texture with a COM reference"
    );
    found.Reset( );
    target[ 0x121 ] = 0;
    require(
        sdk::dwm->frame_texture( drawing.data( ), &found ) == S_FALSE && !found,
        "offscreen targets do not receive the overlay"
    );
    target[ 0x121 ] = 1;
    put( target, 0xf0, static_cast<void**>( nullptr ) );
    require( FAILED( sdk::dwm->frame_texture( drawing.data( ), &found ) ) && !found, "reject mismatched interface" );
    require(
        FAILED( sdk::dwm->frame_texture( nullptr, &found ) ) && !found &&
            sdk::dwm->frame_texture( nullptr, nullptr ) == E_POINTER,
        "null guards"
    );
    std::puts(
        "PASS: guest entry points, native virtual methods, interface adjustors, COM ownership and target filtering"
    );
}
