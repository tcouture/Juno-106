#include <Arduino.h>
#include "Config.h"
#include "SynthEngine.h"
#include "MidiHandler.h"
#include "UI.h"
#include "PatchManager.h"
#include "Arpeggiator.h"
#include "FactoryPatches.h"

void setup() {
    Serial.begin(115200);
    delay(200);

    synth.begin();
    midiHandler.begin();
    arp.begin();

    if (!patchManager.begin()) {
        Serial.println("Patch storage unavailable; running without SD.");
    } else {
#if INSTALL_FACTORY_ON_BOOT
        installFactoryPatches();
#endif
        PatchData p;
        if (patchManager.loadPatch(0, p)) synth.applyPatch(p);
    }

    ui.begin();
    Serial.println("Juno-106 emulator ready.");
}

void loop() {
    midiHandler.update();
    synth.update();
    arp.tick(millis());
    ui.update();
}
