#pragma once
#include "mbc_common.h"
#include <chrono>

class ZMemoryBankController3 : public ZROMController
{
public:
	virtual uint8 LoadMemory(uint16 address) override;
	virtual void StoreMemory(uint16 address, uint8 value) override;
	ZMemoryBankController3(uint8* romdata, uint32 romsize);
	virtual ~ZMemoryBankController3();
private:
	uint32 m_rombank;
	bool m_write_enabled;

	// External RAM
	bool m_ram_enabled;
	uint32 m_ramsize;
	uint8* m_ram;

	uint32 m_ramrtcbank;

	// Clock
	uint8 m_rtc_latch;
	bool m_rtc_latch_state;
	bool m_rtc_enabled;
	uint8 m_rtc_counters[5];
	uint8 m_rtc_counters_latched[5];
	uint64 m_seconds_elapsed = 0;

	bool GetHALT();

	std::chrono::system_clock::time_point m_time;
};
