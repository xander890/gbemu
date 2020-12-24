#include "mbc3.h"

uint8 ZMemoryBankController3::LoadMemory(uint16 address)
{
	if (address < 0x8000)
	{
		return m_rom[MapROM(address, m_rombank)];
	}
	else if (IsESRAM(address))
	{
		if (m_ramrtcbank < 0x04 && m_ram_enabled)
		{
			return m_ram[MapESRAM(address, m_ramrtcbank)];
		}
		else if (m_ramrtcbank >= 0x08 && m_ramrtcbank <= 0x0C && m_rtc_enabled && m_rtc_latch_state)
		{
			return m_rtc_counters[m_ramrtcbank - 0x08];
		}
	}
	return 0;
}

void ConvertDiffToTimeRegisters(uint8* registers, long long diff_secs)
{
	registers[0] = diff_secs % 60;
	long long diff_mins = diff_secs / 60;
	registers[1] = diff_mins % 60;
	long long diff_hours = diff_mins / 60;
	registers[2] = diff_hours % 24;
	long long diff_days = diff_hours / 24;
	registers[3] = diff_days & 0xFF;
	long long carries = diff_days >> 8;
	// Clear all apart from bit 6 (HALT)
	registers[4] &= (1 << 6);
	registers[4] |= (carries & 1) << 0;
	registers[4] |= (carries & 2) << (-1 + 7); // 7th bit is carry
}

void ZMemoryBankController3::StoreMemory(uint16 address, uint8 value)
{
	// Control registers
	if (address < 0x2000)
	{
		m_write_enabled = value == RAM_WRITE_ENABLE;
	}
	else if (address < 0x4000)
	{
		assert(value < 0x80);
		m_rombank = value;
	}
	else if (address < 0x6000)
	{
		m_ramrtcbank = value;
	}
	else if (address < 0x8000)
	{
		if (m_rtc_latch == 0 && value == 1)
		{
			// latch data
			m_rtc_latch_state = !m_rtc_latch_state;
			if (m_rtc_latch_state)
			{
				long long diff = 0;
				if (!GetHALT())
				{
					auto time = std::chrono::system_clock::now();
					diff = std::chrono::duration_cast<std::chrono::seconds>(time - m_time).count();
				}
				// latch the data, calculate how much time has past
				ConvertDiffToTimeRegisters(m_rtc_counters_latched, m_seconds_elapsed + diff);
			}
			else
			{
				memset(m_rtc_counters_latched, 0, sizeof(m_rtc_counters_latched));
			}
		}
		m_rtc_latch = value;
	}
	else if (IsESRAM(address))
	{
		if (m_ramrtcbank < 0x04 && m_ram_enabled && m_write_enabled)
		{
			m_ram[MapESRAM(address, m_ramrtcbank)] = value;
		}
		if (m_ramrtcbank >= 0x08 && m_ramrtcbank <= 0x0C && m_rtc_enabled && m_write_enabled)
		{
			bool halt = GetHALT();
			m_rtc_counters[m_ramrtcbank - 0x08] = value;
			bool newhalt = GetHALT();
			if (halt != newhalt)
			{
				// Start measuring...
				auto time = std::chrono::system_clock::now();
				if (newhalt)
				{
					uint64 diff = std::chrono::duration_cast<std::chrono::seconds>(time - m_time).count();
					m_seconds_elapsed += diff;
				}
				m_time = time;
			}
		}
	}
}

ZMemoryBankController3::ZMemoryBankController3(uint8* romdata, uint32 romsize) : ZROMController(romdata, romsize) 
{
	uint32 flags = GetROMFlags();
	uint32 ramsize = GetRAMSize();

	m_rombank = 1;
	m_write_enabled = false;

	m_ram_enabled = false;
	m_ramrtcbank = 0;
	m_ram = nullptr;
	m_ramsize = ramsize;
	if ((flags & SRAM) != 0)
	{
		m_ram_enabled = true;
		m_ram = new uint8[ramsize];
	}
	m_rtc_latch = 0;
	m_rtc_latch_state = false;
	m_rtc_enabled = 0;
	memset(m_rtc_counters, 0, sizeof(m_rtc_counters));

	if ((flags & RTC) != 0)
	{
		m_rtc_enabled = true;
	}

	m_time = std::chrono::system_clock::now();
}

ZMemoryBankController3::~ZMemoryBankController3()
{
	if (m_ram)
	{
		delete[] m_ram;
	}
}

inline bool ZMemoryBankController3::GetHALT()
{
	return ((m_rtc_counters[4] >> 6) & 0x1) != 0;
}