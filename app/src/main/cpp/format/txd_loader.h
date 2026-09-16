#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "../archive/rw_stream.h"

namespace samp_editor {

struct DecodedTexture {
    std::string name;
    uint32_t width{0};
    uint32_t height{0};
    std::vector<uint8_t> rgbaPixels; // 32-bit RGBA8888
};

class TXDLoader {
public:
    static bool LoadFromMemory(const uint8_t* data, size_t size, std::unordered_map<std::string, DecodedTexture>& outTextures);

    // Software decompressors
    static void DecompressDXT1(const uint8_t* src, uint32_t width, uint32_t height, std::vector<uint8_t>& dst);
    static void DecompressDXT5(const uint8_t* src, uint32_t width, uint32_t height, std::vector<uint8_t>& dst);
    static void ConvertBGRAtoRGBA(const uint8_t* src, uint32_t width, uint32_t height, std::vector<uint8_t>& dst);

private:
    static bool ParseTextureNative(RwStreamReader& reader, size_t nativeSize, DecodedTexture& outTex);
};

} // namespace samp_editor
