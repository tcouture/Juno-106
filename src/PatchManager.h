#pragma once
#include "SynthEngine.h"

class PatchManager {
public:
    bool begin();
    bool savePatch(int slot, const PatchData& p);
    bool loadPatch(int slot, PatchData& p);

    // Returns true if slot has a patch; fills 'name' (<=16 chars + null).
    bool getPatchName(int slot, char* nameOut /*>=17*/);
};

extern PatchManager patchManager;
