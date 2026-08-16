#pragma once

#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cstdint>
#include <filesystem>
#include <vector>

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

    struct DecodedImage
    {
        std::vector<std::uint8_t> pixels;

        std::uint32_t width{};
        std::uint32_t height{};
        std::uint32_t rowPitch{};
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
        bool DecodeFromFile(
            const std::filesystem::path& path,
            DecodedImage& outputImage) const;

        [[nodiscard]]
        static bool CreateTextureFromDecodedImage(
            ID3D11Device& device,
            const DecodedImage& image,
            LoadedTexture& outputTexture);

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