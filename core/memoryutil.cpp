#include "memoryutil.h"

uint8 ZMemoryControllerSimple::LoadMemory(uint16 address)
{
    return memory[address - m_start];
}

void ZMemoryControllerSimple::StoreMemory(uint16 address, uint8 value)
{
    memory[address - m_start] = value;
}

ZMemoryControllerSimple::ZMemoryControllerSimple(uint16 start, uint32 size)
{
    memory = new uint8[size];
    m_start = start;
    m_size = size;
    Reset();
}

ZMemoryControllerSimple::~ZMemoryControllerSimple()
{
    if (memory)
    {
        delete[] memory;
    }
}

void ZMemoryControllerSimple::Reset()
{
    memset(&memory[0], 0xCD, sizeof(memory));
}


uint8 ZMemoryControllerEcho::LoadMemory(uint16 address)
{
    return m_other->LoadMemory(address + m_offset);
}

void ZMemoryControllerEcho::StoreMemory(uint16 address, uint8 value)
{
    m_other->StoreMemory(address + m_offset, value);
}

ZMemoryControllerEcho::ZMemoryControllerEcho(IMemoryController* pMemory, int16 offset)
{
    m_other = pMemory;
    m_offset = offset;
}

uint8 ZMemoryControllerInvalid::LoadMemory(uint16)
{
    assert(false);
    return 0;
}

void ZMemoryControllerInvalid::StoreMemory(uint16, uint8)
{
    assert(false);
}
