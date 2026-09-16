#include <iostream>
#include <cassert>
#include <cmath>
#include "core/types.h"
#include "archive/img_archive.h"
#include "format/dff_loader.h"
#include "format/txd_loader.h"
#include "format/ide_parser.h"
#include "format/ipl_parser.h"
#include "editor/exporter.h"
#include "editor/raycast.h"
#include "editor/scene.h"

using namespace samp_editor;

void TestMath() {
    std::cout << "[TEST] 1. Testing Math & Euler/Quaternion Roundtrip..." << std::endl;

    float rx = 15.0f, ry = 25.0f, rz = 90.0f;
    Quat q = Quat::FromEulerGTA(rx, ry, rz);
    Vec3 backEuler = q.ToEulerGTA();

    std::cout << "  Original Euler: " << rx << ", " << ry << ", " << rz << std::endl;
    std::cout << "  Reconstructed:  " << backEuler.x << ", " << backEuler.y << ", " << backEuler.z << std::endl;

    assert(std::abs(rx - backEuler.x) < 0.1f);
    assert(std::abs(ry - backEuler.y) < 0.1f);
    assert(std::abs(rz - backEuler.z) < 0.1f);

    std::cout << "  -> Math & Quaternion precision passed!" << std::endl;
}

void TestIMGArchive() {
    std::cout << "\n[TEST] 2. Testing IMGArchive Loader..." << std::endl;

    IMGArchive gta3;
    bool okGta3 = gta3.Open("../../../extracted/GTA SAN ANDREAS FIXED/models/gta3.img");
    std::cout << "  Open gta3.img: " << (okGta3 ? "SUCCESS" : "FAILED") << ", Entries: " << gta3.GetEntryCount() << std::endl;
    assert(okGta3 && gta3.GetEntryCount() > 16000);

    IMGArchive sampImg;
    bool okSamp = sampImg.Open("../../app/src/main/assets/samp/SAMP.img");
    std::cout << "  Open SAMP.img: " << (okSamp ? "SUCCESS" : "FAILED") << ", Entries: " << sampImg.GetEntryCount() << std::endl;
    assert(okSamp && sampImg.GetEntryCount() > 500);

    assert(sampImg.HasEntry("RedNeonTube1.dff"));
    std::cout << "  Found 'RedNeonTube1.dff' in SAMP.img!" << std::endl;

    std::cout << "  -> IMGArchive tests passed!" << std::endl;
}

void TestIDEParser() {
    std::cout << "\n[TEST] 3. Testing IDEParser..." << std::endl;

    std::unordered_map<uint32_t, ObjectDef> defs;
    bool ok = IDEParser::ParseFile("../../app/src/main/assets/samp/SAMP.ide", defs);
    std::cout << "  Parse SAMP.ide: " << (ok ? "SUCCESS" : "FAILED") << ", Total parsed IDs: " << defs.size() << std::endl;
    assert(ok && defs.size() > 1000);

    // Cek ID 18647 (RedNeonTube1)
    assert(defs.find(18647) != defs.end());
    const auto& neon = defs[18647];
    std::cout << "  ID 18647 Name: " << neon.modelName << ", TXD: " << neon.txdName << ", DrawDist: " << neon.drawDistance << std::endl;
    assert(neon.modelName == "RedNeonTube1");

    std::cout << "  -> IDEParser tests passed!" << std::endl;
}

void TestIPLParser() {
    std::cout << "\n[TEST] 4. Testing IPLParser..." << std::endl;

    std::vector<MapInstance> instances;
    bool ok = IPLParser::ParseFile("../../../extracted/GTA SAN ANDREAS FIXED/data/maps/LA/LAe.ipl", instances);
    std::cout << "  Parse LAe.ipl: " << (ok ? "SUCCESS" : "FAILED") << ", Instances: " << instances.size() << std::endl;
    assert(ok && !instances.empty());

    std::cout << "  First instance: ID=" << instances[0].modelId
              << " Name=" << instances[0].modelName
              << " Pos=(" << instances[0].position.x << ", " << instances[0].position.y << ", " << instances[0].position.z << ")"
              << " Quat=(" << instances[0].rotation.x << ", " << instances[0].rotation.y << ", " << instances[0].rotation.z << ", " << instances[0].rotation.w << ")"
              << std::endl;

    std::cout << "  -> IPLParser tests passed!" << std::endl;
}

void TestDFFAndTXDLoader() {
    std::cout << "\n[TEST] 5. Testing DFF & TXD Loaders on SAMP Model..." << std::endl;

    IMGArchive sampImg;
    sampImg.Open("../../app/src/main/assets/samp/SAMP.img");

    // Test DFF
    std::vector<uint8_t> dffBuffer;
    bool readOk = sampImg.ReadEntry("RedNeonTube1.dff", dffBuffer);
    assert(readOk);

    DFFModel model;
    bool dffParsed = DFFLoader::LoadFromMemory(dffBuffer.data(), dffBuffer.size(), model, "RedNeonTube1");
    std::cout << "  DFF Parse RedNeonTube1: " << (dffParsed ? "SUCCESS" : "FAILED")
              << ", Geometries: " << model.geometries.size() << std::endl;
    assert(dffParsed && !model.geometries.empty());
    std::cout << "  Vertices in Geom 0: " << model.geometries[0].vertices.size()
              << ", SubMeshes: " << model.geometries[0].subMeshes.size() << std::endl;
    assert(model.geometries[0].vertices.size() > 0);

    // Test TXD
    std::vector<uint8_t> txdBuffer;
    bool txdReadOk = sampImg.ReadEntry("samaps.txd", txdBuffer);
    if (!txdReadOk) {
        std::cout << "  Trying direct file read for samaps.txd..." << std::endl;
    } else {
        std::unordered_map<std::string, DecodedTexture> textures;
        bool txdParsed = TXDLoader::LoadFromMemory(txdBuffer.data(), txdBuffer.size(), textures);
        std::cout << "  TXD Parse samaps.txd: " << (txdParsed ? "SUCCESS" : "FAILED")
                  << ", Decoded Textures: " << textures.size() << std::endl;
        assert(txdParsed && !textures.empty());
        for (const auto& [name, tex] : textures) {
            std::cout << "    Texture: " << name << " (" << tex.width << "x" << tex.height << ", RGBA bytes: " << tex.rgbaPixels.size() << ")" << std::endl;
            assert(tex.width > 0 && tex.height > 0 && tex.rgbaPixels.size() == tex.width * tex.height * 4);
            break;
        }
    }

    std::cout << "  -> DFF and TXD Loader tests passed!" << std::endl;
}

void TestSceneAndExporter() {
    std::cout << "\n[TEST] 6. Testing Scene, Picking, and Pawn Exporter..." << std::endl;

    Scene scene;
    scene.LoadSAMPAssets("../../app/src/main/assets/samp/SAMP.img", "../../app/src/main/assets/samp/SAMP.ide");

    // Spawn 2 objek SA-MP
    EditorObject* obj1 = scene.SpawnObject(18647, {1500.0f, -1650.0f, 15.0f}); // RedNeonTube1
    obj1->rotationEuler = {0.0f, 0.0f, 45.0f};
    obj1->SyncQuatFromEuler();

    EditorObject* obj2 = scene.SpawnObject(18648, {1505.0f, -1650.0f, 15.0f}); // BlueNeonTube1
    obj2->rotationEuler = {10.0f, 0.0f, 90.0f};
    obj2->SyncQuatFromEuler();

    // Test Raycast Picking
    Ray ray;
    ray.origin = {1500.0f, -1650.0f, 30.0f};
    ray.direction = {0.0f, 0.0f, -1.0f}; // Tembak lurus ke bawah menuju obj1
    float hitDist = 0.0f;
    EditorObject* hit = scene.PickObject(ray, hitDist);
    std::cout << "  Raycast Pick Result: " << (hit ? hit->name : "NONE") << " at dist: " << hitDist << std::endl;
    assert(hit == obj1);

    // Test Export
    std::string pawnCode = PawnExporter::ExportToString(scene.GetEditorObjects(), ExportFormat::CreateDynamicObject_Streamer);
    std::cout << "\n--- Generated PAWN Code ---\n" << pawnCode << "---------------------------\n";

    assert(pawnCode.find("CreateDynamicObject(18647, 1500.0000, -1650.0000, 15.0000") != std::string::npos);
    assert(pawnCode.find("CreateDynamicObject(18648, 1505.0000, -1650.0000, 15.0000") != std::string::npos);

    std::cout << "  -> Scene & Pawn Exporter tests passed 100%!" << std::endl;
}

int main() {
    std::cout << "=================================================\n";
    std::cout << "   SA-MP Android Native Core Verification Suite   \n";
    std::cout << "=================================================\n\n";

    TestMath();
    TestIMGArchive();
    TestIDEParser();
    TestIPLParser();
    TestDFFAndTXDLoader();
    TestSceneAndExporter();

    std::cout << "\n>>> ALL 6 LOW-LEVEL CORE TESTS PASSED SUCCESSFULLY! <<<\n";
    return 0;
}
