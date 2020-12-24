#pragma once
#include "types.h"
#include "imemory.h"

class ZMemoryControllerSimple : public IMemoryController
{
public:
	uint8 LoadMemory(uint16 address);
	void StoreMemory(uint16 address, uint8 value);

	ZMemoryControllerSimple(uint16 start, uint32 size);
	virtual ~ZMemoryControllerSimple();

	void Reset();
private:
	uint8* memory;
	uint16 m_start;
	uint32 m_size;
};

class ZMemoryControllerEcho : public IMemoryController
{
public:
	uint8 LoadMemory(uint16 address);
	void StoreMemory(uint16 address, uint8 value);
	ZMemoryControllerEcho(IMemoryController* pMemory, int16 offset);
private:
	IMemoryController* m_other;
	int16 m_offset;
};

class ZMemoryControllerInvalid : public IMemoryController
{
public:
	uint8 LoadMemory(uint16 address);
	void StoreMemory(uint16 address, uint8 value);
};