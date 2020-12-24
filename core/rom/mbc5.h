#pragma once
#include "mbc_common.h"

class ZMemoryBankController5 : public ZROMController
{
public:
	virtual uint8 LoadMemory(uint16 address) override;
	virtual void StoreMemory(uint16 address, uint8 value) override;
	ZMemoryBankController5(uint8* romdata, uint32 romsize);
	virtual ~ZMemoryBankController5();
private:
	uint32 m_rombank;
	uint32 m_rombankhigh;
	uint32 m_rambank;
	uint32 m_ramsize;
	uint8* m_ram;
	bool m_enabled;
};