#pragma once
#include "../types.h"
#include "../rom.h"
#include "../address_defines.h"

#define RAM_WRITE_ENABLE 0x0A

inline bool IsESRAM(uint16 address)
{
	return address >= ADDRESS_SRAM_START && address <= ADDRESS_SRAM_END;
}

inline uint32 MapESRAM(uint32 address, uint32 bank)
{
	return (address - ADDRESS_SRAM_START) + bank * SRAM_SIZE;
}

inline uint32 MapROM(uint32 address, uint32 bank)
{
	// Bank 0 is always mapped
	if (address <= ADDRESS_ROM_BANK0_END)
	{
		return address;
	}
	else
	{
		// If bank 0 is set, MBCs still map it to bank 1.
		bank = bank == 0 ? 1 : bank;
		return address + (bank - 1) * ROM_BANK1_SIZE;
	}
}