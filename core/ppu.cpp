#include "ppu.h"
#include "emulator.h"
#include "memory.h"
#include "vram.h"
#include "ioregisters.h"

ZPPU::ZPPU(ZEmulator* emu)
{
	m_emulator = emu;
	m_vram = emu->GetMemory()->GetVRAM();
}

ZPPU::~ZPPU()
{
}

bool ZPPU::IsLCDEnabled()
{
	uint8 lcd = m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_LCD_CONTROL);
	SLCDConfig* pConfig = (SLCDConfig*)&lcd;
	return pConfig->lcd_enable;
}

bool ZPPU::IsWindowEnabled()
{
	uint8 lcd = m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_LCD_CONTROL);
	SLCDConfig* pConfig = (SLCDConfig*)&lcd;
	return pConfig->window_enable;
}


bool ZPPU::IsSpriteEnabled()
{
	uint8 lcd = m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_LCD_CONTROL);
	SLCDConfig* pConfig = (SLCDConfig*)&lcd;
	return pConfig->sprite_enable;
}

uint8 ZPPU::GetSpriteSize()
{
	uint8 lcd = m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_LCD_CONTROL);
	SLCDConfig* pConfig = (SLCDConfig*)&lcd;
	return pConfig->sprite_size == 0? 8 : 16;
}

void ZPPU::OAMScan(uint8 scanline)
{
	m_oamfound = 0;
	uint8 h = GetSpriteSize();
	for (uint8 i = 0; i < MAX_SPRITES; i++)
	{
		uint16 sprite_address = ADDRESS_OAM_START + i * 4; // 4 bytes per sprite
		uint8 x = m_emulator->GetMemory()->LoadMemory(sprite_address + 1);
		uint8 y = m_emulator->GetMemory()->LoadMemory(sprite_address + 0);
		uint8 translated_ly = scanline + 16;
		bool visible = x != 0 && translated_ly >= y && translated_ly < (y + h);
		if (visible)
		{
			m_oamscan[m_oamfound++] = i;
		}
	}
}

void ZPPU::PixelTransfer(uint8 scanline)
{
	uint8 window_x = m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_WINDOW_X);
	uint8 window_y = m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_WINDOW_Y);
	uint8 scroll_x = m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_SCROLL_X);
	uint8 scroll_y = m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_SCROLL_Y);
	bool can_get_window = scanline >= window_y;
	uint8 translated_ly = scanline + 16;

	m_oam_fifo.clear();
	m_bg_fifo.clear();

	for (uint8 x = 0; x < SCREEN_WIDTH; x++)
	{
		// Sprites!
		if (IsSpriteEnabled())
		{
			for (uint8 k = 0; k < m_oamfound; k++)
			{
				uint16 sprite_address = ADDRESS_OAM_START + m_oamscan[k] * 4; // 4 bytes per sprite
				uint8 x_sprite = m_emulator->GetMemory()->LoadMemory(sprite_address + 1);
				if (x == x_sprite)
				{
					// Load sprite
					// FIXME unsupported 8x16 load
					// FIXME unsupported gameboy color
					uint8 y_sprite = m_emulator->GetMemory()->LoadMemory(sprite_address + 0);
					uint8 sprite_tile = m_emulator->GetMemory()->LoadMemory(sprite_address + 2);
					uint8 sprite_flags = m_emulator->GetMemory()->LoadMemory(sprite_address + 3);

					bool flip_x = (sprite_flags >> 5) & 1;
					bool flip_y = (sprite_flags >> 6) & 1;
					bool bg_prio = (sprite_flags >> 7) & 1;
					uint8 sprite_palette = (sprite_flags >> 4) & 1;

					uint8 sprite_delta_y = translated_ly - y_sprite;
					sprite_delta_y = flip_y ? 7 - sprite_delta_y : sprite_delta_y;
					// First row of sprite
					uint16 sprite_vram_address = m_vram->GetSpriteDataStart() + sprite_tile * 16;
					uint16 sprite_vram_row = sprite_vram_address + sprite_delta_y;

					uint8 sprite_low = m_emulator->GetMemory()->LoadMemory(sprite_vram_row);
					uint8 sprite_high = m_emulator->GetMemory()->LoadMemory(sprite_vram_row + 1);
					SPixel p = {};

					uint8 existing_pixels = (uint8)m_oam_fifo.size();
					for (uint8 i = 0; i < 8; i++)
					{
						// Extract pixel
						SPixel current;
						uint8 msb = 7 - i;
						uint8 lsb = i;

						uint8 bit = flip_x ? lsb : msb;
						current.color = ((sprite_low >> bit) & 0x1) | (((sprite_high >> bit) & 0x1) << 1);
						current.palette = sprite_palette;
						current.bg_prio = bg_prio;
						current.sprite_prio = 0;

						if (i < existing_pixels)
						{
							SPixel* to_merge = &m_oam_fifo[i];
							if (to_merge->color == 0x0 || to_merge->sprite_prio > current.sprite_prio)
							{
								*to_merge = current;
							}
						}
						else
						{
							m_oam_fifo.push_back(current);
						}
					}

				}
			}
		}

		uint16 fetch_x = scroll_x + x; // this can overflow
		uint8 fetch_y = ((scroll_y + scanline) & 0xff);
		uint16 tile_start_address = m_vram->GetBackgroundTileStart();

		if (IsWindowEnabled() && can_get_window)
		{
			uint8 wx = window_x - 7;
			uint8 wy = window_y;

			if (x == wx)
			{
				m_bg_fifo.clear(); // Discard current pixels
			}

			if (x >= wx && m_bg_fifo.empty())
			{
				// Use window
				fetch_x = wx - x;
				fetch_y = wy - scanline;
				tile_start_address = m_vram->GetWindowTileStart();
			}
		}

		if (m_bg_fifo.empty())
		{
			// Background!
			uint8 fetch_tile_x = (fetch_x / 8) & 0x1f;
			uint8 fetch_tile_y = fetch_y / 8;
			// 32x32 tiles of one byte each
			uint16 tile = fetch_tile_y * 32 + fetch_tile_x;
			uint16 tile_address = tile_start_address + tile;
			uint8 tile_number = m_vram->LoadMemory(tile_address);

			uint16 fetch_address = 0;
			uint16 tile_data_start_address = m_vram->GetBackgroundDataStart();
			if (tile_data_start_address == BACKGROUND_AND_WINDOW_DATA_START_0)
			{
				// Unsigned int fetch
				fetch_address = tile_data_start_address + tile_number * 16;
			}
			else
			{
				fetch_address = tile_data_start_address + *reinterpret_cast<int8*>(&tile_number) * 16;
			}

			// Calculate sprite
			uint8 tile_y_inner = fetch_y % 8;
			fetch_address += tile_y_inner * 2;

			uint8 data_low = m_emulator->GetMemory()->LoadMemory(fetch_address);
			uint8 data_high = m_emulator->GetMemory()->LoadMemory(fetch_address + 1);
			bool flip_x = false;

			for (uint8 i = 0; i < 8; i++)
			{
				// Extract pixel
				SPixel current;
				uint8 msb = 7 - i;
				uint8 lsb = i;

				uint8 bit = flip_x ? lsb : msb;
				current.color = ((data_low >> bit) & 0x1) | (((data_high >> bit) & 0x1) << 1);
				current.palette = 0;
				current.bg_prio = 0;
				current.sprite_prio = 0;

				m_bg_fifo.push_back(current);
			}
		}

		// Pixel merge!
		SPixel pixel = m_bg_fifo.front();

		uint8 bg_palette = m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_BACKGROUND_PALETTE_DMG);
		uint8 color_index = (bg_palette >> (2 * pixel.color)) & 0x3; // 0 first two bits, 1 second two bits..

		if (m_oam_fifo.size() != 0)
		{
			SPixel oam_pixel = m_oam_fifo.front();
			uint8 sprite_palettes[2] = {
				m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_OBJ0_PALETTE_DMG),
				m_emulator->GetMemory()->LoadMemory(VIDEO_REGISTER_OBJ1_PALETTE_DMG)
			};

			if (oam_pixel.color > 0) // translucency
			{
				// sprite wins!
				color_index = (sprite_palettes[oam_pixel.palette] >> (2 * oam_pixel.color)) & 0x3;
			}

			m_oam_fifo.pop_front();
		}
		
		m_bg_fifo.pop_front();

		uint32 palette_to_rgb_dmg[4] = { 0xffffffff, 0xffaaaaaa, 0xff555555, 0xff000000 };
		uint32 final_color = palette_to_rgb_dmg[color_index];

	}
}
