#include "graphicsutil.h"

// Decodes 8 pixels
void DecodeLine(uint8* data, uint32* dst, uint32* palette)
{
	uint8 low = data[0];
	uint8 high = data[1];
	for (uint8 i = 0; i < 8; i++)
	{
		uint8 bit = 7 - i;
		uint8 idx = ((low >> bit) & 0x1) | (((high >> bit) & 0x1) << 1);
		dst[i] = palette[idx];
	}
}

void DecodeTile(uint8* src, uint32 tile_height, uint32* dst, uint32* palette)
{
	for (uint32 line = 0; line < tile_height; line++)
	{
		DecodeLine(&src[line * 2], &dst[line * 8], palette); // 2 bytes per line
	}
}

void CopyTileToDestination(uint32* tile, uint32 tile_height, uint32* dst, uint32 x_dst, uint32 y_dst, uint32 width_dst)
{
	for (uint32 x = 0; x < tile_height; x++)
	{
		for (uint32 y = 0; y < 8; y++)
		{
			uint32 idx_tile = x * 8 + y;
			uint32 idx_dst = (x_dst + x) * width_dst + y_dst + y;
			dst[idx_dst] = tile[idx_tile];
		}
	}
}

void DecodeImageFromTiles(uint8* src, uint32 src_size, uint32 dst_pitch_in_tiles, uint32* dst, bool is8x8, uint32* palette)
{
	uint32 default_palette[4] = { 0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000 };
	if (!palette)
	{
		palette = &default_palette[0];
	}

	uint32 tile_width = 8;
	uint32 tile_height = is8x8 ? 8 : 16;
	uint32 tile_size = tile_height * 2; // each line is 16 bits
	uint32 n_tiles = src_size / tile_size;
	uint32 dst_width = dst_pitch_in_tiles * tile_width;
	for (uint32 n = 0; n < n_tiles; n++)
	{
		uint32 tile = n * tile_size;		
		uint32 x_tile = n / dst_pitch_in_tiles;
		uint32 y_tile = n % dst_pitch_in_tiles;

		uint32 x_line_start = x_tile * tile_width;
		uint32 y_line_start = y_tile * tile_height;

		uint32 tile_temp[128];
		DecodeTile(&src[tile], tile_height, &tile_temp[0], palette);
		CopyTileToDestination(tile_temp, tile_height, dst, x_line_start, y_line_start, dst_width);
	}
}

void DecodeBackground(uint8* src, uint8* map, uint32* dst, bool is8x8, uint32* palette)
{
	uint32 default_palette[4] = { 0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000 };
	if (!palette)
	{
		palette = &default_palette[0];
	}

	uint32 dst_pitch_in_tiles = 32;

	uint32 tile_width = 8;
	uint32 tile_height = is8x8 ? 8 : 16;
	uint32 tile_size = tile_height * 2; // each line is 16 bits
	uint32 n_tiles = dst_pitch_in_tiles * dst_pitch_in_tiles;
	uint32 dst_width = dst_pitch_in_tiles * tile_width;

	for (uint32 n = 0; n < n_tiles; n++)
	{
		uint32 tile = map[n] * tile_size;
		uint32 x_tile = n / dst_pitch_in_tiles;
		uint32 y_tile = n % dst_pitch_in_tiles;
		uint32 x_line_start = x_tile * tile_width;
		uint32 y_line_start = y_tile * tile_height;

		uint32 tile_temp[128];
		DecodeTile(&src[tile], tile_height, &tile_temp[0], palette);
		CopyTileToDestination(tile_temp, tile_height, dst, x_line_start, y_line_start, dst_width);
	}
}
