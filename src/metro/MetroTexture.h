#pragma once
#include "mycommon.h"

class MetroTexture {
public:
    enum class PixelFormat : uint32_t {
        Invalid                 = ~0u,
        BC1                     = 0,
        BC3                     = 1,
        RGBA8_UNORM             = 2,
        RGBA8_SNORM             = 3,
        BC6H                    = 4,
        BC7                     = 5,
        RG8_UNORM               = 6,
        RG8_SNORM               = 7,
        DEPTH_32F_S8            = 8,
        DEPTH_32F               = 9,
        R32_F                   = 10,
        RGBA16_F                = 11,
        RG16_F                  = 12,
        RGBA16_U                = 13,
        R8_UNORM                = 14,
        R8_U                    = 15,
        RGB10_UNORM_A2_UNORM    = 16,
        RGB10_SNORM_A2_UNORM    = 17,   //#NOTE_SK: this format is unavailable on PC for DirectX, tho available on Vulkan and XBox
        R11G11B10_F             = 18,
        R16_UNORM               = 19,
        R32_U                   = 20,
        RGBA32_F                = 21,
        PPS                     = 22,   //#NOTE_SK: have no clue wtf is this
        BGRA8_UNORM             = 23
    };

public:
    MetroTexture();
    ~MetroTexture();

    bool            LoadFromData(MemStream& stream, const MyHandle file);
    bool            LoadFromFile(const fs::path& fileName);

    bool            SaveAsDDS(const fs::path& filePath);
    bool            SaveAsLegacyDDS(const fs::path& filePath);
    bool            SaveAsTGA(const fs::path& filePath);
    bool            SaveAsPNG(const fs::path& filePath);
    bool            SaveAsMetroTexture(const fs::path& filePath);

    bool            IsCubemap() const;
    size_t          GetWidth() const;
    size_t          GetHeight() const;
    size_t          GetDepth() const;
    size_t          GetNumMips() const;
    PixelFormat     GetFormat() const;
    bool            HasAlpha() const;

    bool            GetRGBA(BytesArray& imagePixels) const;
    bool            GetBGRA(BytesArray& imagePixels) const;
    const uint8_t*  GetRawData() const;

private:
    bool            DecrunchTexture(const uint8_t* data, const size_t dataLength);

private:
    BytesArray      mData;
    bool            mIsCubemap;
    size_t          mWidth;
    size_t          mHeight;
    size_t          mDepth;
    size_t          mNumMips;
    PixelFormat     mFormat;
    bool            mHasAlpha;
};
