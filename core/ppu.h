#pragma once
#include "types.h"
#include <queue>

class ZEmulator;
class ZVideoRAMController;

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 160
#define MAX_SPRITES 40
#define MAX_SPRITES_PER_SCANLINE 10

struct SLCDConfig
{
	uint8 window_display_prio : 1;
	uint8 sprite_enable : 1;
	uint8 sprite_size : 1;
	uint8 background_tilemap_start : 1;
	uint8 background_data_start : 1;
	uint8 window_enable : 1;
	uint8 window_tilemap_start : 1;
	uint8 lcd_enable : 1;
};

static_assert(sizeof(SLCDConfig) == 1, "");

class ZPPU
{
public:
	ZPPU(ZEmulator* emu);
	~ZPPU();
	bool IsLCDEnabled();
	bool IsWindowEnabled();
	bool IsSpriteEnabled();
	uint8 GetSpriteSize();

private:
	uint8 m_oamscan[MAX_SPRITES_PER_SCANLINE];
	uint8 m_oamfound = 0;

	struct SPixel
	{
		uint8 color;
		uint8 palette;
		uint8 sprite_prio;
		uint8 bg_prio;
	};

	std::deque<SPixel> m_oam_fifo;
	std::deque<SPixel> m_bg_fifo;

	void OAMScan(uint8 scanline);
	void PixelTransfer(uint8 scanline);

	ZVideoRAMController* m_vram;
	ZEmulator* m_emulator;
};