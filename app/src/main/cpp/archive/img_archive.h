#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <memory>
#include <cstring>
#include <algorithm>

namespace samp_editor {

struct IMGEntry {
    uint32_t offsetSector{0};
    uint16_t sizeSectors{0};
    uint16_t streamingSize{0};
    std::string name;

    uint64_t GetByteOffset() const { return static_cast<uint64_t>(offsetSector) * 2048ULL; }
    uint64_t GetByteSize() const { return static_cast<uint64_t>(sizeSectors) * 2048ULL; }
};

class IMGArchive {
public:
    IMGArchive() = default;
    ~IMGArchive();

    bool Open(const std::string& filePath);
    bool OpenFromFd(int fd, uint64_t fileLength = 0);
    void Close();

    bool IsOpen() const { return m_FileStream.is_open() || m_Fd >= 0; }
    size_t GetEntryCount() const { return m_Entries.size(); }

    bool HasEntry(const std::string& name) const;
    const IMGEntry* FindEntry(const std::string& name) const;
    bool ReadEntry(const std::string& name, std::vector<uint8_t>& outBuffer);
    bool ReadEntry(const IMGEntry& entry, std::vector<uint8_t>& outBuffer);

    const std::unordered_map<std::string, IMGEntry>& GetAllEntries() const { return m_Entries; }
    const std::string& GetPath() const { return m_FilePath; }

private:
    static std::string ToLower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return str;
    }

    bool ReadDirectoryTable(uint32_t entryCount);

    std::string m_FilePath;
    mutable std::ifstream m_FileStream;
    int m_Fd{-1};
    uint64_t m_FileSize{0};
    std::unordered_map<std::string, IMGEntry> m_Entries;
};

} // namespace samp_editor
