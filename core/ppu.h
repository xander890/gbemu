#pragma once
#include "types.h"
#include <queue>
#include <mutex>

class ZMainMemory;
class ZVideoRAMController;
struct SInternalTexture;

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define MAX_SPRITES 40
#define MAX_SPRITES_PER_SCANLINE 10

enum ELCDConfigFlags : uint8
{
	WINDOW_DISPLAY_PRIO_FLAG		= 1 << 0,
	SPRITE_ENABLE_FLAG				= 1 << 1,
	SPRITE_SIZE_FLAG				= 1 << 2,
	BACKGROUND_TILEMAP_START_FLAG	= 1 << 3,
	BACKGROUND_DATA_START_FLAG		= 1 << 4,
	WINDOW_ENABLE_FLAG				= 1 << 5,
	WINDOW_TILEMAP_START_FLAG		= 1 << 6,
	LCD_ENABLE_FLAG					= 1 << 7
};

enum ELCDStatFlags : uint8
{
	MODE_FLAG_MASK					= 0x3,
	COINCIDENCE_TYPE_FLAG			= 1 << 2,
	HBLANK_INTERRUPT_FLAG			= 1 << 3,
	VBLANK_INTERRUPT_FLAG			= 1 << 4,
	OAM_INTERRUPT_FLAG				= 1 << 5,
	CONCIDENCE_INTERRUPT_FLAG		= 1 << 6,
};

class ZPPU
{
public:
	ZPPU(ZMainMemory* emu);
	~ZPPU();
	bool IsLCDEnabled();
	bool IsWindowEnabled();
	bool IsSpriteEnabled();
	uint8 GetSpriteSize();
	uint8 GetMode();
	void SetMode(uint8 mode);
	void DrawGUI();
	uint32 Step();

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

	void Flip();

	ZVideoRAMController* m_vram;
	ZMainMemory* m_memory;

	uint32* m_currentFrame;

	std::mutex m_ppuLock;
	uint32* m_currentFramePresent;
	SInternalTexture* mainscreentex;
};