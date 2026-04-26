#include "PatchManager.h"
#include "Config.h"
#include <SD.h>
#include <SPI.h>

PatchManager patchManager;

static void slotFilename(int slot, char* buf, size_t n) {
    snprintf(buf, n, "/patches/p%03d.bin", slot);
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
    char fn[32]; slotFilename(slot, fn, sizeof(fn));
    if (SD.exists(fn)) SD.remove(fn);
    File f = SD.open(fn, FILE_WRITE);
    if (!f) return false;
    f.write((const uint8_t*)&p, sizeof(PatchData));
    f.close();
    return true;
}

bool PatchManager::loadPatch(int slot, PatchData& p) {
    if (slot < 0 || slot >= NUM_PATCH_SLOTS) return false;
    char fn[32]; slotFilename(slot, fn, sizeof(fn));
    if (!SD.exists(fn)) return false;
    File f = SD.open(fn, FILE_READ);
    if (!f) return false;
    size_t n = f.read((uint8_t*)&p, sizeof(PatchData));
    f.close();
    return (n == sizeof(PatchData));
}

bool PatchManager::getPatchName(int slot, char* nameOut) {
    PatchData tmp;
    if (!loadPatch(slot, tmp)) { nameOut[0] = 0; return false; }
    strncpy(nameOut, tmp.name, 16);
    nameOut[16] = 0;
    return true;
}
