#include "types.h"
#include <thread>

class ZCPU;
class ZPPU;
class ZMainMemory;
class IMemoryController;

class ZEmulator
{
public:
	void RunTests();
	void DrawGUI();
	void LoadROM(const char* file);
	ZEmulator();
	~ZEmulator();
	ZCPU* GetCPU() { return pCPU; }
	ZPPU* GetPPU() { return pPPU; }
	ZMainMemory* GetMemory();
	void Run();
	void Quit();

private:
	ZCPU* pCPU;
	ZPPU* pPPU;
	ZMainMemory* pMemory;
	std::thread m_thread;
};