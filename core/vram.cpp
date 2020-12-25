#include "vram.h"
#include "gui.h"
#include "graphicsutil.h"
#include "emulator.h"
#include "memory.h"
#include "ppu.h"
#include "ioregisters.h"

uint32 TEX_WIDTH = 16 * 8;

uint32 BACKGROUND_WIDTH = 32 * 8;
uint32 BACKGROUND_HEIGHT = 32 * 8;

ZVideoRAMController::ZVideoRAMController(ZMainMemory * pmemory) : m_memory(pmemory)
{
	m_videobank0 = new uint8[0x2000];
	texdebug = CreateTexture2D(TEX_WIDTH, TEX_WIDTH);
	backgroundtex = CreateTexture2D(BACKGROUND_WIDTH, BACKGROUND_HEIGHT);
	memset(m_videobank0, 0xcd, 0x2000);
}

ZVideoRAMController::~ZVideoRAMController()
{
	delete[] m_videobank0;
	FreeTexture2D(texdebug);
	FreeTexture2D(backgroundtex);
}

uint8 ZVideoRAMController::LoadMemory(uint16 address)
{
	return m_videobank0[address - ADDRESS_VRAM_START];
}

void ZVideoRAMController::StoreMemory(uint16 address, uint8 value)
{
	m_videobank0[address - ADDRESS_VRAM_START] = value;
}

uint8* ZVideoRAMController::Get(uint16 address)
{
	return &m_videobank0[address - ADDRESS_VRAM_START];
}


uint16 ZVideoRAMController::GetSpriteDataStart()
{
	return SPRITE_DATA_START;
}

uint16 ZVideoRAMController::GetBackgroundDataStart()
{
	uint8 lcd = m_memory->LoadMemory(VIDEO_REGISTER_LCD_CONTROL);
	bool bZero = (lcd & BACKGROUND_DATA_START_FLAG) == 0;
	return bZero? BACKGROUND_AND_WINDOW_DATA_START_0 : BACKGROUND_AND_WINDOW_DATA_START_1;
}

uint16 ZVideoRAMController::GetBackgroundTileStart()
{
	uint8 lcd = m_memory->LoadMemory(VIDEO_REGISTER_LCD_CONTROL);
	bool bZero = (lcd & BACKGROUND_TILEMAP_START_FLAG) == 0;
	return bZero? BACKGROUND_TILE_MAP_START_0 : BACKGROUND_TILE_MAP_START_1;
}

uint16 ZVideoRAMController::GetWindowTileStart()
{
	uint8 lcd = m_memory->LoadMemory(VIDEO_REGISTER_LCD_CONTROL);
	bool bZero = (lcd & WINDOW_TILEMAP_START_FLAG) == 0;
	return bZero? WINDOW_TILE_MAP_START_0 : WINDOW_TILE_MAP_START_1;
}

void ZVideoRAMController::DrawGUI()
{
	assert(texdebug->size / 4 == TEX_WIDTH * TEX_WIDTH);
	uint32* texcpu = (uint32*)MapTexture2D(texdebug);
	DecodeImageFromTiles(Get(GetBackgroundDataStart()), 0x1000, 16, texcpu, true);
	UnmapTexture2D(texdebug);

	uint32* texback = (uint32*)MapTexture2D(backgroundtex);
	DecodeBackground(Get(GetBackgroundDataStart()), Get(GetBackgroundTileStart()), texback, true);
	UnmapTexture2D(backgroundtex);

	ImGui::Image(texdebug, ImVec2(2.0f*TEX_WIDTH, 2.0f*TEX_WIDTH));
	ImGui::Image(backgroundtex, ImVec2(2.0f*BACKGROUND_WIDTH, 2.0f*BACKGROUND_HEIGHT));
}
