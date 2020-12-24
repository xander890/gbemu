#include "mbc0.h"

uint8 ZMemoryBankController0::LoadMemory(uint16 address)
{
	if (address < 0x8000)
	{
		return m_rom[MapROM(address, 1)];
	}
	else if (m_ram && IsESRAM(address))
	{
		return m_ram[MapESRAM(address, 0)];
	}
	return 0;
}

void ZMemoryBankController0::StoreMemory(uint16, uint8)
{
	// No control registers
}

ZMemoryBankController0::ZMemoryBankController0(uint8* romdata, uint32 romsize) : ZROMController(romdata, romsize) 
{
	uint32 flags = GetROMFlags();

	assert((flags & MBC_0) != 0);
	m_ram = nullptr;
	if ((flags & SRAM) != 0)
	{
		m_ram = new uint8[SRAM_SIZE];
	}
}

ZMemoryBankController0::~ZMemoryBankController0()
{
	if (m_ram)
	{
		delete[] m_ram;
	}
}
