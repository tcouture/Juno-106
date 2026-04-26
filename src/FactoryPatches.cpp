#include "FactoryPatches.h"
#include "SynthEngine.h"
#include "PatchManager.h"
#include <Arduino.h>

static void makePatch(PatchData& p, const char* name) {
    PatchData init;      // defaults
    p = init;
    strncpy(p.name, name, 16);
    p.name[16] = 0;
}

void installFactoryPatches() {
    int slot = 0;
    PatchData p;

    // 0 - BRASS STAB
    makePatch(p, "BRASS STAB");
    p.sawLevel=0.8f; p.pulseLevel=0.6f; p.subLevel=0.2f; p.pulseWidth=0.5f;
    p.cutoff=2500; p.resonance=1.5f; p.envAmount=0.7f; p.hpfCutoff=20;
    p.ampA=5; p.ampD=120; p.ampS=0.6f; p.ampR=300;
    p.fltA=2; p.fltD=80;  p.fltS=0.3f; p.fltR=250;
    p.chorusMode=1; p.chorusRate=0.513f; p.chorusDepth=22.f;
    p.velDest=VEL_DEST_CUTOFF; p.velAmount=0.8f;
    patchManager.savePatch(slot++, p);

    // 1 - WARM PAD
    makePatch(p, "WARM PAD");
    p.sawLevel=0.9f; p.pulseLevel=0.4f; p.subLevel=0.3f;
    p.cutoff=900; p.resonance=0.9f; p.envAmount=0.3f;
    p.ampA=500; p.ampD=1500; p.ampS=0.8f; p.ampR=2000;
    p.fltA=800; p.fltD=1000; p.fltS=0.5f; p.fltR=1500;
    p.lfoRate=0.4f; p.lfoDepth=0.2f; p.lfoDest=LFO_DEST_FILTER; p.lfoShape=1;
    p.chorusMode=2; p.chorusRate=0.863f; p.chorusDepth=36.f;
    p.velAmount=0.4f;
    patchManager.savePatch(slot++, p);

    // 2 - PLUCKY BASS
    makePatch(p, "PLUCKY BASS");
    p.sawLevel=1.0f; p.pulseLevel=0.2f; p.subLevel=0.8f;
    p.cutoff=1200; p.resonance=1.8f; p.envAmount=0.9f; p.hpfCutoff=30;
    p.ampA=2; p.ampD=180; p.ampS=0.0f; p.ampR=120;
    p.fltA=2; p.fltD=250; p.fltS=0.0f; p.fltR=200;
    p.chorusMode=0;
    p.velDest=VEL_DEST_VCA; p.velAmount=0.8f;
    patchManager.savePatch(slot++, p);

    // 3 - SYNC LEAD
    makePatch(p, "SYNC LEAD");
    p.sawLevel=0.3f; p.pulseLevel=1.0f; p.subLevel=0.2f; p.pulseWidth=0.3f;
    p.cutoff=1800; p.resonance=2.5f; p.envAmount=0.6f;
    p.ampA=5; p.ampD=80;  p.ampS=0.7f; p.ampR=200;
    p.fltA=5; p.fltD=120; p.fltS=0.5f; p.fltR=300;
    p.lfoRate=5.0f; p.lfoDepth=0.4f; p.lfoDest=LFO_DEST_PITCH; p.lfoShape=0;
    p.chorusMode=1;
    p.velDest=VEL_DEST_CUTOFF; p.velAmount=0.7f;
    p.glideMs=20.0f;
    patchManager.savePatch(slot++, p);

    // 4 - STRINGS
    makePatch(p, "STRINGS");
    p.sawLevel=0.9f; p.pulseLevel=0.7f; p.subLevel=0.3f;
    p.cutoff=1500; p.resonance=1.0f; p.envAmount=0.4f; p.hpfCutoff=60;
    p.ampA=400; p.ampD=1200; p.ampS=0.8f; p.ampR=1800;
    p.fltA=600; p.fltD=900;  p.fltS=0.6f; p.fltR=1200;
    p.chorusMode=2;
    p.velAmount=0.5f;
    patchManager.savePatch(slot++, p);

    // 5 - ORGAN
    makePatch(p, "ORGAN");
    p.sawLevel=0.4f; p.pulseLevel=0.4f; p.subLevel=0.8f;
    p.cutoff=3000; p.resonance=0.8f; p.envAmount=0.2f;
    p.ampA=2; p.ampD=5; p.ampS=1.0f; p.ampR=100;
    p.fltA=2; p.fltD=5; p.fltS=1.0f; p.fltR=100;
    p.chorusMode=1;
    p.velDest=VEL_DEST_OFF; p.velAmount=0.0f;
    patchManager.savePatch(slot++, p);

    // 6 - FAT SAWS
    makePatch(p, "FAT SAWS");
    p.sawLevel=1.0f; p.pulseLevel=0.3f; p.subLevel=0.5f;
    p.cutoff=2200; p.resonance=1.3f; p.envAmount=0.5f;
    p.ampA=8; p.ampD=200; p.ampS=0.7f; p.ampR=500;
    p.fltA=5; p.fltD=300; p.fltS=0.3f; p.fltR=600;
    p.lfoRate=0.2f; p.lfoDepth=0.1f; p.lfoDest=LFO_DEST_PITCH; p.lfoShape=1;
    p.chorusMode=2;
    p.velDest=VEL_DEST_CUTOFF; p.velAmount=0.6f;
    patchManager.savePatch(slot++, p);

    // 7 - BELL TINES
    makePatch(p, "BELL TINES");
    p.sawLevel=0.2f; p.pulseLevel=0.8f; p.subLevel=0.4f; p.pulseWidth=0.2f;
    p.cutoff=2800; p.resonance=2.0f; p.envAmount=0.8f; p.hpfCutoff=40;
    p.ampA=2; p.ampD=400; p.ampS=0.0f; p.ampR=800;
    p.fltA=2; p.fltD=600; p.fltS=0.0f; p.fltR=1000;
    p.chorusMode=1;
    p.velAmount=0.9f;
    patchManager.savePatch(slot++, p);

    // 8 - PERCUSSIVE
    makePatch(p, "PERCUSSIVE");
    p.sawLevel=0.7f; p.pulseLevel=0.5f; p.subLevel=0.6f;
    p.cutoff=1800; p.resonance=2.2f; p.envAmount=1.0f; p.hpfCutoff=100;
    p.ampA=2; p.ampD=80; p.ampS=0.0f; p.ampR=40;
    p.fltA=2; p.fltD=120; p.fltS=0.0f; p.fltR=60;
    p.chorusMode=0;
    p.velDest=VEL_DEST_CUTOFF; p.velAmount=1.0f;
    patchManager.savePatch(slot++, p);

    // 9 - DREAMY LFO
    makePatch(p, "DREAMY LFO");
    p.sawLevel=0.8f; p.pulseLevel=0.6f; p.subLevel=0.3f;
    p.cutoff=1100; p.resonance=1.2f; p.envAmount=0.3f;
    p.ampA=800; p.ampD=2000; p.ampS=0.7f; p.ampR=2500;
    p.fltA=1000; p.fltD=1500; p.fltS=0.5f; p.fltR=2000;
    p.lfoRate=0.3f; p.lfoDepth=0.3f; p.lfoDest=LFO_DEST_FILTER; p.lfoShape=1;
    p.chorusMode=2;
    p.velAmount=0.3f;
    p.glideMs=80.0f;
    patchManager.savePatch(slot++, p);

    Serial.print("Installed ");
    Serial.print(slot);
    Serial.println(" factory patches.");
}
