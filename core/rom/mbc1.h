
#pragma once
#include "mbc_common.h"

class ZMemoryBankController1 : public ZROMController
{
public:
	virtual uint8 LoadMemory(uint16 address) override;
	virtual void StoreMemory(uint16 address, uint8 value) override;
	ZMemoryBankController1(uint8* romdata, uint32 romsize);
	virtual ~ZMemoryBankController1();
private:
	uint32 m_rombank;
	uint32 m_rombankhigh;
	uint32 m_rambank;
	uint32 m_ramsize;
	uint8* m_ram;
	uint32 m_banktype;
	bool m_enabled;
};
