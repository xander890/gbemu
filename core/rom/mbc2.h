#pragma once
#include "mbc_common.h"

class ZMemoryBankController2 : public ZROMController
{
public:
	virtual uint8 LoadMemory(uint16 address) override;
	virtual void StoreMemory(uint16 address, uint8 value) override;
	ZMemoryBankController2(uint8* romdata, uint32 romsize);
	virtual ~ZMemoryBankController2();
private:
	uint32 m_rombank;
	uint8 m_ram[0x200]; // 512 x 4-bits mapped to address space
	bool m_ram_enabled;
};

