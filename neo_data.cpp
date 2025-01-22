
/*
 * data to play out on neopixels
 * built-in patterns
 */
#include "neo_data.h"


// adjust MAX_SEQUENCES in neo_data.h to match number initialized
neo_data_t neo_sequences[MAX_SEQUENCES] = {
  { "RED-MED",
    {
    { 0,   0, 0, 0, 50 },
    { 8,   0, 0, 0, 50 },
    { 16,  0, 0, 0, 50 },
    { 24,  0, 0, 0, 50 },
    { 32,  0, 0, 0, 50 },
    { 40,  0, 0, 0, 50 },
    { 48,  0, 0, 0, 50 },
    { 56,  0, 0, 0, 50 },
    { 64,  0, 0, 0, 50 },
    { 72,  0, 0, 0, 50 },
    { 80,  0, 0, 0, 50 },
    { 88,  0, 0, 0, 50 },
    { 96,  0, 0, 0, 50 },
    { 104, 0, 0, 0, 50 },
    { 112, 0, 0, 0, 50 },
    { 120, 0, 0, 0, 50 },
    { 128, 0, 0, 0, 50 },
    { 128, 0, 0, 0, 50 },
    { 120, 0, 0, 0, 50 },
    { 112, 0, 0, 0, 50 },
    { 104, 0, 0, 0, 50 },
    { 96,  0, 0, 0, 50 },
    { 88,  0, 0, 0, 50 },
    { 80,  0, 0, 0, 50 },
    { 72,  0, 0, 0, 50 },
    { 64,  0, 0, 0, 50 },
    { 56,  0, 0, 0, 50 },
    { 48,  0, 0, 0, 50 },
    { 40,  0, 0, 0, 50 },
    { 32,  0, 0, 0, 50 },
    { 24,  0, 0, 0, 50 },
    { 16,  0, 0, 0, 50 },
    { 8,   0, 0, 0, 50 },
    { 0,   0, 0, 0, 50 },
    { 0,   0, 0, 0, -1 },
    }
  }, // red-med
  { "GREEN-MED",
    {
    { 0, 0,   0, 0, 50 },
    { 0, 8,   0, 0, 50 },
    { 0, 16,  0, 0, 50 },
    { 0, 24,  0, 0, 50 },
    { 0, 32,  0, 0, 50 },
    { 0, 40,  0, 0, 50 },
    { 0, 48,  0, 0, 50 },
    { 0, 56,  0, 0, 50 },
    { 0, 64,  0, 0, 50 },
    { 0, 72,  0, 0, 50 },
    { 0, 80,  0, 0, 50 },
    { 0, 88,  0, 0, 50 },
    { 0, 96,  0, 0, 50 },
    { 0, 104, 0, 0, 50 },
    { 0, 112, 0, 0, 50 },
    { 0, 120, 0, 0, 50 },
    { 0, 128, 0, 0, 50 },
    { 0, 128, 0, 0, 50 },
    { 0, 120, 0, 0, 50 },
    { 0, 112, 0, 0, 50 },
    { 0, 104, 0, 0, 50 },
    { 0, 96,  0, 0, 50 },
    { 0, 88,  0, 0, 50 },
    { 0, 80,  0, 0, 50 },
    { 0, 72,  0, 0, 50 },
    { 0, 64,  0, 0, 50 },
    { 0, 56,  0, 0, 50 },
    { 0, 48,  0, 0, 50 },
    { 0, 40,  0, 0, 50 },
    { 0, 32,  0, 0, 50 },
    { 0, 24,  0, 0, 50 },
    { 0, 16,  0, 0, 50 },
    { 0, 8,   0, 0, 50 },
    { 0, 0,   0, 0, 50 },
    { 0, 0,   0, 0, -1 },
    }
  }, // green-med
  { "PURPLE-SLOW",
    {
    { 0,   0, 0,   0, 50 },
    { 4,   0, 4,   0, 50 },
    { 8,   0, 8,   0, 50 },
    { 12,  0, 12,  0, 50 },
    { 16,  0, 16,  0, 50 },
    { 20,  0, 20,  0, 50 },
    { 24,  0, 24,  0, 50 },
    { 32,  0, 32,  0, 50 },
    { 40,  0, 40,  0, 50 },
    { 44,  0, 44,  0, 50 },
    { 48,  0, 48,  0, 50 },
    { 52,  0, 52,  0, 50 },
    { 56,  0, 56,  0, 50 },
    { 60,  0, 60,  0, 50 },
    { 64,  0, 64,  0, 50 },
    { 68,  0, 68,  0, 50 },
    { 72,  0, 72,  0, 50 },
    { 76,  0, 76,  0, 50 },
    { 80,  0, 80,  0, 50 },
    { 84,  0, 84,  0, 50 },
    { 88,  0, 88,  0, 50 },
    { 92,  0, 92,  0, 50 },
    { 96,  0, 96,  0, 50 },
    { 100, 0, 100, 0, 50 },
    { 104, 0, 104, 0, 50 },
    { 108, 0, 108, 0, 50 },
    { 112, 0, 112, 0, 50 },
    { 116, 0, 116, 0, 50 },
    { 120, 0, 120, 0, 50 },
    { 124, 0, 124, 0, 50 },
    { 128, 0, 128, 0, 50 },
    { 128, 0, 128, 0, 50 },
    { 124, 0, 124, 0, 50 },
    { 120, 0, 120, 0, 50 },
    { 116, 0, 116, 0, 50 },
    { 112, 0, 112, 0, 50 },
    { 108, 0, 108, 0, 50 },
    { 104, 0, 104, 0, 50 },
    { 100, 0, 100, 0, 50 },
    { 96,  0, 96,  0, 50 },
    { 92,  0, 92,  0, 50 },
    { 88,  0, 88,  0, 50 },
    { 84,  0, 84,  0, 50 },
    { 80,  0, 80,  0, 50 },
    { 76,  0, 76,  0, 50 },
    { 72,  0, 72,  0, 50 },
    { 68,  0, 68,  0, 50 },
    { 64,  0, 64,  0, 50 },
    { 60,  0, 60,  0, 50 },
    { 56,  0, 56,  0, 50 },
    { 52,  0, 52,  0, 50 },
    { 48,  0, 48,  0, 50 },
    { 44,  0, 44,  0, 50 },
    { 40,  0, 40,  0, 50 },
    { 36,  0, 36,  0, 50 },
    { 32,  0, 32,  0, 50 },
    { 28,  0, 28,  0, 50 },
    { 24,  0, 24,  0, 50 },
    { 20,  0, 20,  0, 50 },
    { 16,  0, 16,  0, 50 },
    { 12,  0, 12,  0, 50 },
    { 8,   0, 8,   0, 50 },
    { 4,   0, 4,   0, 50 },
    { 0,   0, 0,   0, 50 },
    { 0,   0, 0,   0, -1},
    }
  }, //purple-slow
  { "USER-1",
    {
      { 0, 0, 0, 0, -1 },
    }
  }, // user-1
  { "USER-2",
    {
      { 0, 0, 0, 0, -1 },
    }
  }, // user-2
  { "USER-3",
    {
      { 0, 0, 0, 0, -1 },
    }
  }, // user-3
};