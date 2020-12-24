#pragma once
#include "types.h"
#include "instructions.h"
#include "memory.h"
#include <functional>
#include <chrono>

enum EInterruptFlags : uint8
{
	INTERRUPT_FLAG_VSYNC =		1 << 0,
	INTERRUPT_FLAG_LCD_STAT	=	1 << 1,
	INTERRUPT_FLAG_TIMER =		1 << 2,
	INTERRUPT_FLAG_SERIAL =		1 << 3,
	INTERRUPT_FLAG_JOYPAD =		1 << 4,
};

struct SIntructionArguments
{
	uint8 opcode;
	uint8 cycles;
};

typedef std::function<void ()> InstructionFunc;

void RegistersWindow(ZEmulator* pCPU);

class ZCPU
{
public:
	ZCPU(IMemoryController* memory);
	void ControlGUI();
	void SetBreakPoint(uint16 bp);
	bool GetMasterInterruptEnable() { return master_interrupt; }

	enum 
	{
		CPU_FLAG_ZERO_BIT = 7,
		CPU_FLAG_SUBTRACT_BIT = 6,
		CPU_FLAG_HALF_CARRY_BIT = 5,
		CPU_FLAG_CARRY_BIT = 4
	};

	enum Flags : uint8
	{
		CPU_FLAG_ZERO		= 0x80,
		CPU_FLAG_SUBTRACT	= 0x40,
		CPU_FLAG_HALF_CARRY	= 0x20,
		CPU_FLAG_CARRY		= 0x10
	};

	enum Argument16
	{
		REGISTER_AF = 0,
		REGISTER_BC = 1,
		REGISTER_DE = 2,
		REGISTER_HL = 3,
		REGISTER_SP = 4,
		REGISTER_PC = 5,
		CONSTANT_16 = 6,
	};

	enum Argument8
	{
		REGISTER_A = 1,
		REGISTER_C = 2,
		REGISTER_B = 3,
		REGISTER_E = 4,
		REGISTER_D = 5,
		REGISTER_L = 6,
		REGISTER_H = 7,
		CONSTANT_8 = 8,
		POINTER_CONSTANT = 9,
		POINTER_FIRST_PAGE_CONSTANT = 10,
		POINTER_FIRST_PAGE_REGISTER_C = 11,
		POINTER_ENUM_OFFSET = 11, // To convert to space 16
		POINTER_BC = POINTER_ENUM_OFFSET + REGISTER_BC,
		POINTER_DE = POINTER_ENUM_OFFSET + REGISTER_DE,
		POINTER_HL = POINTER_ENUM_OFFSET + REGISTER_HL, // shorthand for math instruction
	};



	enum JumpCondition
	{
		JUMP_CONDITION_ALWAYS = 0,
		JUMP_CONDITION_CARRY = 1,
		JUMP_CONDITION_NOT_CARRY = 2,
		JUMP_CONDITION_ZERO = 3,
		JUMP_CONDITION_NOT_ZERO = 4
	};

	struct Registers8
	{
		Flags f;
		uint8 a;
		uint8 c;
		uint8 b;
		uint8 e;
		uint8 d;
		uint8 l;
		uint8 h;
		uint16 sp;
		uint16 pc;
	};

	struct Registers16
	{
		uint16 af;
		uint16 bc;
		uint16 de;
		uint16 hl;
		uint16 sp;
		uint16 pc;
	};

	union Registers
	{
		Registers8 r8;
		Registers16 r16;
		uint8 raw8[12];
		uint16 raw16[6];
	};

	const Registers* GetRegisterSnapshot() { return &registers; };
	uint16 GetProgramCounter() { return registers.r16.pc; }

	void RunStep();
	void Run();
	void Start();
	void Quit();
	void Pause();
	void InstructionWait(uint8 cycles);
	uint32 micros_per_cycle = 1;
	std::chrono::high_resolution_clock::time_point begin_time;
	bool m_run = false;
	bool m_quit = false;

private:

	void ResetRegisters();
	Registers registers;
	uint8 master_interrupt = 0;
	bool master_interrupt_enable_request = false;

	SIntructionArguments m_instruction_args;

	static_assert(sizeof(Registers8) == sizeof(Registers), "error in registers");

	InstructionFunc instructions[0x100];
	InstructionFunc instructions_ext[0x100];
	IMemoryController* memory;
	int32 m_breakpoint = -1;
	// Memory
	uint8 LoadMemory(uint16 address) { return memory->LoadMemory(address); }
	void StoreMemory(uint16 address, uint8 value) { memory->StoreMemory(address, value); }

	template<Argument8 offset>
	uint8 LoadOffset();

	template<Argument8 offset>
	void StoreOffset(uint8 value);

	template<typename T>
	T ParseArgument();

	template<>
	uint8 ParseArgument();

	template<>
	uint16 ParseArgument();

	void IncrementPC();

	void SetFlag(Flags f, bool bEnabled);
	bool GetFlag(Flags f);

	template<JumpCondition condition>
	bool EvaluateCondition();

	// Instructions

	// 8-bit loads
	template<Argument8 storeoffset, Argument8 loadoffset>
	void LoadStore();
	template<Argument8 storeoffset, Argument8 loadoffset>
	void LoadStoreIncrement();
	template<Argument8 storeoffset, Argument8 loadoffset>
	void LoadStoreDecrement();

	// 16-bit loads
	template<Argument16 offset>
	void LoadConstant();
	void LoadSPHL();
	void LoadHLSPOffset();
	void StoreSP();
	template<Argument16 offset16>
	void Push();
	template<Argument16 offset16>
	void Pop();

	// Arithmetic 8
	template<Argument8 offset>
	void Increment();
	template<Argument8 offset>
	void Decrement();
	template<Argument8 offset>
	void Add();
	template<Argument8 offset>
	void AddWithCarry();
	template<Argument8 offset>
	void Sub();
	template<Argument8 offset>
	void SubWithCarry();
	template<Argument8 offset>
	void And();
	template<Argument8 offset>
	void Or();
	template<Argument8 offset>
	void Xor();
	template<Argument8 offset>
	void Compare();

	// Arithmetic 16-bit
	template<int offsetfirst, int offsetsecond>
	void Add16();
	void AddSPOffset();
	template<int offset>
	void Increment16();
	template<int offset>
	void Decrement16();

	// Rotates/shift 8-bit
	template<bool withcarry>
	void RotateLeftA(); // RLA withcarry RCLA nocarry
	template<bool withcarry>
	void RotateRightA();
	template<bool withcarry, Argument8 offset>
	void RotateLeft();
	template<bool withcarry, Argument8 offset>
	void RotateRight();
	template<Argument8 offset>
	void ShiftLeft();
	template<bool zeromsb, Argument8 offset>
	void ShiftRight();
	template<Argument8 offset>
	void Swap();

	// Bit manipulation
	template<int bit, Argument8 offset>
	void Bit();
	template<int bit, Argument8 offset>
	void Set();
	template<int bit, Argument8 offset>
	void Reset();

	// Jumps
	template<JumpCondition condition>
	void Jump();
	template<JumpCondition condition>
	void JumpIncrement();
	template<JumpCondition condition>
	void Call();
	template<JumpCondition condition>
	void Return();
	template<uint8 addresslowbits>
	void Restart();

	void JumpAddress();
	void ReturnInterrupt();

	// Misc
	void SetCarry();
	void ComplementCarry();
	void Complement();
	void DecimalAdjust();
	void Nop();
	void Stop();
	void Halt();
	void EnableInterrupts();
	void DisableInterrupts();
	void PrefixCB() {}
	void Invalid() {}

	uint8 GetCarry();

	friend class ZCPUTester;
};



