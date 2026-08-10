#include "MetroTexture.h"
#include "MetroFileSystem.h"
#include "VFXReader.h"
#include "MetroCompression.h"
#include "MetroBinArchive.h"
#include "MetroReflection.h"

#include "dds_utils.h"

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning (disable : 4793) // `anonymous namespace'::stbiw__outfile': function compiled as native
#include "stb_image_write.h"
#pragma warning (default : 4793)

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_JPEG
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include "../libs/crunch/inc/crn_decomp.h"

#include <fstream>
#include <intrin.h>
#include <cwctype>


struct TextureInfoRedux {
    CharString  bump_name;
    CharString  det_name;
    CharString  aux0_name;
    CharString  aux1_name;
    uint8_t     type;
    uint32_t    width;
    uint32_t    height;
    float       bump_height;
    uint8_t     displ_type;
    float       displ_height;
    bool        animated;
    bool        mipmapped;
    bool        streamable;
    uint8_t     priority;
    float       det_u_scale;
    float       det_v_scale;
    float       det_intesity;
    uint8_t     format;
    uint32_t    avg_color;

    void Serialize(MetroReflectionReader& s) {
        METRO_READ_MEMBER(s, bump_name);
        METRO_READ_MEMBER(s, det_name);
        METRO_READ_MEMBER(s, aux0_name);
        METRO_READ_MEMBER(s, aux1_name);
        METRO_READ_MEMBER(s, type);
        METRO_READ_MEMBER(s, width);
        METRO_READ_MEMBER(s, height);
        METRO_READ_MEMBER(s, bump_height);
        METRO_READ_MEMBER(s, displ_type);
        METRO_READ_MEMBER(s, displ_height);
        METRO_READ_MEMBER(s, animated);
        METRO_READ_MEMBER(s, mipmapped);
        METRO_READ_MEMBER(s, streamable);
        METRO_READ_MEMBER(s, priority);
        METRO_READ_MEMBER(s, det_u_scale);
        METRO_READ_MEMBER(s, det_v_scale);
        METRO_READ_MEMBER(s, det_intesity);
        METRO_READ_MEMBER(s, format);
        METRO_READ_MEMBER(s, avg_color);
    }
};


static size_t NumMipsFromResolution(const size_t resolution) {
    size_t result = 0;

    unsigned long index;
    if (_BitScanReverse64(&index, resolution)) {
        result = static_cast<size_t>(index) + 1;
    }

    return result;
}


MetroTexture::MetroTexture()
    : mIsCubemap(false)
    , mWidth(0)
    , mHeight(0)
    , mDepth(0)
    , mNumMips(0)
    , mFormat(PixelFormat::Invalid)
    , mHasAlpha(false)
{
}
MetroTexture::~MetroTexture() {
}


bool MetroTexture::LoadFromData(MemStream& stream, const MyHandle file) {
    bool result = false;

    const uint8_t* data = stream.GetDataAtCursor();
    const size_t length = stream.Remains();

    const MetroFileSystem& mfs = MetroFileSystem::Get();
    const CharString& fileName = mfs.GetName(file);

    const bool isBin = StrEndsWith(fileName, ".bin");
    if (isBin) { //#NOTE_SK: Redux versions
        MetroBinArchive bin(kEmptyString, stream, MetroBinArchive::kHeaderNotExist);
        MetroReflectionReader reader = bin.ReflectionReader();
        if (reader.Good()) {
            TextureInfoRedux texInfo;
            reader >> texInfo;

            // save the format so we don't have to guess it
            mFormat = scast<MetroTexture::PixelFormat>(texInfo.format);

            const MyHandle folder = mfs.GetParentFolder(file);
            if (folder != kInvalidHandle) {
                CharString textureName = mfs.GetName(file);

                CharString resStr = std::to_string(texInfo.width);

                CharString texName = textureName.substr(0, textureName.length() - 4) + '.' + resStr;
                MyHandle textureFile = mfs.FindFile(texName, folder);
                if (textureFile == kInvalidHandle) {
                    textureName = mfs.GetName(file);
                    texName = textureName.substr(0, textureName.length() - 4) + '.' + resStr + 'c';
                    textureFile = mfs.FindFile(texName, folder);
                }
                if (textureFile != kInvalidHandle) {
                    MemStream stream = mfs.OpenFileStream(textureFile);
                    if (stream) {
                        return this->LoadFromData(stream, textureFile);
                    }
                }
            }
        }
    } else {
        if (*rcast<const uint32_t*>(data) == cDDSFileSignature) {
            // this is a plain DDS file
            stream.SkipBytes(4); // skip DDS magic

            DDSURFACEDESC2 ddsHdr;
            DDS_HEADER_DXT10 dx10Hdr;

            stream.ReadStruct(ddsHdr);
            if (ddsHdr.ddpfPixelFormat.dwFourCC == PIXEL_FMT_FOURCC('D', 'X', '1', '0')) {
                stream.ReadStruct(dx10Hdr);

                switch (dx10Hdr.dxgiFormat) {
                    case DXGI_FORMAT_BC7_TYPELESS:
                    case DXGI_FORMAT_BC7_UNORM:
                    case DXGI_FORMAT_BC7_UNORM_SRGB: {
                        mFormat = PixelFormat::BC7;
                    } break;

                    case DXGI_FORMAT_BC6H_TYPELESS:
                    case DXGI_FORMAT_BC6H_SF16:
                    case DXGI_FORMAT_BC6H_UF16: {
                        mFormat = PixelFormat::BC6H;
                        mIsCubemap = true;
                    } break;

                    default:
                        return false;
                }
            } else {
                switch (ddsHdr.ddpfPixelFormat.dwFourCC) {
                    case PIXEL_FMT_FOURCC('D', 'X', 'T', '1'): {
                        mFormat = PixelFormat::BC1;
                    } break;

                    case PIXEL_FMT_FOURCC('D', 'X', 'T', '5'): {
                        mFormat = PixelFormat::BC3;
                    } break;

                    default:
                        return false;
                }
            }

            mWidth = ddsHdr.dwWidth;
            mHeight = ddsHdr.dwHeight;
            mDepth = 1;
            mNumMips = (ddsHdr.dwMipMapCount > 1) ? ddsHdr.dwMipMapCount : 1;

            mData.resize(stream.Remains());
            stream.ReadToBuffer(mData.data(), mData.size());

            result = true;
        } else {
            CharString extension = fs::path(fileName).extension().string();
            size_t dimension = 0, numMips = 0;
            if (extension == ".512" || extension == ".512c") {
                dimension = 512;
                numMips = 10;
            } else if (extension == ".1024" || extension == ".1024c") {
                dimension = 1024;
                numMips = 1;
            } else if (extension == ".2048" || extension == ".2048c") {
                dimension = 2048;
                numMips = 1;
            } else if (extension == ".4096") {
                dimension = 4096;
                numMips = 1;
            }

            const bool isCrunched = extension.back() == 'c';

            if (dimension > 0) {
                const size_t gameVersion = mfs.GetGameVersion();
                const bool isLZ4BC7 = (gameVersion == VFXReader::kVFXVersionUnknown)
                                        ? (mFormat == PixelFormat::Invalid)
                                        : (gameVersion >= VFXReader::kVFXVersionArktika1);

                if (isLZ4BC7) {
                    const size_t bc7size = DDS_GetCompressedSizeBC7(dimension, dimension, numMips);
                    mData.resize(bc7size);
                    const size_t uresult = MetroCompression::DecompressBlob(data, length, mData.data(), bc7size);
                    if (uresult != bc7size) {
                        mData.resize(0);
                    } else {
                        mWidth = dimension;
                        mHeight = dimension;
                        mDepth = 1;
                        mNumMips = numMips;
                        mFormat = PixelFormat::BC7;

                        result = true;
                    }
                } else {
                    bool ok;
                    if (isCrunched) {
                        ok = this->DecrunchTexture(data, length);
                    } else {
                        mData.resize(length);
                        std::memcpy(mData.data(), data, length);

                        if (mFormat == PixelFormat::Invalid) {
                            const size_t bc1size = DDS_GetCompressedSizeBC1(dimension, dimension, numMips);
                            mFormat = (length == bc1size) ? PixelFormat::BC1 : PixelFormat::BC3;
                        }

                        ok = true;
                    }

                    if (ok) {
                        mWidth = dimension;
                        mHeight = dimension;
                        mDepth = 1;
                        mNumMips = numMips;

                        result = true;
                    }
                }
            }
        }
    }

    return result;
}

bool MetroTexture::LoadFromFile(const fs::path& fileName) {
    bool result = false;

    std::wstring ext = fileName.extension().native();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    if (ext == L".tga" || ext == L".png" || ext == L".bmp") {
        std::ifstream file(fileName, std::ifstream::binary);
        if (file.good()) {
            BytesArray fileData;

            file.seekg(0, std::ios::end);
            fileData.resize(file.tellg());
            file.seekg(0, std::ios::beg);
            file.read(rcast<char*>(fileData.data()), fileData.size());
            file.close();

            int width, height, bpp;
            uint8_t* pixels = stbi_load_from_memory(fileData.data(), scast<int>(fileData.size()), &width, &height, &bpp, STBI_rgb_alpha);
            fileData.resize(0);
            if (pixels) {
                mData.resize(scast<size_t>(width) * scast<size_t>(height) * 4);
                memcpy(mData.data(), pixels, mData.size());

                stbi_image_free(pixels);

                mWidth = scast<size_t>(width);
                mHeight = scast<size_t>(height);
                mDepth = 1;
                mNumMips = 1;
                mFormat = PixelFormat::RGBA8_UNORM;
                mHasAlpha = (bpp == 4);

                result = true;
            }
        }
    }

    return result;
}

bool MetroTexture::SaveAsDDS(const fs::path& filePath) {
    bool result = false;

    DDSURFACEDESC2 ddsHdr;
    DDS_HEADER_DXT10 dx10Hdr;
    DDS_MakeDX10Headers(ddsHdr, dx10Hdr, mWidth, mHeight, mNumMips, mIsCubemap);

    std::ofstream file(filePath, std::ofstream::binary);
    if (file.good()) {
        file.write(rcast<const char*>(&cDDSFileSignature), sizeof(cDDSFileSignature));
        file.write(rcast<const char*>(&ddsHdr), sizeof(ddsHdr));
        file.write(rcast<const char*>(&dx10Hdr), sizeof(dx10Hdr));
        file.write(rcast<const char*>(mData.data()), mData.size());

        result = true;
    }

    return result;
}

//#TODO: also mips ?
bool MetroTexture::SaveAsLegacyDDS(const fs::path& filePath) {
    bool result = false;

    //#TODO: add support!
    if (this->IsCubemap()) {
        return false;
    }

    std::ofstream file(filePath, std::ofstream::binary);
    if (file.good()) {
        DDSURFACEDESC2 ddsHdr;
        DDS_MakeDX9Header(ddsHdr, mWidth, mHeight, 1);

        BytesArray rgbaPixels;
        this->GetRGBA(rgbaPixels);

        BytesArray bc3Blocks(DDS_GetCompressedSizeBC7(mWidth, mHeight, 1));
        DDS_CompressBC3(rgbaPixels.data(), bc3Blocks.data(), mWidth, mHeight);

        file.write(rcast<const char*>(&cDDSFileSignature), sizeof(cDDSFileSignature));
        file.write(rcast<const char*>(&ddsHdr), sizeof(ddsHdr));
        file.write(rcast<const char*>(bc3Blocks.data()), bc3Blocks.size());

        result = true;
    }

    return result;
}

bool MetroTexture::SaveAsTGA(const fs::path& filePath) {
    bool result = false;

    std::ofstream file(filePath, std::ofstream::binary);
    if (file.good()) {
        BytesArray bgraPixels;
        if (this->GetBGRA(bgraPixels)) {
            uint16_t hdr[9] = { 0 };
            hdr[1] = 2;
            hdr[6] = scast<uint16_t>(mWidth);
            hdr[7] = scast<uint16_t>(mHeight);
            hdr[8] = 32;
            file.write(rcast<const char*>(hdr), sizeof(hdr));

            const size_t pitch = mWidth * 4;
            const char* pixels = rcast<const char*>(bgraPixels.data()) + (pitch * (mHeight - 1));
            for (size_t y = 0; y < mHeight; ++y) {
                file.write(pixels, pitch);
                pixels -= pitch;
            }

            result = true;
        }
    }

    return result;
}

bool MetroTexture::SaveAsPNG(const fs::path& filePath) {
    bool result = false;

    std::ofstream file(filePath, std::ofstream::binary);
    if (file.good()) {
        BytesArray rgbaPixels;
        if (this->GetRGBA(rgbaPixels)) {
            const int success = stbi_write_png_to_func([](void* ptr, void* data, int size) {
                std::ofstream* filePtr = rcast<std::ofstream*>(ptr);
                filePtr->write(rcast<const char*>(data), size);
            }, &file, scast<int>(mWidth), scast<int>(mHeight), 4, rgbaPixels.data(), 0);

            result = (success > 0);
        }
    }

    return result;
}

bool MetroTexture::SaveAsMetroTexture(const fs::path& filePath) {
    bool result = false;

    size_t resolution = scast<size_t>(mWidth);

    BytesArray mipBuffer = mData;
    BytesArray nextMipBuffer; nextMipBuffer.resize(mipBuffer.size() / 4);

    if (resolution == 4096) {
        std::ofstream file(filePath.native() + L".4096", std::ofstream::binary);
        if (file.good()) {
            const size_t bc7Size = DDS_GetCompressedSizeBC7(resolution, resolution, 1);
            BytesArray bc7Buffer;
            bc7Buffer.resize(bc7Size);

            DDS_CompressBC7(mipBuffer.data(), bc7Buffer.data(), resolution, resolution);

            BytesArray lz4Buffer;
            MetroCompression::CompressBlob(bc7Buffer.data(), bc7Size, lz4Buffer);

            file.write(rcast<const char*>(lz4Buffer.data()), lz4Buffer.size());
            file.flush();
            file.close();
        }

        stbir_resize_uint8(mipBuffer.data(), scast<int>(resolution), scast<int>(resolution), 0,
                           nextMipBuffer.data(), scast<int>(resolution / 2), scast<int>(resolution / 2), 0, 4);

        resolution /= 2;
        mipBuffer = nextMipBuffer;
    }

    if (resolution == 2048) {
        std::ofstream file(filePath.native() + L".2048", std::ofstream::binary);
        if (file.good()) {
            const size_t bc7Size = DDS_GetCompressedSizeBC7(resolution, resolution, 1);
            BytesArray bc7Buffer;
            bc7Buffer.resize(bc7Size);

            DDS_CompressBC7(mipBuffer.data(), bc7Buffer.data(), resolution, resolution);

            BytesArray lz4Buffer;
            MetroCompression::CompressBlob(bc7Buffer.data(), bc7Size, lz4Buffer);

            file.write(rcast<const char*>(lz4Buffer.data()), lz4Buffer.size());
            file.flush();
            file.close();
        }

        stbir_resize_uint8(mipBuffer.data(), scast<int>(resolution), scast<int>(resolution), 0,
                           nextMipBuffer.data(), scast<int>(resolution / 2), scast<int>(resolution / 2), 0, 4);

        resolution /= 2;
        mipBuffer = nextMipBuffer;
    }

    if (resolution == 1024) {
        std::ofstream file(filePath.native() + L".1024", std::ofstream::binary);
        if (file.good()) {
            const size_t bc7Size = DDS_GetCompressedSizeBC7(resolution, resolution, 1);
            BytesArray bc7Buffer;
            bc7Buffer.resize(bc7Size);

            DDS_CompressBC7(mipBuffer.data(), bc7Buffer.data(), resolution, resolution);

            BytesArray lz4Buffer;
            MetroCompression::CompressBlob(bc7Buffer.data(), bc7Size, lz4Buffer);

            file.write(rcast<const char*>(lz4Buffer.data()), lz4Buffer.size());
            file.flush();
            file.close();
        }

        stbir_resize_uint8(mipBuffer.data(), scast<int>(resolution), scast<int>(resolution), 0,
                           nextMipBuffer.data(), scast<int>(resolution / 2), scast<int>(resolution / 2), 0, 4);

        resolution /= 2;
        mipBuffer = nextMipBuffer;
    }

    if (resolution == 512) {
        std::ofstream file(filePath.native() + L".512", std::ofstream::binary);
        if (file.good()) {
            const size_t bc7Size = DDS_GetCompressedSizeBC7(resolution, resolution, 10);
            BytesArray bc7Buffer;
            bc7Buffer.resize(bc7Size);

            uint8_t* bufferA = mipBuffer.data();
            uint8_t* bufferB = nextMipBuffer.data();

            uint8_t* bc7Blocks = bc7Buffer.data();
            size_t mipResolution = resolution;
            for (size_t i = 0; i < 10; ++i) {
                DDS_CompressBC7(bufferA, bc7Blocks, mipResolution, mipResolution);

                const size_t mipSize = DDS_GetCompressedSizeBC7(mipResolution, mipResolution, 1);
                bc7Blocks += mipSize;

                const size_t nextResolution = std::max<size_t>(4, mipResolution / 2);
                if (nextResolution != resolution) { //mip size changed - resample
                    stbir_resize_uint8(bufferA, scast<int>(mipResolution), scast<int>(mipResolution), 0,
                                       bufferB, scast<int>(nextResolution), scast<int>(nextResolution), 0, 4);

                    std::swap(bufferA, bufferB);
                }
                mipResolution = nextResolution;
            }

            BytesArray lz4Buffer;
            MetroCompression::CompressBlob(bc7Buffer.data(), bc7Size, lz4Buffer);

            file.write(rcast<const char*>(lz4Buffer.data()), lz4Buffer.size());
            file.flush();
            file.close();

            result = true;
        }
    }

    return result;
}

bool MetroTexture::IsCubemap() const {
    return mIsCubemap;
}

size_t MetroTexture::GetWidth() const {
    return mWidth;
}

size_t MetroTexture::GetHeight() const {
    return mHeight;
}

size_t MetroTexture::GetDepth() const {
    return mDepth;
}

size_t MetroTexture::GetNumMips() const {
    return mNumMips;
}

MetroTexture::PixelFormat MetroTexture::GetFormat() const {
    return mFormat;
}

bool MetroTexture::HasAlpha() const {
    return mHasAlpha;
}

bool MetroTexture::GetRGBA(BytesArray& imagePixels) const {
    bool result = false;

    if (!mData.empty()) {
        //#TODO: add support for other formats!
        if (mFormat == PixelFormat::BC7) {
            imagePixels.resize(mWidth * mHeight * 4);
            DDS_DecompressBC7(mData.data(), imagePixels.data(), mWidth, mHeight);
            result = true;
        } else if (mFormat == PixelFormat::BC3) {
            imagePixels.resize(mWidth * mHeight * 4);
            DDS_DecompressBC3(mData.data(), imagePixels.data(), mWidth, mHeight);
            result = true;
        } else if (mFormat == PixelFormat::BC1) {
            imagePixels.resize(mWidth * mHeight * 4);
            memset(imagePixels.data(), 255, imagePixels.size());
            DDS_DecompressBC1(mData.data(), imagePixels.data(), mWidth, mHeight, 4);
            result = true;
        }
    }

    return result;
}

bool MetroTexture::GetBGRA(BytesArray& imagePixels) const {
    const bool result = this->GetRGBA(imagePixels);
    if (result) {
        uint8_t* rgba = imagePixels.data();
        for (size_t i = 0; i < mWidth * mHeight; ++i) {
            std::swap(rgba[0], rgba[2]);
            rgba += 4;
        }
    }
    return result;
}

const uint8_t* MetroTexture::GetRawData() const {
    return mData.data();
}

bool MetroTexture::DecrunchTexture(const uint8_t* data, const size_t dataLength) {
    bool result = true;

    crnd::crn_texture_info info;
    info.m_struct_size = sizeof(crnd::crn_texture_info);
    if (crnd::crnd_get_texture_info(data, scast<uint32_t>(dataLength), &info)) {
        crnd::crnd_unpack_context ctx = crnd::crnd_unpack_begin(data, scast<uint32_t>(dataLength));

        const size_t blockSize = (info.m_format == cCRNFmtDXT1) ? 8 : 16;

        const size_t ddsSize = (info.m_format == cCRNFmtDXT1) ?
            DDS_GetCompressedSizeBC1(info.m_width, info.m_height, info.m_levels) :
            DDS_GetCompressedSizeBC7(info.m_width, info.m_height, info.m_levels);

        mData.resize(ddsSize);

        uint8_t* dstPtr = mData.data();

        size_t size = info.m_width;
        for (size_t mip = 0; mip < info.m_levels; ++mip) {
            const size_t numBlocks = (size / 4);
            const size_t pitch = numBlocks * blockSize;
            const size_t mipSize = numBlocks * pitch;

            if (!crnd::crnd_unpack_level(ctx, rcast<void**>(&dstPtr), scast<uint32_t>(mipSize), scast<uint32_t>(pitch), scast<uint32_t>(mip))) {
                crnd::crnd_unpack_end(ctx);
                result = false;
                break;
            }

            dstPtr += mipSize;

            size >>= 1;
            if (size < 4) {
                size = 4;
            }
        }

        if (result) {
            crnd::crnd_unpack_end(ctx);

            if (mFormat == PixelFormat::Invalid) {
                mFormat = (info.m_format == cCRNFmtDXT1) ? PixelFormat::BC1 : PixelFormat::BC3;
            }
        }
    } else {
        result = false;
    }

    return result;
}
