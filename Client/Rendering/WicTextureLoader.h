#pragma once

#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cstdint>
#include <filesystem>

namespace DungeonSync::Rendering
{
    struct LoadedTexture
    {
        Microsoft::WRL::ComPtr<
            ID3D11ShaderResourceView>
            shaderResourceView;

        std::uint32_t width{};
        std::uint32_t height{};
    };

    class WicTextureLoader final
    {
    public:
        WicTextureLoader() = default;
        ~WicTextureLoader() = default;

        WicTextureLoader(
            const WicTextureLoader&) = delete;

        WicTextureLoader& operator=(
            const WicTextureLoader&) = delete;

        WicTextureLoader(
            WicTextureLoader&&) = delete;

        WicTextureLoader& operator=(
            WicTextureLoader&&) = delete;

        [[nodiscard]]
        bool Initialize();

        [[nodiscard]]
        bool LoadFromFile(
            ID3D11Device& device,
            const std::filesystem::path& path,
            LoadedTexture& outputTexture) const;

    private:
        Microsoft::WRL::ComPtr<
            IWICImagingFactory>
            factory_;
    };
}