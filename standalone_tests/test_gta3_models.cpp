#include <iostream>
#include <cassert>
#include "../app/src/main/cpp/archive/img_archive.h"
#include "../app/src/main/cpp/format/dff_loader.h"
#include "../app/src/main/cpp/format/ipl_parser.h"

using namespace samp_editor;

int main() {
    IMGArchive gta3;
    bool ok = gta3.Open("/home/gardenxxxxx/Downloads/project/extracted/GTA SAN ANDREAS FIXED/models/gta3.img");
    std::cout << "Open gta3.img: " << ok << ", entries: " << gta3.GetEntryCount() << std::endl;
    assert(ok);

    std::vector<MapInstance> instances;
    bool okIpl = IPLParser::ParseFile("/home/gardenxxxxx/Downloads/project/android-samp-map-editor/app/src/main/assets/data/maps/LA/LAe.ipl", instances);
    std::cout << "Parse LAe.ipl: " << okIpl << ", instances: " << instances.size() << std::endl;

    int successModels = 0;
    int failedModels = 0;
    int missingEntries = 0;

    for (size_t i = 0; i < std::min(instances.size(), size_t(20)); ++i) {
        std::string dffName = instances[i].modelName + ".dff";
        std::vector<uint8_t> buffer;
        if (!gta3.ReadEntry(dffName, buffer)) {
            std::cout << "Missing entry: " << dffName << std::endl;
            missingEntries++;
            continue;
        }

        DFFModel model;
        if (DFFLoader::LoadFromMemory(buffer.data(), buffer.size(), model, instances[i].modelName)) {
            std::cout << "SUCCESS model: " << instances[i].modelName 
                      << " geoms=" << model.geometries.size() 
                      << " vertices=" << (model.geometries.empty() ? 0 : model.geometries[0].vertices.size())
                      << " submeshes=" << (model.geometries.empty() ? 0 : model.geometries[0].subMeshes.size())
                      << std::endl;
            successModels++;
        } else {
            std::cout << "FAILED to parse DFF: " << instances[i].modelName << " (buffer size=" << buffer.size() << ")" << std::endl;
            failedModels++;
        }
    }

    std::cout << "Summary: success=" << successModels << ", failed=" << failedModels << ", missing=" << missingEntries << std::endl;
    return 0;
}
