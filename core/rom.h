#pragma once
#include "types.h"
#include "imemory.h"
#include "types.h"
#include <vector>
#include <string>
#include <map>

enum CartridgeRunMode : uint8
{
	RUN_MODE_DMG = 0x00,
	RUN_MODE_GBC = 0xC0,
	RUN_MODE_GBC_AND_DMG = 0x80
};

enum CartridgeSGBMode : uint8
{
	SGB_ENABLED = 0x03,
	SGB_DISABLED = 0x00
};

enum CartridgeFlags : uint32
{
	MBC_0 = 1 << 0,
	MBC_1 = 1 << 1,
	MBC_2 = 1 << 2,
	MBC_3 = 1 << 3,
	MBC_5 = 1 << 4,
	RTC = 1 << 5,
	RUMBLE = 1 << 6,
	SRAM = 1 << 7,
	BATTERY = 1 << 8,
};


enum CartridgeRegion : uint8
{
	REGION_JAPAN = 0x00,
	REGION_WORLD = 0x01,
};

struct SCartridgeHeader
{
	uint8 startVector[4];
	uint8 nintendoLogo[48];
	char gameTitle[11];
	char manufacturer[4];
	CartridgeRunMode gbcFlag;
	char newLicenseeCode[2];
	CartridgeSGBMode sgbFlag;
	uint8 cartridgeType;
	uint8 ROMSize;
	uint8 RAMSize;
	CartridgeRegion region;
	uint8 oldLicenseeCode;
	uint8 ROMVersion;
	uint8 headerChecksum;
	uint8 globalCheckSum[2];
};

class ZROMController : public IMemoryController
{
public:
	virtual uint8 LoadMemory(uint16 address) override = 0;
	virtual void StoreMemory(uint16 address, uint8 value) override = 0;
	virtual ~ZROMController();
	static ZROMController* Create(uint8* romdata, uint32 romsize);
	SCartridgeHeader* GetHeader();

	uint32 GetROMSize();
	uint32 GetROMFlags();
	uint32 GetRAMSize();

protected:
	ZROMController(uint8* rom, uint32 romsize);
	uint32 m_romsize;
	uint8* m_rom;

private:
	static std::map<uint8, uint32> g_CartridgeDescription;
};

void ROMWindow(ZROMController* pController);