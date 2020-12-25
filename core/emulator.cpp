#include "emulator.h"
#include "util.h"
#include "instructions.h"
#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "rom.h"
#include "ioregisters.h"
#include "gui.h"
#include "vram.h"


void ZEmulator::RunTests()
{
}

void ZEmulator::DrawGUI()
{
    static bool show_demo_window = false;
    static bool show_registers = false;
    static bool show_instructions = false;
    static bool show_memory = false;
    static bool show_rom = false;
    static bool show_cpu = false;
    static bool show_vram = false;
    static bool show_input = false;

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
    {
        ImGui::ShowDemoWindow(&show_demo_window);
    }
    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        ImGui::Begin("Emulator");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Registers", &show_registers);
        ImGui::Checkbox("Instructions", &show_instructions);
        ImGui::Checkbox("Memory", &show_memory);
        ImGui::Checkbox("ROM", &show_rom);
        ImGui::Checkbox("CPU", &show_cpu);
        ImGui::Checkbox("Input", &show_input);
        ImGui::Checkbox("VRAM", &show_vram);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_registers)
    {
        ImGui::Begin("Registers", &show_registers, ImGuiWindowFlags_AlwaysAutoResize);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        RegistersWindow(this);
        ImGui::End();
    }

    if (show_instructions)
    {
        ImGui::Begin("Instructions", &show_instructions);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        InstructionsWindow(this);
        ImGui::End();
    }

    if (show_memory)
    {
        ImGui::Begin("Memory", &show_memory);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        MemoryWindow(this);
        ImGui::End();
    }

    if (show_rom)
    {
        ImGui::Begin("ROM", &show_rom);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        ROMWindow(pMemory->GetROM());
        ImGui::End();
    }

    if (show_cpu)
    {
        ImGui::Begin("CPU", &show_cpu);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        pCPU->ControlGUI();
        ImGui::End();
    }

    if (show_input)
    {
        ImGui::Begin("Input", &show_input);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        InputWindow(this);
        ImGui::End();
    }

    if (show_vram)
    {
        ImGui::Begin("VRAM", &show_input);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        GetMemory()->GetVRAM()->DrawGUI();
        ImGui::End();
    } 
}

void ZEmulator::LoadROM(const char* file)
{
    std::vector<uint8> data;
    LoadBinaryData(file, data);
    pMemory->LoadROM(data.data(), (uint32)data.size());
}

ZEmulator::ZEmulator()
{
    pMemory = new ZMainMemory(this);
    pCPU = new ZCPU(pMemory);
    pPPU = new ZPPU(pMemory);
    m_thread = std::thread([this] { pCPU->Run(); });
}

ZEmulator::~ZEmulator()
{
    delete pMemory;
    delete pCPU;
}

 ZMainMemory* ZEmulator::GetMemory() { return pMemory; }

 void ZEmulator::Run() 
 { 
     pCPU->Start();
 }

 void ZEmulator::Quit()
 {
     pCPU->Quit();
     m_thread.join();
 }
