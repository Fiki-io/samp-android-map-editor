#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

namespace samp_editor {

enum RwChunkType : uint32_t {
    rwID_NAOBJECT        = 0x0000,
    rwID_STRUCT          = 0x0001,
    rwID_STRING          = 0x0002,
    rwID_EXTENSION       = 0x0003,
    rwID_TEXTURE         = 0x0006,
    rwID_MATERIAL        = 0x0007,
    rwID_MATLIST         = 0x0008,
    rwID_FRAMELIST       = 0x000E,
    rwID_GEOMETRY        = 0x000F,
    rwID_CLUMP           = 0x0010,
    rwID_LIGHT           = 0x0012,
    rwID_ATOMIC          = 0x0014,
    rwID_TEXTURENATIVE   = 0x0015,
    rwID_TEXDICTIONARY   = 0x0016,
    rwID_GEOMETRYLIST    = 0x001A,
    rwID_ANIMANIMATION   = 0x001B,
    rwID_RIGHTTORENDER   = 0x001F,
    rwID_COLLISION       = 0x002C,
    rwID_BINMESH         = 0x050E,
    rwID_2DFX            = 0x0253F2F8
};

#pragma pack(push, 1)
struct RwChunkHeader {
    uint32_t type;
    uint32_t size;
    uint32_t version;
};
#pragma pack(pop)

class RwStreamReader {
public:
    RwStreamReader(const uint8_t* data, size_t size)
        : m_Data(data), m_Size(size), m_Offset(0) {}

    bool ReadHeader(RwChunkHeader& header) {
        if (m_Offset + sizeof(RwChunkHeader) > m_Size) {
            return false;
        }
        std::memcpy(&header, m_Data + m_Offset, sizeof(RwChunkHeader));
        m_Offset += sizeof(RwChunkHeader);
        return true;
    }

    bool Skip(size_t bytes) {
        if (m_Offset + bytes > m_Size) return false;
        m_Offset += bytes;
        return true;
    }

    template<typename T>
    bool Read(T& outVal) {
        if (m_Offset + sizeof(T) > m_Size) return false;
        std::memcpy(&outVal, m_Data + m_Offset, sizeof(T));
        m_Offset += sizeof(T);
        return true;
    }

    bool ReadBytes(void* dest, size_t bytes) {
        if (m_Offset + bytes > m_Size) return false;
        std::memcpy(dest, m_Data + m_Offset, bytes);
        m_Offset += bytes;
        return true;
    }

    std::string ReadString(size_t length) {
        if (m_Offset + length > m_Size) return "";
        std::string s(reinterpret_cast<const char*>(m_Data + m_Offset), length);
        m_Offset += length;
        // Trim null terminator if any
        size_t nullPos = s.find('\0');
        if (nullPos != std::string::npos) {
            s.resize(nullPos);
        }
        return s;
    }

    const uint8_t* CurrentPtr() const { return m_Data + m_Offset; }
    size_t GetOffset() const { return m_Offset; }
    size_t GetRemaining() const { return m_Size > m_Offset ? m_Size - m_Offset : 0; }
    void SetOffset(size_t off) { m_Offset = std::min(off, m_Size); }

private:
    const uint8_t* m_Data;
    size_t m_Size;
    size_t m_Offset;
};

} // namespace samp_editor
