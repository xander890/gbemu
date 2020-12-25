#pragma once
#include "types.h"
#include "imemory.h"
#include "graphicsdevice.h"
#include "address_defines.h"

#define BACKGROUND_AND_WINDOW_DATA_SIZE 0x1000
#define SPRITE_DATA_SIZE 0x1000

// Note: sprites have no tilemap, they use OAM
#define BACKGROUND_TILEMAP_SIZE 0x400
#define WINDOW_TILEMAP_SIZE 0x400

#define SPRITE_DATA_START 0x8000
#define WINDOW_TILE_MAP_START_0 0x9800
#define WINDOW_TILE_MAP_START_1 0x9C00
#define BACKGROUND_TILE_MAP_START_0 0x9800
#define BACKGROUND_TILE_MAP_START_1 0x9C00
#define BACKGROUND_AND_WINDOW_DATA_START_0 0x8800
#define BACKGROUND_AND_WINDOW_DATA_START_1 0x8000

class ZEmulator;
class ZVideoRAMController : public IMemoryController
{
public:
	ZVideoRAMController(ZEmulator * pEmu);
	~ZVideoRAMController();
	// Inherited via IMemoryController
	virtual uint8 LoadMemory(uint16 address) override;
	virtual void StoreMemory(uint16 address, uint8 value) override;

	uint8* GetSpriteData();
	uint8* GetBackgroundData();
	uint8* GetBackgroundTile();
	uint8* GetWindowTile();

	uint16 GetSpriteDataStart();
	uint16 GetBackgroundDataStart();
	uint16 GetBackgroundTileStart();
	uint16 GetWindowTileStart();

	void DrawGUI();
private:
	uint8* m_videobank0;
	SInternalTexture* texdebug;
	SInternalTexture* backgroundtex;
	
	ZEmulator* m_emulator;
};