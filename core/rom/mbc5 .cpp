#include "mbc5.h"

uint8 ZMemoryBankController5::LoadMemory(uint16 address)
{
	if (address < 0x8000)
	{
		uint32 rombank = (m_rombankhigh << 8) | m_rombank;
		return m_rom[MapROM(address, rombank)];
	}
	else if (m_ram && IsESRAM(address))
	{
		return m_ram[MapESRAM(address, m_rambank)];
	}
	return 0;
}

void ZMemoryBankController5::StoreMemory(uint16 address, uint8 value)
{
	// Control registers
	if (address < 0x2000)
	{
		m_enabled = value == RAM_WRITE_ENABLE;
	}
	else if (address < 0x3000)
	{
		m_rombank = value == 0 ? 1 : value;
	}
	else if (address < 0x4000)
	{
		m_rombankhigh = value;
	}
	else if (address < 0x6000)
	{
		m_rambank = value;
	}
	else if (m_ram && m_enabled && IsESRAM(address))
	{
		m_ram[MapESRAM(address, m_rambank)] = value;
	}
}

ZMemoryBankController5::ZMemoryBankController5(uint8* romdata, uint32 romsize) : ZROMController(romdata, romsize) 
{
	uint32 flags = GetROMFlags();
	uint32 ramsize = GetRAMSize();

	m_rombank = 1;
	m_rombankhigh = 0;
	m_enabled = false;
	m_rambank = 0;
	m_ram = nullptr;
	m_ramsize = ramsize;
	if ((flags & SRAM) != 0)
	{
		m_ram = new uint8[ramsize];
	}
}

ZMemoryBankController5::~ZMemoryBankController5()
{
	if (m_ram)
	{
		delete[] m_ram;
	}
}
