#include "Arpeggiator.h"
#include "SynthEngine.h"

Arpeggiator arp;

void Arpeggiator::begin() {
    heldCount = 0;
    lastStepMs = 0;
    lastPlayedNote = 255;
}

void Arpeggiator::setMode(ArpMode m) {
    mode = m;
    if (m == ARP_OFF) allOff();
    stepIdx = 0; octIdx = 0; goingUp = true;
}

void Arpeggiator::setRateHz(float hz) {
    if (hz < 0.5f)  hz = 0.5f;
    if (hz > 16.0f) hz = 16.0f;
    rateHz = hz;
}

void Arpeggiator::setOctaves(uint8_t o) {
    if (o < 1) o = 1;
    if (o > 4) o = 4;
    octaves = o;
}

void Arpeggiator::noteOn(uint8_t note, uint8_t velocity) {
    if (mode == ARP_OFF) { synth.noteOn(note, velocity); return; }
    for (int i = 0; i < heldCount; i++) if (held[i] == note) return;
    if (heldCount < MAX_HELD) {
        int i = heldCount;
        while (i > 0 && held[i-1] > note) { held[i] = held[i-1]; heldVel[i] = heldVel[i-1]; i--; }
        held[i]    = note;
        heldVel[i] = velocity;
        heldCount++;
    }
}

void Arpeggiator::noteOff(uint8_t note) {
    if (mode == ARP_OFF) { synth.noteOff(note); return; }
    for (int i = 0; i < heldCount; i++) {
        if (held[i] == note) {
            for (int j = i; j < heldCount - 1; j++) {
                held[j] = held[j+1];
                heldVel[j] = heldVel[j+1];
            }
            heldCount--;
            break;
        }
    }
    if (heldCount == 0) allOff();
}

int Arpeggiator::advanceStep() {
    if (heldCount == 0) return -1;
    switch (mode) {
        case ARP_UP:
            stepIdx++;
            if (stepIdx >= heldCount) {
                stepIdx = 0;
                octIdx = (octIdx + 1) % octaves;
            }
            break;
        case ARP_DOWN:
            stepIdx--;
            if (stepIdx < 0) {
                stepIdx = heldCount - 1;
                octIdx = (octIdx + 1) % octaves;
            }
            break;
        case ARP_UPDN:
            if (heldCount == 1) { stepIdx = 0; break; }
            if (goingUp) {
                stepIdx++;
                if (stepIdx >= heldCount) {
                    stepIdx = heldCount - 2;
                    goingUp = false;
                }
            } else {
                stepIdx--;
                if (stepIdx < 0) {
                    stepIdx = 1;
                    goingUp = true;
                    octIdx = (octIdx + 1) % octaves;
                }
            }
            break;
        case ARP_RND:
            stepIdx = random(heldCount);
            octIdx  = random(octaves);
            break;
        default:
            return -1;
    }
    return stepIdx;
}

void Arpeggiator::tick(uint32_t nowMs) {
    if (mode == ARP_OFF) return;

    if (lastPlayedNote != 255 && nowMs >= noteOffDueMs) {
        synth.noteOff(lastPlayedNote);
        lastPlayedNote = 255;
    }

    if (heldCount == 0) return;

    uint32_t stepMs = (uint32_t)(1000.0f / rateHz);
    if (nowMs - lastStepMs >= stepMs) {
        lastStepMs = nowMs;
        int idx = advanceStep();
        if (idx >= 0) {
            int n = held[idx] + 12 * octIdx;
            if (n > 127) n = 127;
            synth.noteOn((uint8_t)n, heldVel[idx]);
            lastPlayedNote = (uint8_t)n;
            noteOffDueMs = nowMs + (uint32_t)(stepMs * gate);
        }
    }
}

void Arpeggiator::allOff() {
    if (lastPlayedNote != 255) {
        synth.noteOff(lastPlayedNote);
        lastPlayedNote = 255;
    }
    synth.allNotesOff();
    stepIdx = 0; octIdx = 0; goingUp = true;
}
