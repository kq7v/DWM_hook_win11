#pragma once

#include <d3d11_1.h>
#include <wrl/client.h>

struct ImDrawData;

namespace backends::detail {
    class c_renderer {
        template <class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        struct renderer_state {
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;
            ComPtr<ID3D11DeviceContext1> context1;
            ComPtr<ID3DDeviceContextState> context_state;
            ComPtr<ID3D11RenderTargetView> target;
            ID3D11Texture2D* texture = nullptr;
            LONGLONG time = 0, frequency = 0;
            bool fonts = false;
        } state;

        struct device_state_scope;
        bool initialize( ID3D11Device* device );
        void update_time( );

      public:

        bool new_frame( ID3D11Texture2D* texture );
        void draw( ImDrawData* data );
        void invalidate( );
        void shutdown( );
    };

    extern c_renderer* const renderer;
}
