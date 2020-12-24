#include "rom.h"
#include <cassert>
#include "rom.h"
#include "rom/mbc0.h"
#include "rom/mbc1.h"
#include "rom/mbc2.h"
#include "rom/mbc3.h"
#include "rom/mbc5.h"
#include "gui.h"

ZROMController::ZROMController(uint8* rom, uint32 romsize)
{
	m_rom = new uint8[romsize];
	m_romsize = romsize;
	memcpy(m_rom, rom, romsize);
}

ZROMController::~ZROMController()
{
	delete[] m_rom;
}

ZROMController* ZROMController::Create(uint8* romdata, uint32 romsize)
{
	SCartridgeHeader* header = reinterpret_cast<SCartridgeHeader*>(romdata + 0x100);
	uint32 flags = g_CartridgeDescription[header->cartridgeType];
	if ((flags & MBC_0) != 0)
	{
		return new ZMemoryBankController0(romdata, romsize);
	}
	if ((flags & MBC_1) != 0)
	{
		return new ZMemoryBankController1(romdata, romsize);
	}
	if ((flags & MBC_2) != 0)
	{
		return new ZMemoryBankController2(romdata, romsize);
	}
	if ((flags & MBC_3) != 0)
	{
		return new ZMemoryBankController3(romdata, romsize);
	}
	if ((flags & MBC_5) != 0)
	{
		return new ZMemoryBankController5(romdata, romsize);
	}
	assert(false);
	return nullptr;
}


std::map<uint8, uint32> ZROMController::g_CartridgeDescription =
{
	{(uint8)0x00, MBC_0},
	{(uint8)0x01, MBC_1},
	{(uint8)0x02, MBC_1 | SRAM},
	{(uint8)0x03, MBC_1 | SRAM | BATTERY},
	{(uint8)0x05, MBC_2},
	{(uint8)0x06, MBC_2 | BATTERY},
	{(uint8)0x08, MBC_0 | SRAM},
	{(uint8)0x09, MBC_0 | SRAM | BATTERY},
	{(uint8)0x0F, MBC_3 | RTC | BATTERY},
	{(uint8)0x10, MBC_3 | RTC | SRAM | BATTERY},
	{(uint8)0x11, MBC_3},
	{(uint8)0x12, MBC_3 | SRAM},
	{(uint8)0x13, MBC_3 | SRAM | BATTERY},
	{(uint8)0x19, MBC_5},
	{(uint8)0x1A, MBC_5 | SRAM},
	{(uint8)0x1B, MBC_5 | SRAM | BATTERY},
	{(uint8)0x1C, MBC_5 | RUMBLE},
	{(uint8)0x1D, MBC_5 | RUMBLE | SRAM},
	{(uint8)0x1E, MBC_5 | RUMBLE | SRAM | BATTERY}
};


SCartridgeHeader* ZROMController::GetHeader()
{
	return reinterpret_cast<SCartridgeHeader*>(m_rom + 0x100);
}

uint32 ZROMController::GetROMSize()
{
	uint8 ROMsize = GetHeader()->ROMSize;
	static const uint32 sizeValues[8] =
	{
		32 << 10,
		64 << 10,
		128 << 10,
		256 << 10,
		512 << 10,
		1 << 20,
		2 << 20,
		4 << 20
	};
	return sizeValues[ROMsize];
}

uint32 ZROMController::GetROMFlags()
{
	return g_CartridgeDescription[GetHeader()->cartridgeType];
}

uint32 ZROMController::GetRAMSize()
{
	uint8 RAMsize = GetHeader()->RAMSize;
	static const uint32 sizeValues[4] =
	{
		0 << 10,
		2 << 10,
		8 << 10,
		32 << 10,
	};
	return sizeValues[RAMsize];
}

void ROMWindow(ZROMController* pController)
{
	SCartridgeHeader* pHeader = pController->GetHeader();
	ImGui::Columns(2);
	ImGui::Text("Title");
	ImGui::NextColumn();
	ImGui::Text(pHeader->gameTitle);
	ImGui::NextColumn();

	ImGui::Text("Region");
	ImGui::NextColumn();
	ImGui::Text(pHeader->region == REGION_JAPAN? "Japan" : "World");
	ImGui::NextColumn();

	ImGui::Text("Mode");
	ImGui::NextColumn();
	switch (pHeader->gbcFlag)
	{
	case RUN_MODE_DMG:	ImGui::Text("DMG"); break;
	case RUN_MODE_GBC: ImGui::Text("GBC only"); break; 
	case RUN_MODE_GBC_AND_DMG: ImGui::Text("DMG and CBG"); break;
	}
	ImGui::NextColumn();

	ImGui::Text("ROM Size");
	ImGui::NextColumn();	
	uint64 kbs = pController->GetROMSize() >> 10;
	uint64 mbs = pController->GetROMSize() >> 20;
	ImGui::Text("%d %s", mbs != 0? mbs : kbs, mbs != 0? "MiBs" : "KiBs");
	ImGui::NextColumn();

	ImGui::Text("RAM Size");
	ImGui::NextColumn();
	uint64 kbs2 = pController->GetRAMSize() >> 10;
	uint64 mbs2 = pController->GetRAMSize() >> 20;
	ImGui::Text("%d %s", mbs2 != 0 ? mbs2 : kbs2, mbs2 != 0 ? "MiBs" : "KiBs");
	ImGui::NextColumn();

	ImGui::Text("MBC");
	ImGui::NextColumn();
	switch (pController->GetROMFlags() & 0x1F)
	{
	case MBC_0:	ImGui::Text("MBC_0"); break;
	case MBC_1:	ImGui::Text("MBC_1"); break;
	case MBC_2:	ImGui::Text("MBC_2"); break;
	case MBC_3:	ImGui::Text("MBC_3"); break;
	case MBC_5:	ImGui::Text("MBC_5"); break;
	}
	ImGui::NextColumn();

	ImGui::Text("Battery");
	ImGui::NextColumn();
	ImGui::Text("%s", (pController->GetROMFlags() & BATTERY) == 0? "No" : "Yes");
	ImGui::NextColumn();

	ImGui::Text("SRAM");
	ImGui::NextColumn();
	ImGui::Text("%s", (pController->GetROMFlags() & SRAM) == 0 ? "No" : "Yes");
	ImGui::NextColumn();

	ImGui::Text("Real time clock");
	ImGui::NextColumn();
	ImGui::Text("%s", (pController->GetROMFlags() & RTC) == 0 ? "No" : "Yes");
	ImGui::NextColumn(); 

	ImGui::Columns(1);
}
