#include "mbc2.h"

uint8 ZMemoryBankController2::LoadMemory(uint16 address)
{
	if (address < 0x8000)
	{
		return m_rom[MapROM(address, m_rombank)];
	}
	else if (IsESRAM(address) && address < 0xA200)
	{
		return m_ram[MapESRAM(address, 0)] & 0xF; // 4-bits
	}
	return 0;
}

void ZMemoryBankController2::StoreMemory(uint16 address, uint8 value)
{
	if (address < 0x1000)
	{
		m_ram_enabled = value == RAM_WRITE_ENABLE;
	}
	else if ((address & 0xFF00) == 0x2100)
	{
		m_rombank = value;
	}
	else if (IsESRAM(address) && address < 0xA200 && m_ram_enabled)
	{
		m_ram[MapESRAM(address, 0)] = value & 0xF;
	}
}

ZMemoryBankController2::ZMemoryBankController2(uint8* romdata, uint32 romsize) : ZROMController(romdata, romsize) 
{
	m_rombank = 1;
	m_ram_enabled = false;
	memset(m_ram, 0, sizeof(m_ram));
}

ZMemoryBankController2::~ZMemoryBankController2()
{
}