#include "cputests.h"
#include "../core/memoryutil.h"
#include <cassert>
#include <iostream>

uint8 Z = ZCPU::CPU_FLAG_ZERO;
uint8 C = ZCPU::CPU_FLAG_CARRY;
uint8 N = ZCPU::CPU_FLAG_SUBTRACT;
uint8 H = ZCPU::CPU_FLAG_HALF_CARRY;

void ZCPUTester::RunTests()
{
	ZMemoryControllerSimple Mem(0, 0x10000);
	ZCPU CPU(&Mem);

	uint32 test = 0;

	auto BeginTest = [&]() {
		CPU.ResetRegisters();
		Mem.Reset();
		test++;
		std::cout << "Starting test " << test << "...";
	};

	auto EndTest = [&](bool success)
	{
		assert(success);
		std::cout << (success ? " Succeeded." : " Failed.") << std::endl;
	};

	auto TestFlags = [&](uint8 expected, uint8 mask = 0xF0)
	{
		return (CPU.registers.r8.f & mask) == (expected & mask);
	};
	// 8 bit loads
	BeginTest();
	Mem.StoreMemory(0, 0x24);
	CPU.LoadStore<ZCPU::REGISTER_B, ZCPU::CONSTANT_8>();
	EndTest(CPU.registers.r8.b == 0x24);

	BeginTest();
	Mem.StoreMemory(0xFE10, 0x31);
	CPU.registers.r16.hl = 0xFE10;
	CPU.LoadStore<ZCPU::REGISTER_H, ZCPU::POINTER_HL>();
	EndTest(CPU.registers.r8.h == 0x31);

	BeginTest();
	CPU.registers.r8.a = 0x3C;
	CPU.registers.r16.hl = 0x8AC5;
	CPU.LoadStore<ZCPU::POINTER_HL, ZCPU::REGISTER_A>();
	EndTest(Mem.LoadMemory(0x8AC5) == 0x3C);

	BeginTest();
	Mem.StoreMemory(0x7777, 0x36);
	CPU.registers.r16.bc = 0x7777;
	CPU.LoadStore<ZCPU::REGISTER_A, ZCPU::POINTER_BC>();
	EndTest(CPU.registers.r8.a == 0x36);

	BeginTest();
	CPU.registers.r8.c = 0x95;
	Mem.StoreMemory(0xFF95, 0x36);
	CPU.LoadStore<ZCPU::REGISTER_A, ZCPU::POINTER_FIRST_PAGE_REGISTER_C>();
	EndTest(CPU.registers.r8.a == 0x36);

	BeginTest();
	CPU.registers.r8.a = 0x3C;
	CPU.registers.r8.c = 0x95;
	CPU.LoadStore<ZCPU::POINTER_FIRST_PAGE_REGISTER_C, ZCPU::REGISTER_A>();
	EndTest(Mem.LoadMemory(0xFF95) == 0x3C);

	BeginTest();
	Mem.StoreMemory(0x00, 0x95);
	Mem.StoreMemory(0xFF95, 0x36);
	CPU.LoadStore<ZCPU::REGISTER_A, ZCPU::POINTER_FIRST_PAGE_CONSTANT>();
	EndTest(CPU.registers.r8.a == 0x36);

	BeginTest();
	Mem.StoreMemory(0x00, 0x95);
	CPU.registers.r8.a = 0x3C;
	CPU.LoadStore<ZCPU::POINTER_FIRST_PAGE_CONSTANT, ZCPU::REGISTER_A>();
	EndTest(Mem.LoadMemory(0xFF95) == 0x3C);

	BeginTest();
	Mem.StoreMemory(0x00, 0x95);
	Mem.StoreMemory(0x01, 0xFF);
	Mem.StoreMemory(0xFF95, 0x1A);
	CPU.LoadStore<ZCPU::REGISTER_A, ZCPU::POINTER_CONSTANT>();
	EndTest(CPU.registers.r8.a == 0x1A);

	BeginTest();
	Mem.StoreMemory(0x00, 0x95);
	Mem.StoreMemory(0x01, 0xFF);
	CPU.registers.r8.a = 0x1A;
	CPU.LoadStore<ZCPU::POINTER_CONSTANT, ZCPU::REGISTER_A>();
	EndTest(Mem.LoadMemory(0xFF95) == 0x1A);

	BeginTest();
	CPU.registers.r16.hl = 0xffff;
	CPU.registers.r8.a = 0x3C;
	CPU.LoadStoreIncrement<ZCPU::POINTER_HL, ZCPU::REGISTER_A>();
	EndTest(Mem.LoadMemory(0xFFFF) == 0x3C && CPU.registers.r16.hl == 0x0000);

	BeginTest();
	CPU.registers.r16.hl = 0x4000;
	CPU.registers.r8.a = 0x05;
	CPU.LoadStoreDecrement<ZCPU::POINTER_HL, ZCPU::REGISTER_A>();
	EndTest(Mem.LoadMemory(0x4000) == 0x05 && CPU.registers.r16.hl == 0x3FFF);

	// 16-bit
	BeginTest();
	Mem.StoreMemory(0x00, 0x5B);
	Mem.StoreMemory(0x01, 0x3A);
	CPU.LoadConstant<ZCPU::REGISTER_BC>();
	EndTest(CPU.registers.r16.bc == 0x3A5B);

	BeginTest();
	CPU.registers.r8.b = 0x1A;
	CPU.registers.r8.c = 0x3E;
	CPU.registers.r16.sp = 0xFFFE;
	CPU.Push<ZCPU::REGISTER_BC>();
	EndTest(Mem.LoadMemory(0xFFFD) == 0x1A &&
		Mem.LoadMemory(0xFFFC) == 0x3E &&
		CPU.registers.r16.sp == 0xFFFC);

	BeginTest();
	Mem.StoreMemory(0xFFFD, 0x3C);
	Mem.StoreMemory(0xFFFC, 0x5F);
	CPU.registers.r16.sp = 0xFFFC;
	CPU.Pop<ZCPU::REGISTER_BC>();
	EndTest(CPU.registers.r8.b == 0x3C &&
	CPU.registers.r8.c == 0x5F &&
	CPU.registers.r16.sp == 0xFFFE);

	BeginTest();
	int8 v = 0x02;
	Mem.StoreMemory(0x00, v);
	CPU.registers.r16.sp = 0xFFF8;
	CPU.registers.r8.f = (ZCPU::Flags)0xF0;
	CPU.LoadHLSPOffset();
	EndTest(CPU.registers.r8.f == 0x00 &&
		CPU.registers.r16.hl == 0xFFFA);

	// Arithmetic 8-bit
	//ADD
	BeginTest();
	CPU.registers.r8.a = 0x3A;
	CPU.registers.r8.b = 0xC6;
	CPU.Add<ZCPU::REGISTER_B>();
	EndTest(CPU.registers.r8.a == 0x00 && TestFlags(Z | C | H));

	BeginTest();
	CPU.registers.r8.a = 0x3C;
	Mem.StoreMemory(0x00, 0xFF);
	CPU.Add<ZCPU::CONSTANT_8>();
	EndTest(CPU.registers.r8.a == 0x3B && TestFlags(C | H));

	BeginTest();
	CPU.registers.r8.a = 0x3C;
	Mem.StoreMemory(0x01, 0x12);
	CPU.registers.r16.hl = 0x01;
	CPU.Add<ZCPU::POINTER_HL>();
	EndTest(CPU.registers.r8.a == 0x4E && TestFlags(0));

	//ADC
	BeginTest();
	CPU.registers.r8.a = 0xE1;
	CPU.registers.r8.e = 0x0F;
	CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.AddWithCarry<ZCPU::REGISTER_E>();
	EndTest(CPU.registers.r8.a == 0xF1 && TestFlags(H));

	BeginTest();
	CPU.registers.r8.a = 0xE1;
	CPU.registers.r8.f = (ZCPU::Flags)C;
	Mem.StoreMemory(0x00, 0x3B);
	CPU.AddWithCarry<ZCPU::CONSTANT_8>();
	EndTest(CPU.registers.r8.a == 0x1D && TestFlags(C));

	BeginTest();
	CPU.registers.r8.a = 0xE1;
	Mem.StoreMemory(0x01, 0x1E);
	CPU.registers.r16.hl = 0x01;
	CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.AddWithCarry<ZCPU::POINTER_HL>();
	EndTest(CPU.registers.r8.a == 0x00 && TestFlags(Z | H | C));

	//SUB
	BeginTest();
	CPU.registers.r8.a = 0x3E;
	CPU.registers.r8.e = 0x3E;
	CPU.Sub<ZCPU::REGISTER_E>();
	EndTest(CPU.registers.r8.a == 0x00 && TestFlags(Z | N));

	BeginTest();
	CPU.registers.r8.a = 0x3E;
	Mem.StoreMemory(0x00, 0x0F);
	CPU.Sub<ZCPU::CONSTANT_8>();
	EndTest(CPU.registers.r8.a == 0x2F && TestFlags(H | N));

	BeginTest();
	CPU.registers.r8.a = 0x3E;
	Mem.StoreMemory(0x01, 0x40);
	CPU.registers.r16.hl = 0x01;
	CPU.Sub<ZCPU::POINTER_HL>();
	EndTest(CPU.registers.r8.a == 0xFE && TestFlags(N | C));

	//SBC
	BeginTest();
	CPU.registers.r8.a = 0x3B;
	CPU.registers.r8.h = 0x2A;
	CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.SubWithCarry<ZCPU::REGISTER_H>();
	EndTest(CPU.registers.r8.a == 0x10 && TestFlags(N));

	BeginTest();
	CPU.registers.r8.a = 0x3B;
	CPU.registers.r8.f = (ZCPU::Flags)C;
	Mem.StoreMemory(0x00, 0x3A);
	CPU.SubWithCarry<ZCPU::CONSTANT_8>();
	EndTest(CPU.registers.r8.a == 0x00 && TestFlags(Z | N));

	BeginTest();
	CPU.registers.r8.a = 0x3B;
	Mem.StoreMemory(0x01, 0x4F);
	CPU.registers.r16.hl = 0x01;
	CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.SubWithCarry<ZCPU::POINTER_HL>();
	EndTest(CPU.registers.r8.a == 0xEB && TestFlags(H | N | C));

	//AND
	BeginTest();
	CPU.registers.r8.a = 0x5A;
	CPU.registers.r8.l = 0x3F;
	CPU.And<ZCPU::REGISTER_L>();
	EndTest(CPU.registers.r8.a == 0x1A && TestFlags(H));

	BeginTest();
	CPU.registers.r8.a = 0x5A;
	Mem.StoreMemory(0x00, 0x38);
	CPU.And<ZCPU::CONSTANT_8>();
	EndTest(CPU.registers.r8.a == 0x18 && TestFlags(H));

	BeginTest();
	CPU.registers.r8.a = 0x5A;
	Mem.StoreMemory(0x01, 0x00);
	CPU.registers.r16.hl = 0x01;
	CPU.And<ZCPU::POINTER_HL>();
	EndTest(CPU.registers.r8.a == 0x00 && TestFlags(Z | H));

	//OR
	BeginTest();
	CPU.registers.r8.a = 0x5A;
	CPU.Or<ZCPU::REGISTER_A>();
	EndTest(CPU.registers.r8.a == 0x5A && TestFlags(0, Z));

	BeginTest();
	CPU.registers.r8.a = 0x5A;
	Mem.StoreMemory(0x00, 0x3);
	CPU.Or<ZCPU::CONSTANT_8>();
	EndTest(CPU.registers.r8.a == 0x5B && TestFlags(0, Z));

	BeginTest();
	CPU.registers.r8.a = 0x5A;
	Mem.StoreMemory(0x01, 0x0F);
	CPU.registers.r16.hl = 0x01;
	CPU.Or<ZCPU::POINTER_HL>();
	EndTest(CPU.registers.r8.a == 0x5F && TestFlags(0,Z));

	//XOR
	BeginTest();
	CPU.registers.r8.a = 0xFF;
	CPU.Xor<ZCPU::REGISTER_A>();
	EndTest(CPU.registers.r8.a == 0x00 && TestFlags(Z, Z));

	BeginTest();
	CPU.registers.r8.a = 0xFF;
	Mem.StoreMemory(0x00, 0x0F);
	CPU.Xor<ZCPU::CONSTANT_8>();
	EndTest(CPU.registers.r8.a == 0xF0 && TestFlags(0, Z));

	BeginTest();
	CPU.registers.r8.a = 0xFF;
	Mem.StoreMemory(0x01, 0x8A);
	CPU.registers.r16.hl = 0x01;
	CPU.Xor<ZCPU::POINTER_HL>();
	EndTest(CPU.registers.r8.a == 0x75 && TestFlags(0, Z));

	//CP
	BeginTest();
	CPU.registers.r8.a = 0x3C;
	CPU.registers.r8.b = 0x2F;
	CPU.Compare<ZCPU::REGISTER_B>();
	EndTest(TestFlags(H | N));

	BeginTest();
	CPU.registers.r8.a = 0x3C;
	Mem.StoreMemory(0x00, 0x3C);
	CPU.Compare<ZCPU::CONSTANT_8>();
	EndTest(TestFlags(Z | N));

	BeginTest();
	CPU.registers.r8.a = 0x3C;
	Mem.StoreMemory(0x01, 0x40);
	CPU.registers.r16.hl = 0x01;
	CPU.Compare<ZCPU::POINTER_HL>();
	EndTest(TestFlags(N | C));

	//INC
	BeginTest();
	CPU.registers.r8.a = 0xFF;
	CPU.Increment<ZCPU::REGISTER_A>();
	EndTest(CPU.registers.r8.a == 0x00 && TestFlags(Z | H));

	// DEC
	BeginTest();
	CPU.registers.r8.h = 0x01;
	CPU.Decrement<ZCPU::REGISTER_H>();
	EndTest(CPU.registers.r8.h == 0x00 && TestFlags(Z | N));

	//16-bit arithmetic
	BeginTest();
	CPU.registers.r16.hl = 0x8A23;
	CPU.registers.r16.bc = 0x0605;
	CPU.Add16<ZCPU::REGISTER_HL, ZCPU::REGISTER_BC>();
	EndTest(CPU.registers.r16.hl == 0x9028 && TestFlags(H));

	BeginTest();
	CPU.registers.r16.hl = 0x8A23;
	CPU.Add16<ZCPU::REGISTER_HL, ZCPU::REGISTER_HL>();
	EndTest(CPU.registers.r16.hl == 0x1446 && TestFlags(H | C));

	BeginTest();
	int8 vv = 0x02;
	Mem.StoreMemory(0x00, vv);
	CPU.registers.r16.sp = 0xFFF8;
	CPU.registers.r8.f = (ZCPU::Flags)0xF0;
	CPU.AddSPOffset();
	EndTest(CPU.registers.r8.f == 0x00 &&
		CPU.registers.r16.sp == 0xFFFA);

	// Rotate shift A
	BeginTest();
	CPU.registers.r8.a = 0x85;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.RotateLeftA<false>();
	EndTest(CPU.registers.r8.a == 0x0B && TestFlags(C));

	BeginTest();
	CPU.registers.r8.a = 0x95;
	CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.RotateLeftA<true>();
	EndTest(CPU.registers.r8.a == 0x2B && TestFlags(C));

	BeginTest();
	CPU.registers.r8.a = 0x3B;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.RotateRightA<false>();
	EndTest(CPU.registers.r8.a == 0x9D && TestFlags(C));

	BeginTest();
	CPU.registers.r8.a = 0x81;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.RotateRightA<true>();
	EndTest(CPU.registers.r8.a == 0x40 && TestFlags(C));

	BeginTest();
	CPU.registers.r8.b = 0x85;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.RotateLeft<false, ZCPU::REGISTER_B>();
	EndTest(CPU.registers.r8.b == 0x0B && TestFlags(C));

	BeginTest();
	CPU.registers.r8.l = 0x80;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.RotateLeft<true, ZCPU::REGISTER_L>();
	EndTest(CPU.registers.r8.l == 0x0 && TestFlags(C | Z));

	BeginTest();
	CPU.registers.r8.c = 0x1;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.RotateRight<false, ZCPU::REGISTER_C>();
	EndTest(CPU.registers.r8.c == 0x80 && TestFlags(C));

	BeginTest();
	CPU.registers.r8.a = 0x01;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.RotateRight<true, ZCPU::REGISTER_A>();
	EndTest(CPU.registers.r8.a == 0x0 && TestFlags(C | Z));

	BeginTest();
	CPU.registers.r8.d = 0x80;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.ShiftLeft<ZCPU::REGISTER_D>();
	EndTest(CPU.registers.r8.d == 0x0 && TestFlags(C | Z));

	BeginTest();
	CPU.registers.r8.a = 0x8A;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.ShiftRight<false, ZCPU::REGISTER_A>();
	EndTest(CPU.registers.r8.a == 0xC5 && TestFlags(0));

	BeginTest();
	CPU.registers.r8.a = 0x01;
	//CPU.registers.r8.f = (ZCPU::Flags)C;
	CPU.ShiftRight<true, ZCPU::REGISTER_A>();
	EndTest(CPU.registers.r8.a == 0x00 && TestFlags(C | Z));

	// Bit operations
	BeginTest();
	CPU.registers.r8.a = 0xEF;
	CPU.Bit<4, ZCPU::REGISTER_A>();
	EndTest(TestFlags(H | Z));

	BeginTest();
	CPU.registers.r8.a = 0xEF;
	CPU.Bit<4, ZCPU::REGISTER_A>();
	EndTest(TestFlags(H | Z));

	BeginTest();
	CPU.registers.r8.a = 0x80;
	CPU.Set<3, ZCPU::REGISTER_A>();
	EndTest(CPU.registers.r8.a == 0x88);

	BeginTest();
	CPU.registers.r8.a = 0x80;
	CPU.Reset<7, ZCPU::REGISTER_A>();
	EndTest(CPU.registers.r8.a == 0x00);
	
	// Misc
	BeginTest();
	CPU.registers.r8.a = 0x45;
	CPU.registers.r8.b = 0x38;
	CPU.Add<ZCPU::REGISTER_B>();
	CPU.DecimalAdjust();
	EndTest(CPU.registers.r8.a == 0x83 && TestFlags(0));

	BeginTest();
	CPU.registers.r8.a = 0x45;
	CPU.registers.r8.b = 0x38;
	CPU.Add<ZCPU::REGISTER_B>();
	CPU.DecimalAdjust();
	CPU.Sub<ZCPU::REGISTER_B>();
	CPU.DecimalAdjust();
	EndTest(CPU.registers.r8.a == 0x45 && TestFlags(N));


	BeginTest();
	CPU.registers.r8.a = 0x35;
	CPU.Complement();
	EndTest(CPU.registers.r8.a == 0xCA);
}
