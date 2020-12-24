#pragma once
#include "types.h"

void DecodeImageFromTiles(uint8* src, uint32 src_size, uint32 dst_pitch_in_tiles, uint32* dst, bool is8x8, uint32* palette = 0);

void DecodeBackground(uint8* tiles, uint8* map, uint32* dst, bool is8x8, uint32* palette = 0);