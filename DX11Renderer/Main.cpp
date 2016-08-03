#include "BaseSystem.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, PSTR pScmdl, int iCmdShow)
{
	BaseSystem* sys = new BaseSystem();
	if (!sys)	return -1;

	bool initialized = sys->Init();
	if (initialized)
		sys->Run();

	sys->Exit();
	delete sys;
	return 0;
}