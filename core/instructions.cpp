#include "instructions.h"
#include "memory.h"
#include <sstream>
#include <iostream>
#include <cassert>

std::string PrintInstructionTest(IMemoryController* pMemory, uint16 pc)
{
	SInstruction instr = ParseInstruction(pMemory, pc);
	std::stringstream ss;
	if (instr.size > 1)
	{
		char dest[128];
		char s[8];
		uint32 argssize = instr.size - 1;
		uint8 v1 = pMemory->LoadMemory(pc + 1);
		if (instr.argtype == IMMEDIATE_16)
		{
			assert(argssize == 2);
			uint8 v2 = pMemory->LoadMemory(pc + 2);
			sprintf_s(s, "$%04X", v1 | (v2 << 8));
		}
		else if (instr.argtype == IMMEDIATE_8)
		{
			assert(argssize == 1);
			sprintf_s(s, "$%X", *((uint8*)&v1));
		}
		else if (instr.argtype == SIGNED8)
		{
			assert(argssize == 1);
			int16 val = *((int8*)&v1);
			sprintf_s(s, "%d", val);
		}
		else if (instr.argtype == SIGNED8_JR)
		{
			assert(argssize == 1);
			int16 val = *((int8*)&v1);
			sprintf_s(s, "$%04X", pc + instr.size + val); 
		}
		sprintf_s(dest, instr.debugFormatString, s);
		ss << dest;
	}
	else
	{
		ss << instr.debugFormatString;
	}
	return ss.str();
}

void ParseInstructionIndex(IMemoryController* pMemory, uint16 pc, uint8& opcode, bool& isSecondary)
{
	uint8 instruction = pMemory->LoadMemory(pc);
	isSecondary = instruction == 0xCB;
	opcode = isSecondary ? pMemory->LoadMemory(pc + 1) : instruction;
}

const SInstruction& GetInstruction(uint8 opcode, bool isSecondary)
{
	return isSecondary ? g_InstructionsSecondary[opcode] : g_Instructions[opcode];
}

const SInstruction& ParseInstruction(IMemoryController * pMemory, uint16 pc)
{
	uint8 instruction;
	bool prefix;
	ParseInstructionIndex(pMemory, pc, instruction, prefix);
	return GetInstruction(instruction, prefix);
}
