#pragma once
#include "mbc_common.h"

class ZMemoryBankController0 : public ZROMController
{
public:
	virtual uint8 LoadMemory(uint16 address) override;
	virtual void StoreMemory(uint16 address, uint8 value) override;
	ZMemoryBankController0(uint8* romdata, uint32 romsize);
	virtual ~ZMemoryBankController0();
private:
	uint8* m_ram;
};