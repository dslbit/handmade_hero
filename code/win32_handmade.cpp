#include <windows.h>

int WINAPI
WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR pCmdLine,
		int nCmdShow)
{
	MessageBoxA(0, "This is a Message Box!", "Handmade Hero", MB_OK | MB_ICONINFORMATION);
	return(0);
}
