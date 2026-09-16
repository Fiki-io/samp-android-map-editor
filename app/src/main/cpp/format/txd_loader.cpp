#include "txd_loader.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace samp_editor {

namespace {
    inline void UnpackRGB565(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
        r = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);
        g = static_cast<uint8_t>(((c >> 5) & 0x3F) * 255 / 63);
        b = static_cast<uint8_t>((c & 0x1F) * 255 / 31);
    }

    std::string ToLower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return str;
    }
}

bool TXDLoader::LoadFromMemory(const uint8_t* data, size_t size, std::unordered_map<std::string, DecodedTexture>& outTextures) {
    RwStreamReader reader(data, size);
    RwChunkHeader header{};

    while (reader.ReadHeader(header)) {
        size_t nextChunk = reader.GetOffset() + header.size;
        if (header.type == rwID_TEXDICTIONARY) {
            size_t dictEnd = reader.GetOffset() + header.size;

            RwChunkHeader structHdr{};
            if (!reader.ReadHeader(structHdr) || structHdr.type != rwID_STRUCT) return false;
            reader.Skip(structHdr.size);

            while (reader.GetOffset() < dictEnd) {
                RwChunkHeader texHdr{};
                if (!reader.ReadHeader(texHdr)) break;
                size_t nextTex = reader.GetOffset() + texHdr.size;

                if (texHdr.type == rwID_TEXTURENATIVE) {
                    DecodedTexture tex;
                    if (ParseTextureNative(reader, texHdr.size, tex)) {
                        outTextures[ToLower(tex.name)] = std::move(tex);
                    }
                }
                reader.SetOffset(nextTex);
            }
        }
        reader.SetOffset(nextChunk);
    }

    return !outTextures.empty();
}

bool TXDLoader::ParseTextureNative(RwStreamReader& reader, size_t nativeSize, DecodedTexture& outTex) {
    size_t endOffset = reader.GetOffset() + nativeSize;

    RwChunkHeader structHdr{};
    if (!reader.ReadHeader(structHdr) || structHdr.type != rwID_STRUCT) return false;

    uint32_t platformId = 0;
    reader.Read(platformId);

    uint8_t filterFlags, wrapV, wrapU, pad;
    reader.Read(filterFlags);
    reader.Read(wrapV);
    reader.Read(wrapU);
    reader.Read(pad);

    char diffuseName[32] = {0};
    char alphaName[32] = {0};
    reader.ReadBytes(diffuseName, 32);
    reader.ReadBytes(alphaName, 32);

    outTex.name = std::string(diffuseName);

    uint32_t rasterFormat = 0;
    uint32_t d3dFormat = 0; // 'DXT1' = 0x31545844, 'DXT5' = 0x35545844, dll
    uint16_t width = 0, height = 0;
    uint8_t depth = 0, numMipLevels = 0, rasterType = 0, compressionFlags = 0;

    reader.Read(rasterFormat);
    reader.Read(d3dFormat);
    reader.Read(width);
    reader.Read(height);
    reader.Read(depth);
    reader.Read(numMipLevels);
    reader.Read(rasterType);
    reader.Read(compressionFlags);

    outTex.width = width;
    outTex.height = height;

    if (numMipLevels == 0 || width == 0 || height == 0) return false;

    // Baca Mip Level 0
    uint32_t dataSize = 0;
    if (!reader.Read(dataSize)) return false;

    const uint8_t* rawData = reader.CurrentPtr();
    if (reader.GetRemaining() < dataSize) return false;

    // Dekompresi berdasarkan format D3D
    if (d3dFormat == 0x31545844) { // DXT1
        DecompressDXT1(rawData, width, height, outTex.rgbaPixels);
    } else if (d3dFormat == 0x35545844) { // DXT5
        DecompressDXT5(rawData, width, height, outTex.rgbaPixels);
    } else if (depth == 32) { // 32-bit uncompressed (BGRA di PC)
        ConvertBGRAtoRGBA(rawData, width, height, outTex.rgbaPixels);
    } else {
        // Fallback default: buffer kosong dengan dimensi valid
        outTex.rgbaPixels.assign(width * height * 4, 255);
    }

    reader.SetOffset(endOffset);
    return !outTex.rgbaPixels.empty();
}

void TXDLoader::DecompressDXT1(const uint8_t* src, uint32_t width, uint32_t height, std::vector<uint8_t>& dst) {
    dst.resize(width * height * 4);
    uint32_t blocksX = (width + 3) / 4;
    uint32_t blocksY = (height + 3) / 4;

    size_t srcOffset = 0;

    for (uint32_t by = 0; by < blocksY; ++by) {
        for (uint32_t bx = 0; bx < blocksX; ++bx) {
            uint16_t c0 = *reinterpret_cast<const uint16_t*>(src + srcOffset);
            uint16_t c1 = *reinterpret_cast<const uint16_t*>(src + srcOffset + 2);
            uint32_t code = *reinterpret_cast<const uint32_t*>(src + srcOffset + 4);
            srcOffset += 8;

            uint8_t r[4], g[4], b[4], a[4];
            UnpackRGB565(c0, r[0], g[0], b[0]); a[0] = 255;
            UnpackRGB565(c1, r[1], g[1], b[1]); a[1] = 255;

            if (c0 > c1) {
                r[2] = (2 * r[0] + r[1]) / 3;
                g[2] = (2 * g[0] + g[1]) / 3;
                b[2] = (2 * b[0] + b[1]) / 3;
                a[2] = 255;

                r[3] = (r[0] + 2 * r[1]) / 3;
                g[3] = (g[0] + 2 * g[1]) / 3;
                b[3] = (b[0] + 2 * b[1]) / 3;
                a[3] = 255;
            } else {
                r[2] = (r[0] + r[1]) / 2;
                g[2] = (g[0] + g[1]) / 2;
                b[2] = (b[0] + b[1]) / 2;
                a[2] = 255;

                r[3] = 0; g[3] = 0; b[3] = 0; a[3] = 0;
            }

            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    uint32_t x = bx * 4 + px;
                    uint32_t y = by * 4 + py;
                    if (x < width && y < height) {
                        int shift = 2 * (py * 4 + px);
                        int idx = (code >> shift) & 0x03;
                        size_t dstIdx = (y * width + x) * 4;
                        dst[dstIdx + 0] = r[idx];
                        dst[dstIdx + 1] = g[idx];
                        dst[dstIdx + 2] = b[idx];
                        dst[dstIdx + 3] = a[idx];
                    }
                }
            }
        }
    }
}

void TXDLoader::DecompressDXT5(const uint8_t* src, uint32_t width, uint32_t height, std::vector<uint8_t>& dst) {
    dst.resize(width * height * 4);
    uint32_t blocksX = (width + 3) / 4;
    uint32_t blocksY = (height + 3) / 4;

    size_t srcOffset = 0;

    for (uint32_t by = 0; by < blocksY; ++by) {
        for (uint32_t bx = 0; bx < blocksX; ++bx) {
            uint8_t a0 = src[srcOffset];
            uint8_t a1 = src[srcOffset + 1];

            uint64_t alphaBits = 0;
            std::memcpy(&alphaBits, src + srcOffset + 2, 6);
            srcOffset += 8;

            uint8_t alphas[8];
            alphas[0] = a0;
            alphas[1] = a1;
            if (a0 > a1) {
                alphas[2] = (6 * a0 + 1 * a1) / 7;
                alphas[3] = (5 * a0 + 2 * a1) / 7;
                alphas[4] = (4 * a0 + 3 * a1) / 7;
                alphas[5] = (3 * a0 + 4 * a1) / 7;
                alphas[6] = (2 * a0 + 5 * a1) / 7;
                alphas[7] = (1 * a0 + 6 * a1) / 7;
            } else {
                alphas[2] = (4 * a0 + 1 * a1) / 5;
                alphas[3] = (3 * a0 + 2 * a1) / 5;
                alphas[4] = (2 * a0 + 3 * a1) / 5;
                alphas[5] = (1 * a0 + 4 * a1) / 5;
                alphas[6] = 0;
                alphas[7] = 255;
            }

            uint16_t c0 = *reinterpret_cast<const uint16_t*>(src + srcOffset);
            uint16_t c1 = *reinterpret_cast<const uint16_t*>(src + srcOffset + 2);
            uint32_t code = *reinterpret_cast<const uint32_t*>(src + srcOffset + 4);
            srcOffset += 8;

            uint8_t r[4], g[4], b[4];
            UnpackRGB565(c0, r[0], g[0], b[0]);
            UnpackRGB565(c1, r[1], g[1], b[1]);
            r[2] = (2 * r[0] + r[1]) / 3;
            g[2] = (2 * g[0] + g[1]) / 3;
            b[2] = (2 * b[0] + b[1]) / 3;
            r[3] = (r[0] + 2 * r[1]) / 3;
            g[3] = (g[0] + 2 * g[1]) / 3;
            b[3] = (b[0] + 2 * b[1]) / 3;

            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    uint32_t x = bx * 4 + px;
                    uint32_t y = by * 4 + py;
                    if (x < width && y < height) {
                        int pixelIdx = py * 4 + px;
                        int colShift = 2 * pixelIdx;
                        int colIdx = (code >> colShift) & 0x03;

                        int alphaShift = 3 * pixelIdx;
                        int aIdx = (alphaBits >> alphaShift) & 0x07;

                        size_t dstIdx = (y * width + x) * 4;
                        dst[dstIdx + 0] = r[colIdx];
                        dst[dstIdx + 1] = g[colIdx];
                        dst[dstIdx + 2] = b[colIdx];
                        dst[dstIdx + 3] = alphas[aIdx];
                    }
                }
            }
        }
    }
}

void TXDLoader::ConvertBGRAtoRGBA(const uint8_t* src, uint32_t width, uint32_t height, std::vector<uint8_t>& dst) {
    size_t pixelCount = width * height;
    dst.resize(pixelCount * 4);
    for (size_t i = 0; i < pixelCount; ++i) {
        dst[i * 4 + 0] = src[i * 4 + 2]; // R <- B
        dst[i * 4 + 1] = src[i * 4 + 1]; // G <- G
        dst[i * 4 + 2] = src[i * 4 + 0]; // B <- R
        dst[i * 4 + 3] = src[i * 4 + 3]; // A <- A
    }
}

} // namespace samp_editor
