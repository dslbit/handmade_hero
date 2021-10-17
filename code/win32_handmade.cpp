#include <windows.h>

#define internal 		static
#define global_variable static
#define local_persist 	static

// TODO(Douglas): Por enquanto isso vai ser global. 
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

// NOTE(Douglas): DIB: Device Independent Bitmap (é um bloco de memória que
// pode ser manipulado pela gente e que o Windows usa pra desenhar usando
// a sua biblioteca gráfica gdi)
internal void
Win32ResizeDIBSection(int Width, int Height)
{
	// TODO(Douglas): Talvez não esvaziar a memória primeiro, esvaziar depois, 
	// e só esvaziar primeiro se falhar.

	if(BitmapHandle)
	{
		DeleteObject(BitmapHandle);
	}

	if(!BitmapDeviceContext)
	{
		// TODO(Douglas): Deveríamos re-criar isso em circunstâncias específicas?
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	// TODO(Douglas): Talvez nós mesmos podemos alocar essa memória?
	BitmapHandle = CreateDIBSection(BitmapDeviceContext,
	                                &BitmapInfo,
	                                DIB_RGB_COLORS,
	                                &BitmapMemory,
	                                0, 0);
}

internal void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
	StretchDIBits(DeviceContext,
	              X, Y, Width, Height,
	              X, Y, Width, Height,
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
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
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
		HWND WindowHandle = CreateWindowExA(0,
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

		if(WindowHandle)
		{
			Running = true;

			while(Running)
			{
				MSG Message;
				BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);

				if(MessageResult > 0)
				{
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				else
				{
					break;
				}
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
