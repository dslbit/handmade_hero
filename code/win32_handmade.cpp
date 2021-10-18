#include <windows.h>
#include <stdint.h>

#define internal 		static
#define global_variable static
#define local_persist 	static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// TODO(Douglas): Por enquanto isso vai ser global. 
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void
RenderWeirdGradient(int BlueOffset, int GreenOffset)
{
	/*
		ORDEM NA MEMÓRIA: 			RR GG BB xx
		CARREGADO (da memória):		xx BB GG RR
		WINDOWS: 					xx RR GG BB
		COPIADO PRA MEMÓRIA: 		BB GG RR xx
	*/

	int Width = BitmapWidth;
	int Height = BitmapHeight;
	int Pitch = Width * BytesPerPixel;
	uint8 *Row = (uint8 *)BitmapMemory;

	for(int Y = 0;
	    Y < BitmapHeight;
	    Y++)
	{
		//uint32 *Pixel = (uint32 *)BitmapMemory;
		uint32 *Pixel = (uint32 *)Row;

		for(int X = 0;
		    X < BitmapWidth;
		    X++)
		{
			//             0  8  16 24 32  
			// Registrador: xx RR GG BB
			// Memória: 	BB GG RR xx

			uint8 Red = (X+Y*3 * X) + (GreenOffset-2) + BlueOffset;
			uint8 Blue = 0;//(X + BlueOffset);
			uint8 Green = 0;//(Y + GreenOffset);

			*Pixel = (Red << 16) | (Green << 8) | Blue;
			++Pixel;
		}

		// NOTE(Douglas): a expressão abaixo é a mesma coisa que a seguinte,
		// no momento, pois não temos espaçamento na nossa memória, mas se
		// tivessimos isso falharia.
		//Row = (uint8 *)Pixel;
		Row += Pitch;
	}
}

// NOTE(Douglas): DIB: Device Independent Bitmap (é um bloco de memória que
// pode ser manipulado pela gente e que o Windows usa pra desenhar usando
// a sua biblioteca gráfica gdi)
internal void
Win32ResizeDIBSection(int Width, int Height)
{
	// TODO(Douglas): Talvez não esvaziar a memória primeiro, esvaziar depois, 
	// e só esvaziar primeiro se falhar.

	if(BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = Width;
	BitmapHeight = Height;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight; // negativo: top-down, positivo: down-up
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	// NOTE(Douglas): Graças ao Chris Hecker por explicar que "StretchDIBits",
	// antigamente, era mais lento do que "BitBlt" nós não precisamos mais
	// ter "DeviceContext" voando por ai.
	int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	
	// TODO(Douglas): Provavelmente é preciso limpar isso aqui pra preto.
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect)
{
	int ClientWidth = WindowRect->right - WindowRect->left;
	int ClientHeight = WindowRect->bottom - WindowRect->top;

	StretchDIBits(DeviceContext,
	              /*
	              X, Y, Width, Height,
	              X, Y, Width, Height,
	              */
	              0, 0, ClientWidth, ClientHeight,
	              0, 0, BitmapWidth, BitmapHeight,
	              BitmapMemory,
	              &BitmapInfo,
	              DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
	LRESULT Result = 0;

	switch(Message)
	{
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			Win32UpdateWindow(DeviceContext, &ClientRect);
			EndPaint(Window, &Paint);
		} break;

		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(Width, Height);
		} break;

		case WM_CLOSE:
		{
			// TODO(Douglas): Enviar uma mensagem para o usuário?
			Running = false;
		} break;

		case WM_DESTROY:
		{
			// TODO(Douglas): Tratar o possível erro que pode acontecer aqui - re-criar a janela?
			Running = false;
		} break;

		default:
		{
			//OutputDebugStringA("default\n");
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}

	return(Result);
}

int WINAPI
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR CommandLine,
		int ShowCode)
{
	WNDCLASSA WindowClass = {};
	// TODO(Douglas): Verificar se CS_OWNDC, CS_HREDRAW e CS_VREDRAW ainda é importante
	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	//WindowClass.hIcon = ;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if(RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(0,
		                                   WindowClass.lpszClassName,
		                                   "Handmade Hero",
		                                   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		                                   CW_USEDEFAULT,
		                                   CW_USEDEFAULT,
		                                   CW_USEDEFAULT,
		                                   CW_USEDEFAULT,
		                                   0,
		                                   0,
		                                   Instance,
		                                   0);

		if(Window)
		{
			int XOffset = 0;
			int YOffset = 0;

			Running = true;
			while(Running)
			{
				MSG Message;

				while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						Running = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				RenderWeirdGradient(XOffset, YOffset);

				HDC DeviceContext = GetDC(Window);
				RECT ClientRect;
				GetClientRect(Window, &ClientRect);
				//int ClientWidth = ClientRect.right - ClientRect.left;
				//int ClientHeight = ClientRect.bottom - ClientRect.top;
				Win32UpdateWindow(DeviceContext, &ClientRect);
				ReleaseDC(Window, DeviceContext);

				++XOffset;
				YOffset += 2;
			}
		}
		else
		{
			//TODO(Douglas): Informar
		}
	}
	else
	{
		//TODO(Douglas): Informar
	}

	return(0);
}
