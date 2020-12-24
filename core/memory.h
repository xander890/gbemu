#pragma once
#include "types.h"
#include "imemory.h"
#include <vector>

class ZEmulator;
class ZCPU;
class ZMemoryControllerSimple;
class ZROMController;
class ZVideoRAMController;
void InstructionsWindow(ZEmulator* pemu);
void MemoryWindow(ZEmulator* pemu);

struct MemoryRangeMap
{
	uint16 address_start = 0;
	uint16 address_end = 0;
	IMemoryController* controller_map;
};

class ZMainMemory : public IMemoryController
{
public:
	enum MemoryAreas
	{
		MEMORY_AREA_ROM = 0,
		MEMORY_AREA_VRAM,
		MEMORY_AREA_WRAM,
		MEMORY_AREA_EXTERNAL_RAM,
		MEMORY_AREA_ECHO,
		MEMORY_AREA_INVALID,
		MEMORY_AREA_OAM,
		MEMORY_AREA_IO_REGISTERS,
		MEMORY_AREA_HRAM,
		MEMORY_AREA_INTERRUPT,
		MEMORY_AREA_COUNT,
	};

	ZMainMemory(ZEmulator* emu);
	~ZMainMemory();
	// Class takes ownership of pointer
	void LoadROM(uint8* rom, uint32 romsize);
	void UnloadROM();

	// Inherited via IMemoryController
	virtual uint8 LoadMemory(uint16 address) override;
	virtual void StoreMemory(uint16 address, uint8 value) override;
	uint16 GetLastWrittenAddress() { return m_nlastwritten; }
	ZROMController* GetROM();
	ZVideoRAMController* GetVRAM();
private:
	MemoryRangeMap controllers[MEMORY_AREA_COUNT];
	bool map_base_rom = false;
	uint16 m_nlastwritten = 0;
};
