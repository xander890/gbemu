#include "cpu.h"
#include "emulator.h"
#include <string>
#include <iostream>
#include <sstream>
#include "gui.h"
#include <thread>
#include "ioregisters.h"
#pragma warning(disable : 4127)

// Works for uint8 and uint16 only
bool CheckCarryAdd(uint32 a, uint32 b, uint32 bit)
{
	uint32 nBitSetMask = (1 << (bit + 1));
	uint32 nMask = nBitSetMask - 1;
	return (nBitSetMask & ((a & nMask) + (b & nMask))) == nBitSetMask;
}
bool CheckCarrySub(uint32 a, uint32 b, uint32 bit)
{
	uint32 nMask = (1 << (bit + 1)) - 1;
	uint32 nMaskFinal = 0x80000000;
	return (nMaskFinal & ((a & nMask) - (b & nMask))) == nMaskFinal;
}
bool CheckCarryAdd(uint32 a, uint32 b, uint32 c, uint32 bit)
{
	uint32 nBitSetMask = (1 << (bit + 1));
	uint32 nMask = nBitSetMask - 1;
	return (nBitSetMask & ((a & nMask) + (b & nMask) + (c & nMask))) == nBitSetMask;
}
bool CheckCarrySub(uint32 a, uint32 b, uint32 c, uint32 bit)
{
	uint32 nMask = (1 << (bit + 1)) - 1;
	uint32 nMaskFinal = 0x80000000;
	return (nMaskFinal & ((a & nMask) - (b & nMask) - (c & nMask))) == nMaskFinal;
}

template<>
uint8 ZCPU::ParseArgument()
{
	uint8 val = LoadMemory(registers.r16.pc);
	IncrementPC();
	return val;
}

template<>
uint16 ZCPU::ParseArgument()
{
	uint16 val = LoadMemory(registers.r16.pc);
	IncrementPC();
	val |= LoadMemory(registers.r16.pc) << 8;
	IncrementPC();
	return val;
}

template<ZCPU::Argument8 storeoffset, ZCPU::Argument8 loadoffset>
void ZCPU::LoadStore()
{
	uint8 val = LoadOffset<loadoffset>();
	StoreOffset<storeoffset>(val);
}

template<ZCPU::Argument8 storeoffset, ZCPU::Argument8 loadoffset>
void ZCPU::LoadStoreIncrement()
{
	LoadStore<storeoffset, loadoffset>();
	assert(storeoffset == POINTER_HL || loadoffset == POINTER_HL);
	registers.r16.hl++;
}

template<ZCPU::Argument8 storeoffset, ZCPU::Argument8 loadoffset>
void ZCPU::LoadStoreDecrement()
{
	LoadStore<storeoffset, loadoffset>();
	assert(storeoffset == POINTER_HL || loadoffset == POINTER_HL);
	registers.r16.hl--;
}

template<ZCPU::Argument16 offset>
void ZCPU::LoadConstant()
{
	registers.raw16[offset] = ParseArgument<uint16>();
}

template<ZCPU::Argument8 offset>
inline void ZCPU::Increment()
{
	uint8 reg = LoadOffset<offset>();
	uint32 value = reg;
	reg += 1;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, reg == 0);
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarryAdd(value, 1, 3));
	StoreOffset<offset>(reg);
}

template<ZCPU::Argument8 offset>
inline void ZCPU::Decrement()
{
	uint8 reg = LoadOffset<offset>();
	uint32 value = reg;
	reg -= 1;
	SetFlag(CPU_FLAG_SUBTRACT, true);
	SetFlag(CPU_FLAG_ZERO, reg == 0);
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarrySub(value, 1, 3));
	StoreOffset<offset>(reg);
}

template<ZCPU::Argument8 offset>
uint8 ZCPU::LoadOffset()
{
	if (offset == CONSTANT_8)
	{
		return ParseArgument<uint8>();
	}
	else if (offset == POINTER_CONSTANT)
	{
		uint16 address = ParseArgument<uint16>();
		return LoadMemory(address);
	}
	else if (offset == POINTER_FIRST_PAGE_CONSTANT)
	{
		uint16 address = 0xFF00 | ParseArgument<uint8>();
		return LoadMemory(address);
	}
	else if (offset == POINTER_FIRST_PAGE_REGISTER_C)
	{
		uint16 address = 0xFF00 | registers.r8.c;
		return LoadMemory(address);
	}
	else if (offset > POINTER_ENUM_OFFSET)
	{
		return LoadMemory(registers.raw16[offset - POINTER_ENUM_OFFSET]);
	}
	else
	{
		return registers.raw8[offset];
	}
}

template<ZCPU::Argument8 offset>
void ZCPU::StoreOffset(uint8 value)
{
	if (offset == CONSTANT_8)
	{
		assert(false); // Bad boy
	}
	else if (offset == POINTER_CONSTANT)
	{
		uint16 address = ParseArgument<uint16>();
		StoreMemory(address, value);
	}
	else if (offset == POINTER_FIRST_PAGE_CONSTANT)
	{
		uint16 address = 0xFF00 | ParseArgument<uint8>();
		StoreMemory(address, value);
	}
	else if (offset == POINTER_FIRST_PAGE_REGISTER_C)
	{
		uint16 address = 0xFF00 | registers.r8.c;
		StoreMemory(address, value);
	}
	else if (offset > POINTER_ENUM_OFFSET)
	{
		StoreMemory(registers.raw16[offset - POINTER_ENUM_OFFSET], value);
	}
	else
	{
		registers.raw8[offset] = value;
	}
}

template<ZCPU::Argument16 offset16>
void ZCPU::Push()
{
	uint16 value = registers.raw16[offset16];
	StoreMemory(registers.r16.sp - 1, value >> 8);
	StoreMemory(registers.r16.sp - 2, value & 0xFF);
	registers.r16.sp -= 2;
}

template<ZCPU::Argument16 offset16>
void ZCPU::Pop()
{
	uint16* value = &registers.raw16[offset16];
	*((uint8*)value + 0) = LoadMemory(registers.r16.sp);
	*((uint8*)value + 1) = LoadMemory(registers.r16.sp + 1);
	registers.r16.sp += 2;
}

template<ZCPU::Argument8 offset>
void ZCPU::Add()
{
	uint8 operand = LoadOffset<offset>();
	uint8 a = registers.r8.a;
	uint8 result = operand + a;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, result == 0);
	SetFlag(CPU_FLAG_CARRY, CheckCarryAdd(a, operand, 7));
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarryAdd(a, operand, 3));
	registers.r8.a = result;
}

template<ZCPU::Argument8 offset>
void ZCPU::AddWithCarry()
{
	uint8 operand = LoadOffset<offset>();
	uint8 a = registers.r8.a;
	uint8 carry = GetCarry();
	uint8 result = operand + a + carry;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, result == 0);
	SetFlag(CPU_FLAG_CARRY, CheckCarryAdd(a, operand, carry, 7));
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarryAdd(a, operand, carry, 3));
	registers.r8.a = result;
}

template<ZCPU::Argument8 offset>
void ZCPU::Sub()
{
	uint8 operand = LoadOffset<offset>();
	uint8 a = registers.r8.a;
	uint8 result = a - operand;
	SetFlag(CPU_FLAG_SUBTRACT, true);
	SetFlag(CPU_FLAG_ZERO, result == 0);
	SetFlag(CPU_FLAG_CARRY, CheckCarrySub(a, operand, 7));
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarrySub(a, operand, 3));
	registers.r8.a = result;
}

template<ZCPU::Argument8 offset>
void ZCPU::SubWithCarry()
{
	uint8 operand = LoadOffset<offset>();
	uint8 a = registers.r8.a;
	uint8 carry = GetCarry();
	uint8 result = a - operand - carry;
	SetFlag(CPU_FLAG_SUBTRACT, true);
	SetFlag(CPU_FLAG_ZERO, result == 0);
	SetFlag(CPU_FLAG_CARRY, CheckCarrySub(a, operand, carry, 7));
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarrySub(a, operand, carry, 3));
	registers.r8.a = result;
}

template<ZCPU::Argument8 offset>
void ZCPU::And()
{
	uint8 operand = LoadOffset<offset>();
	uint8 a = registers.r8.a;
	uint8 result = operand & a;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, result == 0);
	SetFlag(CPU_FLAG_CARRY, false);
	SetFlag(CPU_FLAG_HALF_CARRY, true);
	registers.r8.a = result;

}

template<ZCPU::Argument8 offset>
void ZCPU::Or()
{
	uint8 operand = LoadOffset<offset>();
	uint8 a = registers.r8.a;
	uint8 result = operand | a;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, result == 0);
	SetFlag(CPU_FLAG_CARRY, false);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	registers.r8.a = result;
}

template<ZCPU::Argument8 offset>
void ZCPU::Xor()
{
	uint8 operand = LoadOffset<offset>();
	uint8 a = registers.r8.a;
	uint8 result = operand ^ a;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, result == 0);
	SetFlag(CPU_FLAG_CARRY, false);
	SetFlag(CPU_FLAG_HALF_CARRY, true);
	registers.r8.a = result;

}

template<ZCPU::Argument8 offset>
void ZCPU::Compare()
{
	// Like a SUB but no store in A
	uint8 operand = LoadOffset<offset>();
	uint8 a = registers.r8.a;
	uint8 result = a - operand;
	SetFlag(CPU_FLAG_SUBTRACT, true);
	SetFlag(CPU_FLAG_ZERO, result == 0);
	SetFlag(CPU_FLAG_CARRY, CheckCarrySub(a, operand, 7));
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarrySub(a, operand, 3));
}

template<int offsetfirst, int offsetsecond>
void ZCPU::Add16()
{
	uint16 operand = registers.raw16[offsetsecond];
	uint16 second = registers.raw16[offsetfirst];
	uint16 result = operand + second;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_CARRY, CheckCarryAdd(operand, second, 15));
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarryAdd(operand, second, 11));
	registers.raw16[offsetfirst] = result;
}

void ZCPU::AddSPOffset()
{
	uint8 argument = ParseArgument<uint8>();
	int8 val = argument;
	uint16 v = registers.r16.sp;
	registers.r16.sp += val;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, false);
	SetFlag(CPU_FLAG_CARRY, CheckCarryAdd(v, val, 15));
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarryAdd(v, val, 11));
}

template<int offset>
void ZCPU::Increment16()
{
	registers.raw16[offset]++;
}

template<int offset>
void ZCPU::Decrement16()
{
	registers.raw16[offset]--;
}

void RotateLeftImpl(bool withcarry, uint8& reg, uint8& carry)
{
	uint8 lastbit = reg >> 7;
	reg <<= 1;
	if (withcarry)
	{
		reg = reg | carry;
		carry = lastbit;
	}
	else
	{
		reg = reg | lastbit;
		carry = lastbit;
	}
}


void RotateRightImpl(bool withcarry, uint8& reg, uint8& carry)
{
	uint8 firstbit = reg & 0x01;
	reg >>= 1; //Clear last bit
	if (withcarry)
	{
		reg = reg | (carry << 7);
		carry = firstbit;
	}
	else
	{
		reg = reg | (firstbit << 7);
		carry = firstbit;
	}
}

template<bool withcarry>
void ZCPU::RotateLeftA()
{
	uint8 a = registers.r8.a;
	uint8 carry = GetCarry();
	RotateLeftImpl(withcarry, a, carry);
	registers.r8.a = a;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_CARRY, carry);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	SetFlag(CPU_FLAG_ZERO, false);
}

template<bool withcarry>
void ZCPU::RotateRightA()
{
	uint8 a = registers.r8.a;
	uint8 carry = GetCarry();
	RotateRightImpl(withcarry, a, carry);
	registers.r8.a = a;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_CARRY, carry);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	SetFlag(CPU_FLAG_ZERO, false);
}

template<bool withcarry, ZCPU::Argument8 offset>
void ZCPU::RotateLeft()
{
	uint8 reg = LoadOffset<offset>();
	uint8 carry = GetCarry();
	RotateLeftImpl(withcarry, reg, carry);
	StoreOffset<offset>(reg);
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_CARRY, carry);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	SetFlag(CPU_FLAG_ZERO, reg == 0);

}

template<bool withcarry, ZCPU::Argument8 offset>
void ZCPU::RotateRight()
{
	uint8 reg = LoadOffset<offset>();
	uint8 carry = GetCarry();
	RotateRightImpl(withcarry, reg, carry);
	StoreOffset<offset>(reg);
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_CARRY, carry);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	SetFlag(CPU_FLAG_ZERO, reg == 0);
}

template<ZCPU::Argument8 offset>
void ZCPU::ShiftLeft()
{
	uint8 reg = LoadOffset<offset>();
	uint8 bit = (reg >> 7) & 1;
	reg <<= 1;
	StoreOffset<offset>(reg);
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_CARRY, bit);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	SetFlag(CPU_FLAG_ZERO, reg == 0);
}

template<bool zeromsb, ZCPU::Argument8 offset>
void ZCPU::ShiftRight()
{ 
	uint8 reg = LoadOffset<offset>();
	uint8 bit = reg & 0x01;
	uint8 lastbit = reg & 0x80;
	reg >>= 1;
	if (!zeromsb)
	{
		reg |= lastbit;
	}
	StoreOffset<offset>(reg);
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_CARRY, bit);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	SetFlag(CPU_FLAG_ZERO, reg == 0);
}

template<ZCPU::Argument8 offset>
void ZCPU::Swap()
{
	uint8 result = LoadOffset<offset>();
	result = (result << 4) | (result >> 4);
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, result == 0);
	SetFlag(CPU_FLAG_CARRY, false);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	StoreOffset<offset>(result);
}


template<int bit, ZCPU::Argument8 offset>
void ZCPU::Bit()
{
	uint8 result = LoadOffset<offset>();
	uint8 bits = (result >> bit) & 1;
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, 1 - bits);
	SetFlag(CPU_FLAG_HALF_CARRY, true);
}

template<int bit, ZCPU::Argument8 offset>
void ZCPU::Set()
{
	uint8 result = LoadOffset<offset>();
	result |= (1 << bit);
	StoreOffset<offset>(result);
}

template<int bit, ZCPU::Argument8 offset>
void ZCPU::Reset()
{
	uint8 result = LoadOffset<offset>();
	result &= ~(1 << bit);
	StoreOffset<offset>(result);
}

template<ZCPU::JumpCondition condition>
bool ZCPU::EvaluateCondition()
{
	if (condition == ZCPU::JUMP_CONDITION_ALWAYS)
		return true;
	else if (condition == ZCPU::JUMP_CONDITION_CARRY)
		return (registers.r8.f & Flags::CPU_FLAG_CARRY) != 0;
	else if (condition == ZCPU::JUMP_CONDITION_NOT_CARRY)
		return (registers.r8.f & Flags::CPU_FLAG_CARRY) == 0;
	else if (condition == ZCPU::JUMP_CONDITION_ZERO)
		return (registers.r8.f & Flags::CPU_FLAG_ZERO) != 0;
	else if (condition == ZCPU::JUMP_CONDITION_NOT_ZERO)
		return (registers.r8.f & Flags::CPU_FLAG_ZERO) == 0;
	assert(false);
}

template<ZCPU::JumpCondition condition>
void ZCPU::Jump()
{
	bool jump = EvaluateCondition<condition>();
	uint16 argument = ParseArgument<uint16>();
	if (jump)
	{
		registers.r16.pc = argument;
	}
	m_instruction_args.cycles = jump ? g_Instructions[m_instruction_args.opcode].cycles_max : g_Instructions[m_instruction_args.opcode].cycles_min;
}

template<ZCPU::JumpCondition condition>
void ZCPU::JumpIncrement()
{
	bool jump = EvaluateCondition<condition>();
	uint8 argument = ParseArgument<uint8>();
	int16 signedarg = (int16)((int8)argument);
	if (jump)
	{
		registers.r16.pc = registers.r16.pc + signedarg;
	}
	m_instruction_args.cycles = jump ? g_Instructions[m_instruction_args.opcode].cycles_max : g_Instructions[m_instruction_args.opcode].cycles_min;
}

template<ZCPU::JumpCondition condition>
void ZCPU::Call()
{
	bool jump = EvaluateCondition<condition>();
	uint16 argument = ParseArgument<uint16>();
	if (jump)
	{
		StoreMemory(registers.r16.sp - 1, registers.r16.pc >> 8);
		StoreMemory(registers.r16.sp - 2, registers.r16.pc & 0xFF);
		registers.r16.pc = argument;
		registers.r16.sp -= 2;
	}
	m_instruction_args.cycles = jump ? g_Instructions[m_instruction_args.opcode].cycles_max : g_Instructions[m_instruction_args.opcode].cycles_min;
}

template<ZCPU::JumpCondition condition>
void ZCPU::Return()
{
	bool jump = EvaluateCondition<condition>();
	if (jump)
	{
		registers.r16.pc = LoadMemory(registers.r16.sp);
		registers.r16.pc |= LoadMemory(registers.r16.sp + 1) << 8;
		registers.r16.sp += 2;
	}
	m_instruction_args.cycles = jump ? g_Instructions[m_instruction_args.opcode].cycles_max : g_Instructions[m_instruction_args.opcode].cycles_min;
}

template<uint8 addresslowbits>
void ZCPU::Restart()
{
	StoreMemory(registers.r16.sp - 1, registers.r16.pc >> 8);
	StoreMemory(registers.r16.sp - 2, registers.r16.pc & 0xFF);
	registers.r16.pc = addresslowbits;
	registers.r16.sp -= 2;
	DisableInterrupts();
}



ZCPU::ZCPU(IMemoryController* mem)
{
	memory = mem;

#define BIND(...) std::bind(__VA_ARGS__, this)

	instructions[0x00] = BIND(&ZCPU::Nop);	//{"NOP", 1, 4, 4},
	instructions[0x01] = BIND(&ZCPU::LoadConstant<REGISTER_BC>);	//{"LD BC,%02X", 3, 12, 12},
	instructions[0x02] = BIND(&ZCPU::LoadStore<POINTER_BC, REGISTER_A>);	//{"LD (BC),A", 1, 8, 8},
	instructions[0x03] = BIND(&ZCPU::Increment16<REGISTER_BC>);	//{"INC BC", 1, 8, 8},
	instructions[0x04] = BIND(&ZCPU::Increment<REGISTER_B>);	//{"INC B", 1, 4, 4},
	instructions[0x05] = BIND(&ZCPU::Decrement<REGISTER_B>);	//{"DEC B", 1, 4, 4},
	instructions[0x06] = BIND(&ZCPU::LoadStore<REGISTER_B, CONSTANT_8>);	//{"LD B,%X", 2, 8, 8},
	instructions[0x07] = BIND(&ZCPU::RotateLeftA<false>);		//{"RLCA", 1, 4, 4},
	instructions[0x08] = BIND(&ZCPU::StoreSP);		//{"LD (%02X),SP", 3, 20, 20},
	instructions[0x09] = BIND(&ZCPU::Add16<REGISTER_HL, REGISTER_BC>);		//{"ADD HL,BC", 1, 8, 8},
	instructions[0x0A] = BIND(&ZCPU::LoadStore<REGISTER_A, POINTER_BC>);		//{"LD A,(BC)", 1, 8, 8},
	instructions[0x0B] = BIND(&ZCPU::Decrement16<REGISTER_BC>);		//{"DEC BC", 1, 8, 8},
	instructions[0x0C] = BIND(&ZCPU::Increment<REGISTER_C>);		//{"INC C", 1, 4, 4},
	instructions[0x0D] = BIND(&ZCPU::Decrement<REGISTER_C>);		//{"DEC C", 1, 4, 4},
	instructions[0x0E] = BIND(&ZCPU::LoadStore<REGISTER_C, CONSTANT_8>);		//{"LD C,%X", 2, 8, 8},
	instructions[0x0F] = BIND(&ZCPU::RotateRightA<false>);		//{"RRCA", 1, 4, 4},

	instructions[0x10] = BIND(&ZCPU::Stop);		//{"STOP 0", 2, 4, 4},
	instructions[0x11] = BIND(&ZCPU::LoadConstant<REGISTER_DE>);		//{"LD DE,%02X", 3, 12, 12},
	instructions[0x12] = BIND(&ZCPU::LoadStore<POINTER_DE, REGISTER_A>);		//{"LD (DE),A", 1, 8, 8},
	instructions[0x13] = BIND(&ZCPU::Increment16<REGISTER_DE>);		//{"INC DE", 1, 8, 8},
	instructions[0x14] = BIND(&ZCPU::Increment<REGISTER_D>);		//{"INC D", 1, 4, 4},
	instructions[0x15] = BIND(&ZCPU::Decrement<REGISTER_D>);		//{"DEC D", 1, 4, 4},
	instructions[0x16] = BIND(&ZCPU::LoadStore<REGISTER_D, CONSTANT_8>);		//{"LD D,%X", 2, 8, 8},
	instructions[0x17] = BIND(&ZCPU::RotateLeftA<true>);		//{"RLA", 1, 4, 4},
	instructions[0x18] = BIND(&ZCPU::JumpIncrement<ZCPU::JUMP_CONDITION_ALWAYS>);		//{"JR %X", 2, 12, 12},
	instructions[0x19] = BIND(&ZCPU::Add16<REGISTER_HL, REGISTER_DE>);		//{"ADD HL,DE", 1, 8, 8},
	instructions[0x1A] = BIND(&ZCPU::LoadStore<REGISTER_A, POINTER_DE>);		//{"LD A,(DE)", 1, 8, 8},
	instructions[0x1B] = BIND(&ZCPU::Decrement16<REGISTER_DE>);		//{"DEC DE", 1, 8, 8},
	instructions[0x1C] = BIND(&ZCPU::Increment<REGISTER_E>);		//{"INC E", 1, 4, 4},
	instructions[0x1D] = BIND(&ZCPU::Decrement<REGISTER_E>);		//{"DEC E", 1, 4, 4},
	instructions[0x1E] = BIND(&ZCPU::LoadStore<REGISTER_E, CONSTANT_8>);		//{"LD E,%X", 2, 8, 8},
	instructions[0x1F] = BIND(&ZCPU::RotateRightA<true>);		//{"RRA", 1, 4, 4},

	instructions[0x20] = BIND(&ZCPU::JumpIncrement<ZCPU::JUMP_CONDITION_NOT_ZERO>);		//{"JR NZ,%X", 2, 12, 8},
	instructions[0x21] = BIND(&ZCPU::LoadConstant<REGISTER_HL>);		//{"LD HL,%02X", 3, 12, 12},
	instructions[0x22] = BIND(&ZCPU::LoadStoreIncrement<POINTER_HL, REGISTER_A>);		//{"LD (HL+),A", 1, 8, 8},
	instructions[0x23] = BIND(&ZCPU::Increment16<REGISTER_HL>);		//{"INC HL", 1, 8, 8},
	instructions[0x24] = BIND(&ZCPU::Increment<REGISTER_H>);		//{"INC H", 1, 4, 4},
	instructions[0x25] = BIND(&ZCPU::Decrement<REGISTER_H>);		//{"DEC H", 1, 4, 4},
	instructions[0x26] = BIND(&ZCPU::LoadStore<REGISTER_H, CONSTANT_8>);		//{"LD H,%X", 2, 8, 8},
	instructions[0x27] = BIND(&ZCPU::DecimalAdjust);		//{"DAA", 1, 4, 4},
	instructions[0x28] = BIND(&ZCPU::JumpIncrement<ZCPU::JUMP_CONDITION_ZERO>);		//{"JR Z,%X", 2, 12, 8},
	instructions[0x29] = BIND(&ZCPU::Add16<REGISTER_HL, REGISTER_HL>);		//{"ADD HL,HL", 1, 8, 8},
	instructions[0x2A] = BIND(&ZCPU::LoadStoreIncrement<REGISTER_A, POINTER_HL>);		//{"LD A,(HL+)", 1, 8, 8},
	instructions[0x2B] = BIND(&ZCPU::Decrement16<REGISTER_HL>);		//{"DEC HL", 1, 8, 8},
	instructions[0x2C] = BIND(&ZCPU::Increment<REGISTER_L>);		//{"INC L", 1, 4, 4},
	instructions[0x2D] = BIND(&ZCPU::Decrement<REGISTER_L>);		//{"DEC L", 1, 4, 4},
	instructions[0x2E] = BIND(&ZCPU::LoadStore<REGISTER_L, CONSTANT_8>);		//{"LD L,%X", 2, 8, 8},
	instructions[0x2F] = BIND(&ZCPU::Complement);		//{"CPL", 1, 4, 4},

	instructions[0x30] = BIND(&ZCPU::JumpIncrement<ZCPU::JUMP_CONDITION_NOT_CARRY>);		//{"JR NC,%X", 2, 12, 8},
	instructions[0x31] = BIND(&ZCPU::LoadConstant<REGISTER_SP>);		//{"LD SP,%02X", 3, 12, 12},
	instructions[0x32] = BIND(&ZCPU::LoadStoreDecrement<POINTER_HL, REGISTER_A>);		//{"LD (HL-),A", 1, 8, 8},
	instructions[0x33] = BIND(&ZCPU::Increment16<REGISTER_SP>);		//{"INC SP", 1, 8, 8},
	instructions[0x34] = BIND(&ZCPU::Increment<POINTER_HL>);		//{"INC (HL)", 1, 12, 12},
	instructions[0x35] = BIND(&ZCPU::Decrement<POINTER_HL>);		//{"DEC (HL)", 1, 12, 12},
	instructions[0x36] = BIND(&ZCPU::LoadStore<POINTER_HL, CONSTANT_8>);		//{"LD (HL),%X", 2, 12, 12},
	instructions[0x37] = BIND(&ZCPU::SetCarry);		//{"SCF", 1, 4, 4},
	instructions[0x38] = BIND(&ZCPU::JumpIncrement<ZCPU::JUMP_CONDITION_CARRY>);		//{"JR C,%X", 2, 12, 8},
	instructions[0x39] = BIND(&ZCPU::Add16<REGISTER_HL, REGISTER_SP>);		//{"ADD HL,SP", 1, 8, 8},
	instructions[0x3A] = BIND(&ZCPU::LoadStoreDecrement<REGISTER_A, POINTER_HL>);		//{"LD A,(HL-)", 1, 8, 8},
	instructions[0x3B] = BIND(&ZCPU::Decrement16<REGISTER_SP>);		//{"DEC SP", 1, 8, 8},
	instructions[0x3C] = BIND(&ZCPU::Increment<REGISTER_A>);		//{"INC A", 1, 4, 4},
	instructions[0x3D] = BIND(&ZCPU::Decrement<REGISTER_A>);		//{"DEC A", 1, 4, 4},
	instructions[0x3E] = BIND(&ZCPU::LoadStore<REGISTER_A, CONSTANT_8>);		//{"LD A,%X", 2, 8, 8},
	instructions[0x3F] = BIND(&ZCPU::ComplementCarry);		//{"CCF", 1, 4, 4},

#define INSTRUCTION_SET1(inst, baseidx, function, argfixed) \
	inst[baseidx + 0] = BIND(&ZCPU::function<argfixed, REGISTER_B>); \
	inst[baseidx + 1] = BIND(&ZCPU::function<argfixed, REGISTER_C>); \
	inst[baseidx + 2] = BIND(&ZCPU::function<argfixed, REGISTER_D>); \
	inst[baseidx + 3] = BIND(&ZCPU::function<argfixed, REGISTER_E>); \
	inst[baseidx + 4] = BIND(&ZCPU::function<argfixed, REGISTER_H>); \
	inst[baseidx + 5] = BIND(&ZCPU::function<argfixed, REGISTER_L>); \
	inst[baseidx + 6] = BIND(&ZCPU::function<argfixed, POINTER_HL>); \
	inst[baseidx + 7] = BIND(&ZCPU::function<argfixed, REGISTER_A>); 

#define INSTRUCTION_SET(inst, baseidx, function) \
	inst[baseidx + 0] = BIND(&ZCPU::function<REGISTER_B>); \
	inst[baseidx + 1] = BIND(&ZCPU::function<REGISTER_C>); \
	inst[baseidx + 2] = BIND(&ZCPU::function<REGISTER_D>); \
	inst[baseidx + 3] = BIND(&ZCPU::function<REGISTER_E>); \
	inst[baseidx + 4] = BIND(&ZCPU::function<REGISTER_H>); \
	inst[baseidx + 5] = BIND(&ZCPU::function<REGISTER_L>); \
	inst[baseidx + 6] = BIND(&ZCPU::function<POINTER_HL>); \
	inst[baseidx + 7] = BIND(&ZCPU::function<REGISTER_A>); 

	INSTRUCTION_SET1(instructions, 0x40, LoadStore, REGISTER_B) //{"LD B,B", 1, 4, 4},
	INSTRUCTION_SET1(instructions, 0x48, LoadStore, REGISTER_C) //{"LD C,B", 1, 4, 4},
	INSTRUCTION_SET1(instructions, 0x50, LoadStore, REGISTER_D) //{"LD D,B", 1, 4, 4},
	INSTRUCTION_SET1(instructions, 0x58, LoadStore, REGISTER_E) //{"LD E,B", 1, 4, 4},
	INSTRUCTION_SET1(instructions, 0x60, LoadStore, REGISTER_H) //{"LD H,B", 1, 4, 4},
	INSTRUCTION_SET1(instructions, 0x68, LoadStore, REGISTER_L) //{"LD L,B", 1, 4, 4},
	INSTRUCTION_SET1(instructions, 0x70, LoadStore, POINTER_HL) //{"LD (HL),B", 1, 8, 8},
	INSTRUCTION_SET1(instructions, 0x78, LoadStore, REGISTER_A) //{"LD A,B", 1, 4, 4},

	instructions[0x76] = BIND(&ZCPU::Halt);		//{"HALT", 1, 4, 4},

	INSTRUCTION_SET(instructions, 0x80, Add) //{"ADD A,B", 1, 4, 4},
	INSTRUCTION_SET(instructions, 0x88, AddWithCarry) //{"ADC A,B", 1, 4, 4},
	INSTRUCTION_SET(instructions, 0x90, Sub) //{"SUB B", 1, 4, 4},
	INSTRUCTION_SET(instructions, 0x98, SubWithCarry) //{"SBC A,B", 1, 4, 4},
	INSTRUCTION_SET(instructions, 0xA0, And) //{"AND B", 1, 4, 4},
	INSTRUCTION_SET(instructions, 0xA8, Xor) //{"XOR B", 1, 4, 4},
	INSTRUCTION_SET(instructions, 0xB0, Or) //{"OR B", 1, 4, 4},
	INSTRUCTION_SET(instructions, 0xB8, Compare) //{"CP B", 1, 4, 4},

	instructions[0xC0] = BIND(&ZCPU::Return<ZCPU::JUMP_CONDITION_NOT_ZERO>);		//{"RET NZ", 1, 20, 8},
	instructions[0xC1] = BIND(&ZCPU::Pop<REGISTER_BC>);		//{"POP BC", 1, 12, 12},
	instructions[0xC2] = BIND(&ZCPU::Jump<ZCPU::JUMP_CONDITION_NOT_ZERO>);		//{"JP NZ,%02X", 3, 16, 12},
	instructions[0xC3] = BIND(&ZCPU::Jump<ZCPU::JUMP_CONDITION_ALWAYS>);		//{"JP %02X", 3, 16, 16},
	instructions[0xC4] = BIND(&ZCPU::Call<ZCPU::JUMP_CONDITION_NOT_ZERO>);		//{"CALL NZ,%02X", 3, 24, 12},
	instructions[0xC5] = BIND(&ZCPU::Push<REGISTER_BC>);		//{"PUSH BC", 1, 16, 16},
	instructions[0xC6] = BIND(&ZCPU::Add<CONSTANT_8>);		//{"ADD A,%X", 2, 8, 8},
	instructions[0xC7] = BIND(&ZCPU::Restart<0x00>);		//{"RST 00H", 1, 16, 16},
	instructions[0xC8] = BIND(&ZCPU::Return<ZCPU::JUMP_CONDITION_ZERO>);		//{"RET Z", 1, 20, 8},
	instructions[0xC9] = BIND(&ZCPU::Return<ZCPU::JUMP_CONDITION_ALWAYS>);		//{"RET", 1, 16, 16},
	instructions[0xCA] = BIND(&ZCPU::Jump<ZCPU::JUMP_CONDITION_ZERO>);		//{"JP Z,%02X", 3, 16, 12},
	instructions[0xCB] = BIND(&ZCPU::PrefixCB);		//{"PREFIX CB", 1, 4, 4},
	instructions[0xCC] = BIND(&ZCPU::Call<ZCPU::JUMP_CONDITION_ZERO>);		//{"CALL Z,%02X", 3, 24, 12},
	instructions[0xCD] = BIND(&ZCPU::Call<ZCPU::JUMP_CONDITION_ALWAYS>);		//{"CALL %02X", 3, 24, 24},
	instructions[0xCE] = BIND(&ZCPU::AddWithCarry<CONSTANT_8>);		//{"ADC A,%X", 2, 8, 8},
	instructions[0xCF] = BIND(&ZCPU::Restart<0x08>);		//{"RST 08H", 1, 16, 16},

	instructions[0xD0] = BIND(&ZCPU::Return<ZCPU::JUMP_CONDITION_NOT_CARRY>);		//{"RET NC", 1, 20, 8},
	instructions[0xD1] = BIND(&ZCPU::Pop<REGISTER_DE>);		//{"POP DE", 1, 12, 12},
	instructions[0xD2] = BIND(&ZCPU::Jump<ZCPU::JUMP_CONDITION_NOT_CARRY>);		//{"JP NC,%02X", 3, 16, 12},
	instructions[0xD3] = BIND(&ZCPU::Invalid);		//{"", 0, 0, 0},
	instructions[0xD4] = BIND(&ZCPU::Call<ZCPU::JUMP_CONDITION_NOT_ZERO>);		//{"CALL NC,%02X", 3, 24, 12},
	instructions[0xD5] = BIND(&ZCPU::Push<REGISTER_DE>);		//{"PUSH DE", 1, 16, 16},
	instructions[0xD6] = BIND(&ZCPU::Sub<CONSTANT_8>);		//{"SUB %X", 2, 8, 8},
	instructions[0xD7] = BIND(&ZCPU::Restart<0x10>);		//{"RST 10H", 1, 16, 16},
	instructions[0xD8] = BIND(&ZCPU::Return<ZCPU::JUMP_CONDITION_CARRY>);		//{"RET C", 1, 20, 8},
	instructions[0xD9] = BIND(&ZCPU::ReturnInterrupt);		//{"RETI", 1, 16, 16},
	instructions[0xDA] = BIND(&ZCPU::Jump<ZCPU::JUMP_CONDITION_CARRY>);		//{"JP C,%02X", 3, 16, 12},
	instructions[0xDB] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xDC] = BIND(&ZCPU::Call<ZCPU::JUMP_CONDITION_CARRY>);		//{"CALL C,%02X", 3, 24, 12},
	instructions[0xDD] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xDE] = BIND(&ZCPU::SubWithCarry<CONSTANT_8>);		//{"SBC A,%X", 2, 8, 8},
	instructions[0xDF] = BIND(&ZCPU::Restart<0x18>);		//{"RST 18H", 1, 16, 16},

	instructions[0xE0] = BIND(&ZCPU::LoadStore<POINTER_FIRST_PAGE_CONSTANT, REGISTER_A>);		//{"LDH (%X),A", 2, 12, 12},
	instructions[0xE1] = BIND(&ZCPU::Pop<REGISTER_HL>);		//{"POP HL", 1, 12, 12},
	instructions[0xE2] = BIND(&ZCPU::LoadStore<POINTER_FIRST_PAGE_REGISTER_C, REGISTER_A>);		//{"LD (C),A", 2, 8, 8},
	instructions[0xE3] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xE4] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xE5] = BIND(&ZCPU::Push<REGISTER_HL>);		//{"PUSH HL", 1, 16, 16},
	instructions[0xE6] = BIND(&ZCPU::And<CONSTANT_8>);		//{"AND %X", 2, 8, 8},
	instructions[0xE7] = BIND(&ZCPU::Restart<0x20>);		//{"RST 20H", 1, 16, 16},
	instructions[0xE8] = BIND(&ZCPU::AddSPOffset);		//{"ADD SP,%X", 2, 16, 16},
	instructions[0xE9] = BIND(&ZCPU::JumpAddress);		//{"JP (HL)", 1, 4, 4},
	instructions[0xEA] = BIND(&ZCPU::LoadStore<POINTER_CONSTANT, REGISTER_A>);		//{"LD (%02X),A", 3, 16, 16},
	instructions[0xEB] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xEC] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xED] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xEE] = BIND(&ZCPU::Xor<CONSTANT_8>);		//{"XOR %X", 2, 8, 8},
	instructions[0xEF] = BIND(&ZCPU::Restart<0x28>);		//{"RST 28H", 1, 16, 16},

	instructions[0xF0] = BIND(&ZCPU::LoadStore<REGISTER_A, POINTER_FIRST_PAGE_CONSTANT>);		//{"LDH A,(%X)", 2, 12, 12},
	instructions[0xF1] = BIND(&ZCPU::Pop<REGISTER_AF>);		//{"POP AF", 1, 12, 12},
	instructions[0xF2] = BIND(&ZCPU::LoadStore<REGISTER_A, POINTER_FIRST_PAGE_REGISTER_C>);		//{"LD A,(C)", 2, 8, 8},
	instructions[0xF3] = BIND(&ZCPU::DisableInterrupts);		//{"DI", 1, 4, 4},
	instructions[0xF4] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xF5] = BIND(&ZCPU::Push<REGISTER_AF>);		//{"PUSH AF", 1, 16, 16},
	instructions[0xF6] = BIND(&ZCPU::Or<CONSTANT_8>);		//{"OR %X", 2, 8, 8},
	instructions[0xF7] = BIND(&ZCPU::Restart<0x30>);		//{"RST 30H", 1, 16, 16},
	instructions[0xF8] = BIND(&ZCPU::LoadHLSPOffset);		//{"LD HL,SP+%X", 2, 12, 12},
	instructions[0xF9] = BIND(&ZCPU::LoadSPHL);		//{"LD SP,HL", 1, 8, 8},
	instructions[0xFA] = BIND(&ZCPU::LoadStore<REGISTER_A, POINTER_CONSTANT>);		//{"LD A,(%02X)", 3, 16, 16},
	instructions[0xFB] = BIND(&ZCPU::EnableInterrupts);		//{"EI", 1, 4, 4},
	instructions[0xFC] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xFD] = BIND(&ZCPU::Invalid);		//{ "", 0, 0, 0 },
	instructions[0xFE] = BIND(&ZCPU::Compare<CONSTANT_8>);		//{"CP %X", 2, 8, 8},
	instructions[0xFF] = BIND(&ZCPU::Restart<0x38>);		//{"RST 38H", 1, 16, 16}

	INSTRUCTION_SET1(instructions_ext, 0x00, RotateLeft, false);
	INSTRUCTION_SET1(instructions_ext, 0x08, RotateRight, false);
	INSTRUCTION_SET1(instructions_ext, 0x10, RotateLeft, true);
	INSTRUCTION_SET1(instructions_ext, 0x18, RotateRight, true);
	INSTRUCTION_SET(instructions_ext, 0x20, ShiftLeft);
	INSTRUCTION_SET1(instructions_ext, 0x28, ShiftRight, false);
	INSTRUCTION_SET(instructions_ext, 0x30, Swap);
	INSTRUCTION_SET1(instructions_ext, 0x38, ShiftRight, true);

	INSTRUCTION_SET1(instructions_ext, 0x40, Bit, 0);
	INSTRUCTION_SET1(instructions_ext, 0x48, Bit, 1);
	INSTRUCTION_SET1(instructions_ext, 0x50, Bit, 2);
	INSTRUCTION_SET1(instructions_ext, 0x58, Bit, 3);
	INSTRUCTION_SET1(instructions_ext, 0x60, Bit, 4);
	INSTRUCTION_SET1(instructions_ext, 0x68, Bit, 5);
	INSTRUCTION_SET1(instructions_ext, 0x70, Bit, 6);
	INSTRUCTION_SET1(instructions_ext, 0x78, Bit, 7);

	INSTRUCTION_SET1(instructions_ext, 0x80, Reset, 0);
	INSTRUCTION_SET1(instructions_ext, 0x88, Reset, 1);
	INSTRUCTION_SET1(instructions_ext, 0x90, Reset, 2);
	INSTRUCTION_SET1(instructions_ext, 0x98, Reset, 3);
	INSTRUCTION_SET1(instructions_ext, 0xA0, Reset, 4);
	INSTRUCTION_SET1(instructions_ext, 0xA8, Reset, 5);
	INSTRUCTION_SET1(instructions_ext, 0xB0, Reset, 6);
	INSTRUCTION_SET1(instructions_ext, 0xB8, Reset, 7);

	INSTRUCTION_SET1(instructions_ext, 0xC0, Set, 0);
	INSTRUCTION_SET1(instructions_ext, 0xC8, Set, 1);
	INSTRUCTION_SET1(instructions_ext, 0xD0, Set, 2);
	INSTRUCTION_SET1(instructions_ext, 0xD8, Set, 3);
	INSTRUCTION_SET1(instructions_ext, 0xE0, Set, 4);
	INSTRUCTION_SET1(instructions_ext, 0xE8, Set, 5);
	INSTRUCTION_SET1(instructions_ext, 0xF0, Set, 6);
	INSTRUCTION_SET1(instructions_ext, 0xF8, Set, 7);
	
#undef BIND
#undef INSTRUCTION_SET
#undef INSTRUCTION_SET1
	ResetRegisters();
}

void RegistersWindow(ZEmulator* pEMU)
{
	ZCPU* pCPU = pEMU->GetCPU();
	static int value = 0;
	ImGui::RadioButton("8-bit", &value, 0); ImGui::SameLine();
	ImGui::RadioButton("16-bit", &value, 1);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	ImVec2 button_sz(50, 50);

	auto DrawRegister = [](const char* text, int reg, ImVec2 sz, int bits, bool flags = false)
	{
		sz.x *= bits == 8 ? 1 : 2;
		ImGui::Button(text, sz); ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.5f, 0.5f, 0.5f));
		char s[10];
		if (flags)
		{
			s[0] = (reg & ZCPU::CPU_FLAG_ZERO) == 0 ? '-' : 'Z';
			s[1] = (reg & ZCPU::CPU_FLAG_SUBTRACT) == 0 ? '-' : 'N';
			s[2] = (reg & ZCPU::CPU_FLAG_HALF_CARRY) == 0 ? '-' : 'H';
			s[3] = (reg & ZCPU::CPU_FLAG_CARRY) == 0 ? '-' : 'C';
			s[4] = '\0';
		}
		else
		{
			sprintf(s, bits == 8 ? "0x%02X" : "0x%04X", reg);
		}
		ImGui::Button(s, sz);
		ImGui::PopStyleColor(1);
	};

	const ZCPU::Registers &registers = *pCPU->GetRegisterSnapshot();

	if (value == 0)
	{
		DrawRegister("A", registers.r8.a, button_sz, 8); ImGui::SameLine();
		DrawRegister("F", registers.r8.f, button_sz, 8, true);
		DrawRegister("B", registers.r8.b, button_sz, 8); ImGui::SameLine();
		DrawRegister("C", registers.r8.c, button_sz, 8);
		DrawRegister("D", registers.r8.d, button_sz, 8); ImGui::SameLine();
		DrawRegister("E", registers.r8.e, button_sz, 8);
		DrawRegister("H", registers.r8.h, button_sz, 8); ImGui::SameLine();
		DrawRegister("L", registers.r8.l, button_sz, 8);
	}
	else
	{
		DrawRegister("AF", registers.r16.af, button_sz, 16);
		DrawRegister("BC", registers.r16.bc, button_sz, 16);
		DrawRegister("DE", registers.r16.de, button_sz, 16);
		DrawRegister("HL", registers.r16.hl, button_sz, 16);
	}
	DrawRegister("SP", registers.r16.sp, button_sz, 16);
	DrawRegister("PC", registers.r16.pc, button_sz, 16);
	
	ImGui::Separator();
	ImGui::Text("Interrupts");

	button_sz.x *= 2;
	auto DrawInterrupts = [&button_sz](uint8 interrupts)
	{
		const char* interrupts_s = "JSTLV";
		char result[6];
		for (uint32 n = 0; n < 5; n++)
		{
			uint32 bitmask = 1 << (4 - n); // reverse order
			result[n] = (bitmask & interrupts) != 0 ? interrupts_s[n] : '-';
		}
		result[5] = '\0';
		ImGui::Button(result, button_sz);
	};

	ImGui::Button("IF", button_sz); ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.5f, 0.5f, 0.5f));
	DrawInterrupts(pEMU->GetMemory()->LoadMemory(0xFF0F));
	ImGui::PopStyleColor(1);
	ImGui::Button("IE", button_sz); ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.5f, 0.5f, 0.5f));
	DrawInterrupts(pEMU->GetMemory()->LoadMemory(0xFFFF));
	ImGui::PopStyleColor(1);
	ImGui::Button("IME", button_sz); ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.5f, 0.5f, 0.5f));
	ImGui::Button(pCPU->GetMasterInterruptEnable()? "1" : "0", button_sz);
	ImGui::PopStyleColor(1);

	ImGui::PopStyleVar(1);


}


struct SInterruptCallBack
{
	EInterruptFlags flag;
	InstructionFunc Interrupt;
};

void ZCPU::RunStep()
{
	bool enable_interrupts_request = master_interrupt_enable_request;

	uint8 register_if = memory->LoadMemory(INTERRUPT_REQUEST_FLAG);
	uint8 register_ie = memory->LoadMemory(INTERRUPT_ENABLE_FLAG);

	if (master_interrupt && register_if && register_ie)
	{
		static SInterruptCallBack CallBacks[] =
		{
			{INTERRUPT_FLAG_VSYNC,		std::bind(&ZCPU::Restart<0x40>, this)},
			{INTERRUPT_FLAG_LCD_STAT,	std::bind(&ZCPU::Restart<0x48>, this)},
			{INTERRUPT_FLAG_TIMER,		std::bind(&ZCPU::Restart<0x50>, this)},
			{INTERRUPT_FLAG_SERIAL,		std::bind(&ZCPU::Restart<0x58>, this)},
			{INTERRUPT_FLAG_JOYPAD,		std::bind(&ZCPU::Restart<0x60>, this)}
		};

		uint8 enabled_interrupts = register_if & register_ie;
		for (uint32 i = 0; i < 5; i++)
		{
			if (enabled_interrupts & CallBacks[i].flag)
			{
				// Vblank
				master_interrupt = 0;
				register_if &= ~CallBacks[i].flag; // Disable interrupt
				memory->StoreMemory(INTERRUPT_REQUEST_FLAG, register_if);
				CallBacks[i].Interrupt();
				InstructionWait(5); // Always 5 cycles
				break; // Only one interrupt at a time...
			}
		}
	}

	// Fetch
	uint16 pc = GetProgramCounter();
	uint8 opcodefetch = memory->LoadMemory(pc);
	IncrementPC();
	bool prefix = opcodefetch == 0xCB;
	if (prefix)
	{
		pc = GetProgramCounter();
		opcodefetch = memory->LoadMemory(pc);
		IncrementPC();
	}
	m_instruction_args.opcode = opcodefetch;
	// Decode
	SInstruction instr = GetInstruction(opcodefetch, prefix);
	auto Execute = prefix? instructions_ext[opcodefetch] : instructions[opcodefetch];

	m_instruction_args.cycles = instr.cycles_min;

	// Execute
	Execute();

	InstructionWait(m_instruction_args.cycles);

	if (enable_interrupts_request && master_interrupt_enable_request)
	{
		// We had a pending request (EI) and it was not disabled the instruction after by a DI.
		master_interrupt = 1;
		master_interrupt_enable_request = false;
	}
}

void ZCPU::Run()
{
	while (!m_quit)
	{
		if (m_run)
		{
			uint16 pc = GetProgramCounter();

			if (pc == m_breakpoint)
			{
				Pause();
			}
			else
			{
				RunStep();
			}
		}
	}
}

void ZCPU::Start()
{
	m_run = true;
}

void ZCPU::Quit()
{
	m_quit = true;
}

void ZCPU::Pause()
{
	m_run = false;
}

void ZCPU::InstructionWait(uint8 cycles)
{
	assert(cycles % 4 == 0);
	auto time = std::chrono::high_resolution_clock::now();
	auto end_time = begin_time + std::chrono::microseconds(micros_per_cycle * (cycles >> 2));
	if (end_time > time)
	{
		std::this_thread::sleep_for(end_time - time);
	}
	begin_time = std::chrono::high_resolution_clock::now();
}

void ZCPU::ResetRegisters()
{
	memset(&registers, 0, sizeof(registers));
}

void ZCPU::IncrementPC()
{
	registers.r16.pc++;
}

void ZCPU::SetFlag(Flags f, bool bEnabled)
{
	if (bEnabled)
	{
		*((uint8*)&registers.r8.f) |= f;
	}
	else
	{
		*((uint8*)&registers.r8.f) &= ~f;
	}
}

bool ZCPU::GetFlag(Flags f)
{
	return (registers.r8.f & f) != 0;
}


void ZCPU::LoadSPHL()
{
	registers.r16.sp = registers.r16.hl;
}

void ZCPU::LoadHLSPOffset()
{
	uint8 offset = ParseArgument<uint8>();
	int8 val = offset;
	registers.r16.hl = registers.r16.sp + val;

	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_ZERO, false);
	SetFlag(CPU_FLAG_CARRY, CheckCarryAdd(registers.r16.sp, val, 11));
	SetFlag(CPU_FLAG_HALF_CARRY, CheckCarryAdd(registers.r16.sp, val, 15));
}


void ZCPU::StoreSP()
{
	uint16 address = ParseArgument<uint16>();
	StoreMemory(address, registers.r8.sp & 0xFF);
	StoreMemory(address + 1, registers.r8.sp >> 8);
}



void ZCPU::SetCarry()
{
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	SetFlag(CPU_FLAG_CARRY, true);
}

void ZCPU::ComplementCarry()
{
	uint8 carry = GetCarry();
	SetFlag(CPU_FLAG_SUBTRACT, false);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
	SetFlag(CPU_FLAG_CARRY, ~carry);
}

void ZCPU::Complement()
{
	registers.r8.a = ~registers.r8.a;
	SetFlag(CPU_FLAG_SUBTRACT, true);
	SetFlag(CPU_FLAG_HALF_CARRY, true);
}

void ZCPU::JumpAddress()
{
	registers.r16.pc = registers.r16.hl;
}

void ZCPU::ReturnInterrupt()
{
	EnableInterrupts();
	Return<ZCPU::JUMP_CONDITION_ALWAYS>();
}

void ZCPU::DecimalAdjust()
{
	uint8 correction = 0;
	uint8 value = registers.r8.a;

	bool setFlagC = 0;
	bool bHalfCarry = GetFlag(CPU_FLAG_HALF_CARRY);
	bool bCarry = GetFlag(CPU_FLAG_CARRY);
	bool bSub = GetFlag(CPU_FLAG_SUBTRACT);

	if (bHalfCarry || (!bSub && (value & 0xf) > 9)) {
		correction |= 0x6;
	}

	if (bCarry || (!bSub && value > 0x99)) {
		correction |= 0x60;
		setFlagC = true;
	}

	value += bSub ? -correction : correction;
	value &= 0xff;

	registers.r8.a = value;
	SetFlag(CPU_FLAG_ZERO, value == 0);
	SetFlag(CPU_FLAG_CARRY, setFlagC);
	SetFlag(CPU_FLAG_HALF_CARRY, false);
}

void ZCPU::Nop()
{
}

void ZCPU::Stop()
{
	// Stop has a 00 argument...
	ParseArgument<uint8>();
}

void ZCPU::Halt()
{
}

void ZCPU::EnableInterrupts()
{
	master_interrupt_enable_request = true;
}

void ZCPU::DisableInterrupts()
{
	master_interrupt = 0;
	master_interrupt_enable_request = false; // Disables any pending EI
}

uint8 ZCPU::GetCarry()
{
	return (registers.r8.f >> CPU_FLAG_CARRY_BIT) & 0x1;
}


void ZCPU::ControlGUI()
{
	if (ImGui::Button("Run"))
	{
		Start();
	}
	if (ImGui::Button("Step"))
	{
		RunStep();
	}
	if (ImGui::Button("Pause"))
	{
		Pause();
	}
	if (ImGui::Button("Quit"))
	{
		Quit();
	}
	ImGui::InputInt("Microseconds per cycle", (int*)&micros_per_cycle);
}

void ZCPU::SetBreakPoint(uint16 bp)
{
	m_breakpoint = bp;
}
