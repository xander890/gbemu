#pragma once
#include "types.h"
#include "imemory.h"
#include "types.h"

class ZEmulator;

// Joypad
#define JOYPAD_REGISTER				0xFF00

// Serial
#define SERIAL_BYTE_REGISTER		0xFF01
#define SERIAL_CONTROL_REGISTER		0xFF02

// Timer
#define TIMER_DIVIDER_REGISTER		0xFF03
#define TIMER_COUNTER_REGISTER		0xFF04
#define TIMER_MODULO_REGISTER		0xFF05
#define TIMER_CONTROL_REGISTER		0xFF06

// Interrupts
#define INTERRUPT_REQUEST_FLAG		0xFF0F
#define INTERRUPT_ENABLE_FLAG		0xFFFF

// Video
#define VIDEO_REGISTER_LCD_CONTROL				0xFF40
#define VIDEO_REGISTER_LCD_STATUS				0xFF41	
#define VIDEO_REGISTER_SCROLL_X					0xFF42
#define VIDEO_REGISTER_SCROLL_Y					0xFF43
#define VIDEO_REGISTER_LCD_LY_LINE				0xFF44
#define VIDEO_REGISTER_LCD_LY_COMPARE			0xFF45
#define VIDEO_REGISTER_DMA_CONTROL				0xFF46
#define VIDEO_REGISTER_BACKGROUND_PALETTE_DMG	0xFF47
#define VIDEO_REGISTER_OBJ0_PALETTE_DMG			0xFF48
#define VIDEO_REGISTER_OBJ1_PALETTE_DMG			0xFF49
#define VIDEO_REGISTER_WINDOW_X					0xFF4A
#define VIDEO_REGISTER_WINDOW_Y					0xFF4B

// CGB Special flags
#define CGB_FLAG_SPEED_SWITCH					0xFF4D
#define CGB_FLAG_VRAM_BANK						0xFF4F
#define CGB_FLAG_HDMA							0xFF55
#define CGB_FLAG_INFRARED_PORT					0xFF56
#define CGB_FLAG_BACKGROUND_PALETTE_INDEX		0xFF68
#define CGB_FLAG_BACKGROUND_PALETTE_DATA		0xFF69
#define CGB_FLAG_OBJ_PALETTE_INDEX				0xFF6A
#define CGB_FLAG_OBJ_PALETTE_DATA				0xFF6B
#define CGB_FLAG_WRAM_BANK						0xFF70

#define DISABLE_BASE_ROM						0xFF50


enum EButtonFlags : uint8
{
	BUTTON_RIGHT	= 1 << 0,
	BUTTON_LEFT		= 1 << 1,
	BUTTON_UP		= 1 << 2,
	BUTTON_DOWN		= 1 << 3,
	BUTTON_A		= 1 << 4,
	BUTTON_B		= 1 << 5,
	BUTTON_START	= 1 << 6,
	BUTTON_SELECT	= 1 << 7
};

class ZInputOutputMemoryController : public IMemoryController
{
public:
	ZInputOutputMemoryController(ZEmulator* emu) : m_emulator(emu) { memset(m_input_memory, 0x00, sizeof(m_input_memory)); }
	~ZInputOutputMemoryController() {}
	// Inherited via IMemoryController
	virtual uint8 LoadMemory(uint16 address) override;
	virtual void StoreMemory(uint16 address, uint8 value) override;

private:
	uint8 m_input_memory[0x80];

	// All the 8 buttons inputs in one variable
	uint8 m_triggered_inputs = 0; 
	ZEmulator* m_emulator;
};

void InputWindow(ZEmulator* pEMU);

