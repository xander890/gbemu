#include "ioregisters.h"
#include "emulator.h"
#include "gui.h"
#include "ppu.h"
#include "vram.h"
#include "memory.h"
#include <stdio.h>

uint8 ZInputOutputMemoryController::LoadMemory(uint16 address)
{
	uint8 io_addr = address & 0xFF;
	assert(io_addr < 0x80);
	return m_input_memory[io_addr];
}

void ZInputOutputMemoryController::StoreMemory(uint16 address, uint8 value)
{
	uint16 io_addr = address & 0xFF;
	assert(io_addr < 0x80);

	if (address == JOYPAD_REGISTER)
	{
		// TODO input
		bool is_direction = (value & 0x10) != 0;
		bool is_buttons = (value & 0x20) != 0;
		if (is_direction)
		{
			m_input_memory[io_addr] = value | (m_triggered_inputs & 0xF);
		}
		if (is_buttons)
		{
			m_input_memory[io_addr] =  value | (m_triggered_inputs >> 4);
		}
	}
	else
	{
		m_input_memory[io_addr] = value;
	}
}

void Cells(std::initializer_list<const char*> list)
{
	for (auto elem : list)
	{
		ImGui::Text(elem);
		ImGui::NextColumn();
	}
}

void InputWindow(ZEmulator* pEMU)
{
	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("General"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
			ImVec2 button_sz(50, 50);

			struct SRegisterInfo
			{
				const char* debug_name;
				ImU32 color;
			};

			ImU32 color_base = IM_COL32(128, 128, 128, 255);
			ImU32 color_joypad = IM_COL32(51, 204, 51, 255);
			ImU32 color_sound = IM_COL32(194, 116, 43, 255);
			ImU32 color_soundw = IM_COL32(148, 67, 33, 255);
			ImU32 color_serial = IM_COL32(255, 210, 61, 255);
			ImU32 color_timer = IM_COL32(204, 51, 51, 255);
			ImU32 color_video = IM_COL32(20, 94, 255, 255);
			ImU32 color_gcb = IM_COL32(100, 38, 163, 255);

			static SRegisterInfo infos[0x80];
			static bool init = false;

			if (!init)
			{
				init = true;
				for (uint16 n = 0; n < 0x80; n++)
				{
					infos[n].debug_name = "-";
					infos[n].color = color_base;
				}

				infos[JOYPAD_REGISTER & 0xFF] = { "P1", color_joypad };

				infos[SERIAL_BYTE_REGISTER & 0xFF] = { "SB", color_serial };
				infos[SERIAL_CONTROL_REGISTER & 0xFF] = { "SC", color_serial };

				// Timer
				infos[TIMER_DIVIDER_REGISTER & 0xFF] = { "DIV", color_timer };
				infos[TIMER_COUNTER_REGISTER & 0xFF] = { "TIM", color_timer };
				infos[TIMER_MODULO_REGISTER & 0xFF] = { "TMA", color_timer };
				infos[TIMER_CONTROL_REGISTER & 0xFF] = { "TAC", color_timer };

				// Video
				infos[VIDEO_REGISTER_LCD_CONTROL & 0xFF] = { "LCD", color_video };
				infos[VIDEO_REGISTER_LCD_STATUS & 0xFF] = { "STA", color_video };
				infos[VIDEO_REGISTER_SCROLL_X & 0xFF] = { "SCY", color_video };
				infos[VIDEO_REGISTER_SCROLL_Y & 0xFF] = { "SCX", color_video };
				infos[VIDEO_REGISTER_LCD_LY_LINE & 0xFF] = { "LY", color_video };
				infos[VIDEO_REGISTER_LCD_LY_COMPARE & 0xFF] = { "LYC", color_video };
				infos[VIDEO_REGISTER_DMA_CONTROL & 0xFF] = { "DMA", color_video };
				infos[VIDEO_REGISTER_BACKGROUND_PALETTE_DMG & 0xFF] = { "BGP", color_video };
				infos[VIDEO_REGISTER_OBJ0_PALETTE_DMG & 0xFF] = { "OBP0", color_video };
				infos[VIDEO_REGISTER_OBJ1_PALETTE_DMG & 0xFF] = { "OBP1", color_video };
				infos[VIDEO_REGISTER_WINDOW_X & 0xFF] = { "WX", color_video };
				infos[VIDEO_REGISTER_WINDOW_Y & 0xFF] = { "WY", color_video };

				// CGB Special flags
				infos[CGB_FLAG_SPEED_SWITCH & 0xFF] = { "KEY1", color_gcb };
				infos[CGB_FLAG_VRAM_BANK & 0xFF] = { "VBK", color_gcb };
				infos[CGB_FLAG_INFRARED_PORT & 0xFF] = { "RP", color_gcb };
				infos[CGB_FLAG_HDMA & 0xFF] = { "HDMA", color_gcb };
				
				infos[CGB_FLAG_BACKGROUND_PALETTE_INDEX & 0xFF] = { "BCPS", color_gcb };
				infos[CGB_FLAG_BACKGROUND_PALETTE_DATA & 0xFF] = { "BCPD", color_gcb };
				infos[CGB_FLAG_OBJ_PALETTE_INDEX & 0xFF] = { "OCPS", color_gcb };
				infos[CGB_FLAG_OBJ_PALETTE_DATA & 0xFF] = { "OCPD", color_gcb };
				infos[CGB_FLAG_WRAM_BANK & 0xFF] = { "SVBK", color_gcb };

				infos[DISABLE_BASE_ROM & 0xFF] = { "BOOT", 0 };
				infos[INTERRUPT_REQUEST_FLAG & 0xFF] = { "IF", IM_COL32(61, 229, 255, 255) };

				// Sounds
				uint16 addr = 0xFF10;
				for (uint32 i = 0; i < 5; i++)
				{
					for (uint32 j = 0; j < 5; j++)
					{
						char* s = new char[6];
						int v = (i + 1) * 10 + j;
						sprintf(s, "NR%d", v);
						// Unused
						if (v != 20 && v != 40 && v != 53 && v != 54)
						{
							infos[addr & 0xFF] = { s, color_sound };
						}
						addr++;
					}
				}

				uint16 addrw = 0xFF30;
				for (uint32 i = 0; i < 16; i++)
				{
					char* s = new char[6];
					sprintf(s, "W%d", i);
					// Unused
					infos[addrw & 0xFF] = { s, color_soundw };
					addrw++;
				}

			}

			auto DrawValue = [&](SRegisterInfo& s, uint8 value)
			{
				char ss[256];
				sprintf(ss, "%s\n0x%02X", s.debug_name, value);
				ImGui::PushStyleColor(ImGuiCol_Button, s.color);
				ImGui::Button(ss, button_sz);
				ImGui::PopStyleColor(1);
			};

			for (uint16 n = 0; n < 0x80; n++)
			{
				uint16 address = 0xFF00 | n;
				uint8 value = pEMU->GetMemory()->LoadMemory(address);
				DrawValue(infos[n], value);
				if (((n + 1) % 0x10) != 0)
				{
					ImGui::SameLine();
				}
			}

			ImGui::PopStyleVar(1);

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Joypad"))
		{
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Audio"))
		{
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Video"))
		{
			uint32 default_palette[4] = { 0xffffffff, 0xaaaaaaff, 0x555555ff, 0x000000ff };
			//uint8 status = pEMU->GetMemory()->LoadMemory(VIDEO_REGISTER_LCD_STATUS);
			ImGui::Columns(2);
			ZPPU* ppu = pEMU->GetPPU();
			ZVideoRAMController* vram = pEMU->GetMemory()->GetVRAM();
			Cells({"LCD", ppu->IsLCDEnabled() ? "On" : "Off"});
			Cells({"Window", ppu->IsWindowEnabled() ? "On" : "Off"});
			Cells({"Sprites", ppu->IsSpriteEnabled() ? "On" : "Off"});
			char cc[256];
			sprintf(cc, "8x%d", ppu->GetSpriteSize());
			Cells({"Sprite size", cc});

			uint16 start = vram->GetBackgroundDataStart();
			sprintf(cc, "[0x%04X - 0x%04X]", start, start + BACKGROUND_AND_WINDOW_DATA_SIZE);
			Cells({"Background Data", cc});

			start = vram->GetSpriteDataStart();
			sprintf(cc, "[0x%04X - 0x%04X]", start, start + SPRITE_DATA_SIZE);
			Cells({ "Sprite Data", cc });

			start = vram->GetBackgroundTileStart();
			sprintf(cc, "[0x%04X - 0x%04X]", start, start + BACKGROUND_TILEMAP_SIZE);
			Cells({ "Background Tile Data", cc });

			start = vram->GetWindowTileStart();
			sprintf(cc, "[0x%04X - 0x%04X]", start, start + WINDOW_TILEMAP_SIZE);
			Cells({ "Window Tile Data", cc });

			char ss[50];
			sprintf(ss, "%d x %d", pEMU->GetMemory()->LoadMemory(VIDEO_REGISTER_SCROLL_X), pEMU->GetMemory()->LoadMemory(VIDEO_REGISTER_SCROLL_Y));
			Cells({ "Scrolling", ss });
			sprintf(ss, "%d x %d", pEMU->GetMemory()->LoadMemory(VIDEO_REGISTER_WINDOW_X), pEMU->GetMemory()->LoadMemory(VIDEO_REGISTER_WINDOW_Y));
			Cells({ "Window", ss });
			ImGui::EndTabItem();
		}

	ImGui::EndTabBar();
	}

}
