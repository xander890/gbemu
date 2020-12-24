#include "memory.h"
#include "instructions.h"
#include "cpu.h"
#include "rom.h"
#include "vram.h"
#include "gui.h"
#include "baserom.h"
#include "emulator.h"
#include "ioregisters.h"
#include <string>
#include <algorithm>
#include "memoryutil.h"

void InstructionsWindow(ZEmulator* pEmu)
{
    ZCPU* pCPU = pEmu->GetCPU();
    IMemoryController* pMemory = pEmu->GetMemory();
    uint16 pc = pCPU->GetProgramCounter();
    int16 pc_start_i = (int16)pCPU->GetProgramCounter();
    uint16 pc_c = pc_start_i < 0? 0 : pc_start_i;

    ImGui::Columns(3, "instructions", true);  // 3-ways, no border
    ImGui::Text("Address");
    ImGui::NextColumn();
    ImGui::Text("Mnem.");
    ImGui::NextColumn();
    ImGui::Text("Bytes");
    ImGui::NextColumn();
    ImGui::Separator();
    static int32 breakpoint = -1;
    for (int n = 0; n < 15; n++)
    {
        SInstruction instr = ParseInstruction(pMemory, pc_c);
        char pcs[24];
        snprintf(pcs, 24, "%s$0x%04X", (pc_c == pc)? ">" : " ", pc_c);
        if (ImGui::Selectable(pcs, breakpoint == pc_c, ImGuiSelectableFlags_SpanAllColumns))
        {
            breakpoint = pc_c;
            if (breakpoint >= 0)
                pCPU->SetBreakPoint(pc_c);
        }        
        ImGui::NextColumn();

        std::string s = PrintInstructionTest(pMemory, pc_c);
        ImGui::Text(s.c_str());

        ImGui::NextColumn();
        for (uint16 i = 0; i < instr.size; i++)
        {
            ImGui::Text("%02X", pMemory->LoadMemory(pc_c + i)); ImGui::SameLine();
        }
        ImGui::NextColumn();
        pc_c += instr.size;
    }
    ImGui::Columns(1);
    ImGui::Separator();

}

void MemoryWindow(ZEmulator* pEmu)
{
    ZMainMemory* pMemory = (ZMainMemory*)pEmu->GetMemory();
    static uint32 address_v = 0x0000;
    uint32 address_sel_v = pEmu->GetCPU()->GetProgramCounter();
    ImGui::InputScalar("Address", ImGuiDataType_U16, &address_v, nullptr, nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
    static bool track = false;
    ImGui::Checkbox("Track memory writes", &track);
    //ImGui::InputScalar("Selected", ImGuiDataType_U16, &address_sel_v, nullptr, nullptr, "%04X", ImGuiInputTextFlags_CharsHexadecimal);

    uint16 address = track ? pMemory->GetLastWrittenAddress() & 0xFF00 : (uint16)address_v;
    uint16 address_sel = track? pMemory->GetLastWrittenAddress() : (uint16)address_sel_v;

    ImGui::Columns(3, "memory", true);  // 3-ways, no border
    ImGui::SetColumnWidth(0, 80.0);
    ImGui::SetColumnWidth(1, 450.0);
    ImGui::SetColumnWidth(2, 170.0);
    ImGui::Text("Address");
    ImGui::NextColumn();
    int l = 0;
    char s[256];
    for (uint32 i = 0; i < 16; i++)
    {
        l += sprintf(s + l, "%02X ", i);
    }
    ImGui::Text(s);
    ImGui::NextColumn();
    ImGui::Text("Readable");
    ImGui::NextColumn();
    ImGui::Separator();
    uint16 base = address & ~(0xF);
    for (uint16 n = 0; n < 30; n++)
    {
        uint16 row = base + (n << 4);
        ImGui::Text("$0x%04X", row);
        ImGui::NextColumn();
        for (uint16 i = 0; i < 16; i++)
        {
            uint16 add = row + i;
            if (add == address_sel)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));
            uint8 value = pMemory->LoadMemory(add);
            sprintf(s, "%02X ", value);
            ImGui::Text(s); ImGui::SameLine(0,0);
            if (add == address_sel)
                ImGui::PopStyleColor();
        }
        ImGui::NextColumn();
        l = 0;
        for (uint16 i = 0; i < 16; i++)
        {
            char val = (char)pMemory->LoadMemory(row + i);
            s[i] = val > 37 && val < 192? val : '.';
        }
        s[16] = '\0';
        ImGui::Text(s);
        ImGui::NextColumn();
    }
}


ZMainMemory::ZMainMemory(ZEmulator* emu)
{
    controllers[MEMORY_AREA_ROM]            = { 0x0000, 0x7FFF, nullptr };

    controllers[MEMORY_AREA_VRAM]           = { 0x8000, 0x9FFF, nullptr };
    controllers[MEMORY_AREA_VRAM].controller_map = new ZVideoRAMController(emu);

    controllers[MEMORY_AREA_EXTERNAL_RAM]   = { 0xA000, 0xBFFF, controllers[MEMORY_AREA_ROM].controller_map };

    controllers[MEMORY_AREA_WRAM]           = { 0xC000, 0xDFFF, nullptr };
    controllers[MEMORY_AREA_WRAM].controller_map = new ZMemoryControllerSimple(controllers[MEMORY_AREA_WRAM].address_start, controllers[MEMORY_AREA_WRAM].address_end);

    controllers[MEMORY_AREA_ECHO]           = { 0xE000, 0xFDFF, nullptr };
    controllers[MEMORY_AREA_ECHO].controller_map = new ZMemoryControllerEcho(controllers[MEMORY_AREA_WRAM].controller_map, 0xC000 - 0xE000);

    controllers[MEMORY_AREA_OAM]            = { 0xFE00, 0xFE9F, nullptr };
    controllers[MEMORY_AREA_OAM].controller_map = new ZMemoryControllerSimple(controllers[MEMORY_AREA_OAM].address_start, controllers[MEMORY_AREA_OAM].address_end);

    controllers[MEMORY_AREA_INVALID]        = { 0xFEA0, 0xFEFF, nullptr };
    controllers[MEMORY_AREA_INVALID].controller_map = new ZMemoryControllerInvalid();

    controllers[MEMORY_AREA_IO_REGISTERS]   = { 0xFF00, 0xFF7F, nullptr };
    controllers[MEMORY_AREA_IO_REGISTERS].controller_map = new ZInputOutputMemoryController(emu);

    controllers[MEMORY_AREA_HRAM]           = { 0xFF80, 0xFFFE, nullptr };
    controllers[MEMORY_AREA_HRAM].controller_map = new ZMemoryControllerSimple(controllers[MEMORY_AREA_HRAM].address_start, controllers[MEMORY_AREA_HRAM].address_end);

    controllers[MEMORY_AREA_INTERRUPT]      = { 0xFFFF, 0xFFFF, nullptr };
    controllers[MEMORY_AREA_INTERRUPT].controller_map = new ZMemoryControllerSimple(controllers[MEMORY_AREA_INTERRUPT].address_start, controllers[MEMORY_AREA_INTERRUPT].address_end);
}

ZMainMemory::~ZMainMemory()
{
    UnloadROM();
    for (uint32 n = 0; n < MEMORY_AREA_COUNT; n++)
    {
        if (controllers[n].controller_map)
        {
            delete controllers[n].controller_map;
            controllers[n].controller_map = nullptr;
        }
    }
}

void ZMainMemory::LoadROM(uint8* rom, uint32 romsize)
{
    assert(controllers[MEMORY_AREA_ROM].controller_map == nullptr);
    assert(controllers[MEMORY_AREA_EXTERNAL_RAM].controller_map == nullptr);
    ZROMController* romcontroller = ZROMController::Create(rom, romsize);
    controllers[MEMORY_AREA_ROM].controller_map = romcontroller;
    controllers[MEMORY_AREA_EXTERNAL_RAM].controller_map = romcontroller;
    map_base_rom = true;
}

void ZMainMemory::UnloadROM()
{
    if (controllers[MEMORY_AREA_ROM].controller_map != nullptr &&
        controllers[MEMORY_AREA_EXTERNAL_RAM].controller_map != nullptr)
    {
        delete controllers[MEMORY_AREA_ROM].controller_map;
        controllers[MEMORY_AREA_ROM].controller_map = nullptr;
        controllers[MEMORY_AREA_EXTERNAL_RAM].controller_map = nullptr;
    }
    map_base_rom = false;
}

uint8 ZMainMemory::LoadMemory(uint16 address)
{
    if (map_base_rom && address < 0x4000)
    {
        if (address < 0x100 || address >= 0x150)
        {
            uint32 size;
            return GetDMGRom(size)[address];
        }
    }
    for (uint32 n = 0; n < MEMORY_AREA_COUNT; n++)
    {
        if (address >= controllers[n].address_start && address <= controllers[n].address_end)
        {
            return controllers[n].controller_map->LoadMemory(address);
        }
    }
    return 0;
}

void ZMainMemory::StoreMemory(uint16 address, uint8 value)
{
    m_nlastwritten = address;
    if (address == 0xFF50)
    {
        map_base_rom = false;
        return;
    }
    for (uint32 n = 0; n < MEMORY_AREA_COUNT; n++)
    {
        if (address >= controllers[n].address_start && address <= controllers[n].address_end)
        {
            controllers[n].controller_map->StoreMemory(address, value);
        }
    }
}

ZROMController* ZMainMemory::GetROM()
{
    return (ZROMController*)controllers[MEMORY_AREA_ROM].controller_map;
}

ZVideoRAMController* ZMainMemory::GetVRAM()
{
    return (ZVideoRAMController*)controllers[MEMORY_AREA_VRAM].controller_map;
}
