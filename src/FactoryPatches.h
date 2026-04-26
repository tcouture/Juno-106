#pragma once

// Writes a set of starter patches to SD slots 0..N-1.
// Safe to call multiple times — it overwrites existing slots.
void installFactoryPatches();
