#include "img_archive.h"
#include <unistd.h>
#include <iostream>

namespace samp_editor {

#pragma pack(push, 1)
struct RawIMGv2Header {
    char magic[4];       // "VER2"
    uint32_t entryCount;
};

struct RawIMGv2Entry {
    uint32_t offsetSector;
    uint16_t sizeSectors;
    uint16_t streamingSize;
    char name[24];
};
#pragma pack(pop)

IMGArchive::~IMGArchive() {
    Close();
}

void IMGArchive::Close() {
    if (m_FileStream.is_open()) {
        m_FileStream.close();
    }
    if (m_Fd >= 0) {
        close(m_Fd);
        m_Fd = -1;
    }
    m_Entries.clear();
    m_FilePath.clear();
    m_FileSize = 0;
}

bool IMGArchive::Open(const std::string& filePath) {
    Close();
    m_FilePath = filePath;
    m_FileStream.open(filePath, std::ios::binary);
    if (!m_FileStream.is_open()) {
        std::cerr << "[IMGArchive] Failed to open file: " << filePath << std::endl;
        return false;
    }

    m_FileStream.seekg(0, std::ios::end);
    m_FileSize = m_FileStream.tellg();
    m_FileStream.seekg(0, std::ios::beg);

    RawIMGv2Header header{};
    m_FileStream.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (m_FileStream.gcount() != sizeof(header)) {
        Close();
        return false;
    }

    if (std::memcmp(header.magic, "VER2", 4) != 0) {
        std::cerr << "[IMGArchive] Invalid magic, expected VER2: " << filePath << std::endl;
        Close();
        return false;
    }

    return ReadDirectoryTable(header.entryCount);
}

bool IMGArchive::OpenFromFd(int fd, uint64_t fileLength) {
    Close();
    m_Fd = fd;
    m_FileSize = fileLength;

    RawIMGv2Header header{};
    ssize_t bytesRead = pread(m_Fd, &header, sizeof(header), 0);
    if (bytesRead != sizeof(header)) {
        Close();
        return false;
    }

    if (std::memcmp(header.magic, "VER2", 4) != 0) {
        std::cerr << "[IMGArchive] Invalid magic on fd: " << fd << std::endl;
        Close();
        return false;
    }

    return ReadDirectoryTable(header.entryCount);
}

bool IMGArchive::ReadDirectoryTable(uint32_t entryCount) {
    m_Entries.clear();
    m_Entries.reserve(entryCount);

    std::vector<RawIMGv2Entry> rawEntries(entryCount);
    size_t tableSize = static_cast<size_t>(entryCount) * sizeof(RawIMGv2Entry);

    if (m_FileStream.is_open()) {
        m_FileStream.read(reinterpret_cast<char*>(rawEntries.data()), tableSize);
        if (static_cast<size_t>(m_FileStream.gcount()) != tableSize) {
            std::cerr << "[IMGArchive] Failed to read full directory table" << std::endl;
            return false;
        }
    } else if (m_Fd >= 0) {
        ssize_t readBytes = pread(m_Fd, rawEntries.data(), tableSize, sizeof(RawIMGv2Header));
        if (static_cast<size_t>(readBytes) != tableSize) {
            std::cerr << "[IMGArchive] Failed to read full directory table from fd" << std::endl;
            return false;
        }
    } else {
        return false;
    }

    for (uint32_t i = 0; i < entryCount; ++i) {
        const auto& raw = rawEntries[i];
        char cleanName[25];
        std::memset(cleanName, 0, sizeof(cleanName));
        std::memcpy(cleanName, raw.name, 24);

        std::string nameStr(cleanName);
        std::string lowerKey = ToLower(nameStr);

        IMGEntry entry;
        entry.offsetSector = raw.offsetSector;
        entry.sizeSectors = raw.sizeSectors;
        entry.streamingSize = raw.streamingSize;
        entry.name = nameStr;

        m_Entries[lowerKey] = entry;
    }

    return true;
}

bool IMGArchive::HasEntry(const std::string& name) const {
    return m_Entries.find(ToLower(name)) != m_Entries.end();
}

const IMGEntry* IMGArchive::FindEntry(const std::string& name) const {
    auto it = m_Entries.find(ToLower(name));
    if (it != m_Entries.end()) {
        return &it->second;
    }
    return nullptr;
}

bool IMGArchive::ReadEntry(const std::string& name, std::vector<uint8_t>& outBuffer) {
    const IMGEntry* entry = FindEntry(name);
    if (!entry) return false;
    return ReadEntry(*entry, outBuffer);
}

bool IMGArchive::ReadEntry(const IMGEntry& entry, std::vector<uint8_t>& outBuffer) {
    uint64_t offsetBytes = entry.GetByteOffset();
    uint64_t sizeBytes = entry.GetByteSize();

    if (sizeBytes == 0) return false;

    outBuffer.resize(sizeBytes);

    if (m_FileStream.is_open()) {
        m_FileStream.seekg(offsetBytes, std::ios::beg);
        m_FileStream.read(reinterpret_cast<char*>(outBuffer.data()), sizeBytes);
        return static_cast<uint64_t>(m_FileStream.gcount()) == sizeBytes;
    } else if (m_Fd >= 0) {
        ssize_t bytes = pread(m_Fd, outBuffer.data(), sizeBytes, offsetBytes);
        return static_cast<uint64_t>(bytes) == sizeBytes;
    }

    return false;
}

} // namespace samp_editor
