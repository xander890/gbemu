#pragma once
#include "types.h"

class IMemoryController
{
public:
	virtual uint8 LoadMemory(uint16 address) = 0;
	virtual void StoreMemory(uint16 address, uint8 value) = 0;
	virtual ~IMemoryController() {}
};