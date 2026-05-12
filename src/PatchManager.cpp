#include "PatchManager.h"
#include "Config.h"
#include <SD.h>
#include <SPI.h>

PatchManager patchManager;

static void slotFilename(int slot, char* buf, size_t n) {
    snprintf(buf, n, "/patches/p%03d.bin", slot);
}

// ---------- New on-disk header ----------
struct PatchFileHeader {
    uint32_t magic;     // 'PTCH'
    uint16_t version;   // 1
    uint16_t dataSize;  // sizeof(PatchData) when written
    uint32_t crc32;     // CRC of payload bytes only
};

static constexpr uint32_t PATCH_MAGIC   = 0x50544348UL; // "PTCH"
static constexpr uint16_t PATCH_VERSION = 1;

// CRC-32 (poly 0xEDB88320), small/simple implementation
static uint32_t crc32_compute(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            uint32_t mask = (uint32_t)-(int)(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

bool PatchManager::begin() {
    SPI.setMOSI(SDCARD_MOSI_PIN);
    SPI.setSCK(SDCARD_SCK_PIN);
    if (!SD.begin(SDCARD_CS_PIN)) {
        Serial.println("SD (audio shield) init failed");
        return false;
    }
    if (!SD.exists("/patches")) SD.mkdir("/patches");
    return true;
}

bool PatchManager::savePatch(int slot, const PatchData& p) {
    if (slot < 0 || slot >= NUM_PATCH_SLOTS) return false;

    char fn[32];
    slotFilename(slot, fn, sizeof(fn));

    if (SD.exists(fn)) SD.remove(fn);
    File f = SD.open(fn, FILE_WRITE);
    if (!f) return false;

    PatchFileHeader h;
    h.magic    = PATCH_MAGIC;
    h.version  = PATCH_VERSION;
    h.dataSize = (uint16_t)sizeof(PatchData);
    h.crc32    = crc32_compute((const uint8_t*)&p, sizeof(PatchData));

    size_t w1 = f.write((const uint8_t*)&h, sizeof(h));
    size_t w2 = f.write((const uint8_t*)&p, sizeof(PatchData));
    f.close();

    return (w1 == sizeof(h) && w2 == sizeof(PatchData));
}

bool PatchManager::loadPatch(int slot, PatchData& p) {
    if (slot < 0 || slot >= NUM_PATCH_SLOTS) return false;

    char fn[32];
    slotFilename(slot, fn, sizeof(fn));

    if (!SD.exists(fn)) return false;
    File f = SD.open(fn, FILE_READ);
    if (!f) return false;

    size_t fileSize = f.size();
    bool ok = false;

    // Try new format first: [header][payload]
    if (fileSize >= sizeof(PatchFileHeader) + sizeof(PatchData)) {
        PatchFileHeader h;
        size_t nH = f.read((uint8_t*)&h, sizeof(h));
        if (nH == sizeof(h) &&
            h.magic == PATCH_MAGIC &&
            h.version == PATCH_VERSION &&
            h.dataSize == sizeof(PatchData)) {

            PatchData tmp;
            size_t nP = f.read((uint8_t*)&tmp, sizeof(PatchData));
            if (nP == sizeof(PatchData)) {
                uint32_t crc = crc32_compute((const uint8_t*)&tmp, sizeof(PatchData));
                if (crc == h.crc32) {
                    p = tmp;
                    ok = true;
                }
            }
        }

        if (!ok) {
            // If header path failed, fall through and try legacy raw format
            f.seek(0);
        }
    }

    // Legacy format fallback: raw PatchData only
    if (!ok && fileSize == sizeof(PatchData)) {
        PatchData tmp;
        size_t n = f.read((uint8_t*)&tmp, sizeof(PatchData));
        if (n == sizeof(PatchData)) {
            p = tmp;
            ok = true;
        }
    }

    f.close();
    return ok;
}

bool PatchManager::getPatchName(int slot, char* nameOut) {
    PatchData tmp;
    if (!loadPatch(slot, tmp)) {
        nameOut[0] = 0;
        return false;
    }
    strncpy(nameOut, tmp.name, 16);
    nameOut[16] = 0;
    return true;
}
