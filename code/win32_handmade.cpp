#include <windows.h>
#include <xinput.h>
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

struct win32_offscreen_buffer
{
	// NOTE(Douglas): Pixels são sempre 32-bits, Ordem de memória: bb gg rr xx
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct win32_window_dimensions
{
	int Width;
	int Height;
};

// ~

//
// NOTE(Douglas): Evitando que o programa crashe se o jogador não tiver as "dlls" do "xinput".
// a ideia é bem simples, porque os ponteiros das funções ("XInputGetState", "XInputSetState") apontam
// para uma função padrão logo de início.
//

// NOTE(Douglas): Suporte para "XInputGetState"
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(xinput_get_state);
XINPUT_GET_STATE(XInputGetStateStub)
{
	return(0);
}
global_variable xinput_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(Douglas): Suporte para "XInputSetState"
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(XInputSetStateStub)
{
	return(0);
}
global_variable xinput_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");

	if(XInputLibrary)
	{
		XInputGetState = (xinput_get_state *) GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (xinput_set_state *) GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

// ~

// TODO(Douglas): Por enquanto isso vai ser global. 
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

internal win32_window_dimensions
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimensions Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return(Result);
}

internal void
RenderWeirdGradient(win32_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
	/*
		ORDEM NA MEMÓRIA: 			RR GG BB xx
		CARREGADO (da memória):		xx BB GG RR
		WINDOWS: 					xx RR GG BB
		COPIADO PRA MEMÓRIA: 		BB GG RR xx
	*/

	// TODO(Douglas): Vamos ver o que o otimizador vai fazer quando passamos "Buffer"
	// por valor em vez de usar um ponteiro.
	uint8 *Row = (uint8 *)Buffer->Memory;

	for(int Y = 0;
	    Y < Buffer->Height;
	    Y++)
	{
		//uint32 *Pixel = (uint32 *)BitmapMemory;
		uint32 *Pixel = (uint32 *)Row;

		for(int X = 0;
		    X < Buffer->Width;
		    X++)
		{
			//             0  8  16 24 32  
			// Registrador: xx RR GG BB
			// Memória: 	BB GG RR xx

			uint8 Red = 0;//(X+Y*3 * X) + (GreenOffset-2) + BlueOffset;
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);

			*Pixel++ = (Red << 16) | (Green << 8) | Blue;
		}

		// NOTE(Douglas): a expressão abaixo é a mesma coisa que a seguinte,
		// no momento, pois não temos espaçamento na nossa memória, mas se
		// tivessimos isso falharia.
		//Row = (uint8 *)Pixel;
		Row += Buffer->Pitch;
	}
}

// NOTE(Douglas): DIB: Device Independent Bitmap (é um bloco de memória que
// pode ser manipulado pela gente e que o Windows usa pra desenhar usando
// a sua biblioteca gráfica gdi)
internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	// TODO(Douglas): Talvez não esvaziar a memória primeiro, esvaziar depois, 
	// e só esvaziar primeiro se falhar.

	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	int BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // negativo: top-down, positivo: down-up
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	// NOTE(Douglas): Graças ao Chris Hecker por explicar que "StretchDIBits",
	// antigamente, era mais lento do que "BitBlt" nós não precisamos mais
	// ter "DeviceContext" voando por ai.
	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Width * BytesPerPixel;
	
	// TODO(Douglas): Provavelmente é preciso limpar isso aqui pra preto.
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext,
                           win32_offscreen_buffer *Buffer,
                           int Width, int Height)
{
	// TODO(Douglas): Corrigir o "Aspect Ratio"
	// TODO(Douglas): Brincar com os "Stretch Modes"

	StretchDIBits(DeviceContext,
	              /*
	              X, Y, Width, Height,
	              X, Y, Width, Height,
	              */
	              0, 0, Width, Height,
	              0, 0, Buffer->Width, Buffer->Height,
	              Buffer->Memory,
	              &Buffer->Info,
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

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = WParam;
			bool WasDown = ((LParam & (1 << 30)) != 0);
			bool IsDown = ((LParam & (1 << 31)) == 0);

			if(WasDown != IsDown)
			{
				switch(VKCode)
				{
					case 'W':
					{

					} break;

					case 'A':
					{

					} break;

					case 'S':
					{

					} break;

					case 'D':
					{

					} break;

					case 'Q':
					{

					} break;

					case 'E':
					{

					} break;

					case VK_DOWN:
					{

					} break;

					case VK_LEFT:
					{

					} break;

					case VK_UP:
					{

					} break;

					case VK_RIGHT:
					{

					} break;

					case VK_SPACE:
					{

					} break;

					case VK_ESCAPE:
					{
						OutputDebugStringA("ESCAPE: ");
						if(IsDown)
						{
							OutputDebugStringA("IsDown ");
						}
						if(WasDown)
						{
							OutputDebugStringA("WasDown");
						}
						OutputDebugStringA("\n");
					} break;
				}
			}
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);
		} break;

		case WM_CLOSE:
		{
			// TODO(Douglas): Enviar uma mensagem para o usuário?
			GlobalRunning = false;
		} break;

		case WM_DESTROY:
		{
			// TODO(Douglas): Tratar o possível erro que pode acontecer aqui - re-criar a janela?
			GlobalRunning = false;
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
	Win32LoadXInput();
	Win32ResizeDIBSection(&GlobalBackBuffer, 640, 480);

	WNDCLASSA WindowClass = {};
	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // NOTE(Douglas): Flags de importância
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
			// NOTE(Douglas): Já que estamos usando "CS_OWNDC", talvez nós não preicsamos
			// usar "ReleaseDC" porque não estamos compartilhando esse DC com nenhuma outra
			// janela.
			HDC DeviceContext = GetDC(Window);
			int BlueOffset = 0;
			int GreenOffset = 0;

			GlobalRunning = true;
			while(GlobalRunning)
			{
				MSG Message;

				while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				// TODO(Douglas): Deveríamos puxar essas informações com mais frequência?
				for(DWORD ControllerIndex = 0;
				    ControllerIndex < XUSER_MAX_COUNT;
				    ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					
					if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// NOTE(Douglas): Controle está conectado
						// TODO(Douglas): Ver se "ControllerState.dwPacketNumber" incrementa rápido demais

						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BBUtton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;

						if(AButton)
						{
							GreenOffset += 2;
						}
					}
					else
					{
						// NOTE(Douglas): Controle não está disponível
					}
				}

				// NOTE(Douglas): Deixar isso até só até eu conseguir testar. Talvez eu passe esse código
				// pro Rafa dar uma olhada pra mim e ver se o problema é no meu controle, porque acredito
				// que meu código não tem nenhum problema. Deve ser meu controle de PS3 que não está
				// sendo reconhecido e mapeado pelo "xinput" (sei lá).
				XINPUT_VIBRATION Vibration;
				Vibration.wLeftMotorSpeed = 60000;
				Vibration.wRightMotorSpeed = 60000;
				XInputSetState(0, &Vibration);

				RenderWeirdGradient(&GlobalBackBuffer, BlueOffset, GreenOffset);

				win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer, Dimension.Width, Dimension.Height);
				//ReleaseDC(Window, DeviceContext);

				++BlueOffset;
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
