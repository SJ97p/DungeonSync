#include "WicTextureLoader.h"

#include <limits>
#include <vector>
#include <utility>

using Microsoft::WRL::ComPtr;

namespace DungeonSync::Rendering
{
    bool WicTextureLoader::Initialize()
    {
        if (factory_ != nullptr)
        {
            return true;
        }

        const HRESULT result =
            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(
                    factory_.ReleaseAndGetAddressOf()));

        return SUCCEEDED(result);
    }

    bool WicTextureLoader::DecodeFromFile(
        const std::filesystem::path& path,
        DecodedImage& outputImage) const
    {
        outputImage = {};

        if (factory_ == nullptr ||
            path.empty())
        {
            return false;
        }

        ComPtr<IWICBitmapDecoder> decoder;

        HRESULT result =
            factory_->CreateDecoderFromFilename(
                path.c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                decoder.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        ComPtr<IWICBitmapFrameDecode> frame;

        result = decoder->GetFrame(
            0,
            frame.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        UINT width = 0;
        UINT height = 0;

        result = frame->GetSize(
            &width,
            &height);

        if (FAILED(result) ||
            width == 0 ||
            height == 0)
        {
            return false;
        }

        constexpr UINT BytesPerPixel = 4;

        if (width >
            (std::numeric_limits<UINT>::max)() /
            BytesPerPixel)
        {
            return false;
        }

        const UINT rowPitch =
            width * BytesPerPixel;

        if (height >
            (std::numeric_limits<UINT>::max)() /
            rowPitch)
        {
            return false;
        }

        const UINT imageSize =
            rowPitch * height;

        ComPtr<IWICFormatConverter> converter;

        result = factory_->CreateFormatConverter(
            converter.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        result = converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom);

        if (FAILED(result))
        {
            return false;
        }

        std::vector<std::uint8_t>
            pixels(imageSize);

        result = converter->CopyPixels(
            nullptr,
            rowPitch,
            imageSize,
            pixels.data());

        if (FAILED(result))
        {
            return false;
        }

        outputImage.width = width;
        outputImage.height = height;
        outputImage.rowPitch = rowPitch;
        outputImage.pixels = std::move(pixels);

        return true;
    }

    bool WicTextureLoader::CreateTextureFromDecodedImage(
        ID3D11Device& device,
        const DecodedImage& image,
        LoadedTexture& outputTexture)
    {
        outputTexture = {};

        if (image.width == 0 ||
            image.height == 0 ||
            image.rowPitch == 0 ||
            image.pixels.empty())
        {
            return false;
        }

        const std::uint64_t requiredByteCount =
            static_cast<std::uint64_t>(
                image.rowPitch) *
            image.height;

        if (requiredByteCount >
            image.pixels.size())
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC textureDescription{};

        textureDescription.Width = image.width;
        textureDescription.Height = image.height;
        textureDescription.MipLevels = 1;
        textureDescription.ArraySize = 1;
        textureDescription.Format =
            DXGI_FORMAT_R8G8B8A8_UNORM;

        textureDescription.SampleDesc.Count = 1;
        textureDescription.Usage =
            D3D11_USAGE_IMMUTABLE;

        textureDescription.BindFlags =
            D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initialData{};

        initialData.pSysMem =
            image.pixels.data();

        initialData.SysMemPitch =
            image.rowPitch;

        ComPtr<ID3D11Texture2D> texture;

        HRESULT result = device.CreateTexture2D(
            &textureDescription,
            &initialData,
            texture.ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC
            viewDescription{};

        viewDescription.Format =
            textureDescription.Format;

        viewDescription.ViewDimension =
            D3D11_SRV_DIMENSION_TEXTURE2D;

        viewDescription.Texture2D.MostDetailedMip = 0;
        viewDescription.Texture2D.MipLevels = 1;

        result = device.CreateShaderResourceView(
            texture.Get(),
            &viewDescription,
            outputTexture
            .shaderResourceView
            .ReleaseAndGetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        outputTexture.width = image.width;
        outputTexture.height = image.height;

        return true;
    }

    bool WicTextureLoader::LoadFromFile(
        ID3D11Device& device,
        const std::filesystem::path& path,
        LoadedTexture& outputTexture) const
    {
        DecodedImage decodedImage{};

        if (!DecodeFromFile(
            path,
            decodedImage))
        {
            return false;
        }

        return CreateTextureFromDecodedImage(
            device,
            decodedImage,
            outputTexture);
    }
}