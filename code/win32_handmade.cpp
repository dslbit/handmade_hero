#include <windows.h>
#include <hidsdi.h> // NOTE(Douglas): Para "Raw Input" do meu controle
#include <xinput.h>
#include <stdio.h>
#include <assert.h>
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
		// NOTE(Douglas): Registrando o "RawInputDevice" para as mensagens ser mandadas pra
		// "WM_INPUT".
		/*
		case WM_CREATE:
		{
			RAWINPUTDEVICE RawInputDevice;
			RawInputDevice.usUsagePage = 1; // Generic Desktop Controls
			RawInputDevice.usUsage = 5; // joystrick (precisa ser 5, pois o meu controle é considerado "gamepad")
			RawInputDevice.dwFlags = 0;
			RawInputDevice.hwndTarget = Window;

			assert(RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RAWINPUTDEVICE)) != 0);
		} break;
		*/

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		/*
		case WM_INPUT:
		{
			uint32 BufferSize;
			if(GetRawInputData((HRAWINPUT) LParam,
			                   RID_INPUT,
			                   0,
			                   &BufferSize,
			                   sizeof(RAWINPUTHEADER)) == 0)
			{
				if(BufferSize > 0)
				{
					PRAWINPUT RawInputPointer;
					HANDLE HeapHandle;
					HeapHandle = GetProcessHeap();
					RawInputPointer = (PRAWINPUT) HeapAlloc(HeapHandle, 0, BufferSize);
					assert(RawInputPointer != 0);

					uint32 BytesCopied = GetRawInputData((HRAWINPUT) LParam,
					                                     RID_INPUT,
					                                     RawInputPointer,
					                                     &BufferSize,
					                                     sizeof(RAWINPUTHEADER));
					//ParseRawInput(RawInputPointer);
					if(BytesCopied > 0)
					{
						// NOTE(Douglas): Se os dados vir de um dispositivo diferente de um mouse ou teclado
						if(RawInputPointer->header.dwType == 2) 
						{
							uint32 PreparsedDataBufferSize = 0;
							GetRawInputDeviceInfoA(RawInputPointer->header.hDevice,
							                       RIDI_PREPARSEDDATA,
							                       0,
							                       &PreparsedDataBufferSize);

							if(PreparsedDataBufferSize > 0)
							{
								// NOTE(Douglas): Tentar armazenar memória para os dados pré-analisados
								PHIDP_PREPARSED_DATA PreparsedData = (PHIDP_PREPARSED_DATA) HeapAlloc(HeapHandle,
								                                                                      0,
								                                                                      PreparsedDataBufferSize);
								if(GetRawInputDeviceInfoA(RawInputPointer->header.hDevice,
								                          RIDI_PREPARSEDDATA,
								                          PreparsedData,
								                          &PreparsedDataBufferSize) != -1) // >= 0
								{
									// NOTE(Douglas): Obtendo as capacidades do dispositivo
									HIDP_CAPS Caps;
									if(HidP_GetCaps(PreparsedData,
									                &Caps) == HIDP_STATUS_SUCCESS)
									{
										// NOTE(Douglas): Botões
										PHIDP_BUTTON_CAPS ButtonCaps = (PHIDP_BUTTON_CAPS) HeapAlloc(HeapHandle,
										                                                             0,
										                                                             sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps);
										USHORT ButtonCapsLength = Caps.NumberInputButtonCaps;
										HidP_GetButtonCaps(HidP_Input,
										                   ButtonCaps,
										                   &ButtonCapsLength,
										                   PreparsedData);
										uint32 NumberOfButtons = ButtonCaps->Range.UsageMax - ButtonCaps->Range.UsageMin + 1;

										ULONG UsageLength = NumberOfButtons * sizeof(USAGE);
										PUSAGE Usage = (PUSAGE) HeapAlloc(HeapHandle,
										                                  0,
										                                  UsageLength);
										HidP_GetUsages(HidP_Input,
										               ButtonCaps->UsagePage,
										               0,
										               Usage,
										               &UsageLength,
										               PreparsedData,
										               (PCHAR)RawInputPointer->data.hid.bRawData,
										               RawInputPointer->data.hid.dwSizeHid);

										int32 *ButtonStates = (int32 *) HeapAlloc(HeapHandle,
										                                        0,
										                                        NumberOfButtons * sizeof(int32));
										for(uint32 i = 0;
										    i < UsageLength;
										    ++i)
										{
											int32 ButtonIndex = Usage[i] - ButtonCaps->Range.UsageMin;
											ButtonStates[ButtonIndex] = 1;

											char OutputBuffer[256] = {};
											sprintf_s(OutputBuffer,
											          sizeof(OutputBuffer),
											          "Button %d: %d\n", ButtonIndex, ButtonStates[ButtonIndex]);
											OutputDebugStringA(OutputBuffer);
										}

										// NOTE(Douglas): analog sticks
										PHIDP_VALUE_CAPS ValueCaps = (PHIDP_VALUE_CAPS) HeapAlloc(HeapHandle,
										                                                          0,
										                                                          sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps);
										USHORT ValueCapsLength = Caps.NumberInputValueCaps;
										HidP_GetValueCaps(HidP_Input,
										                  ValueCaps,
										                  &ValueCapsLength,
										                  PreparsedData);
										for(uint32 i = 0;
										    i < Caps.NumberInputValueCaps;
										    ++i)
										{
											ULONG Value = 0;
											HidP_GetUsageValue(HidP_Input,
											                   ValueCaps[i].LinkUsagePage,
											                   0,
											                   ValueCaps[i].Range.UsageMin,
											                   &Value,
											                   PreparsedData,
											                   (PCHAR)RawInputPointer->data.hid.bRawData,
											                   RawInputPointer->data.hid.dwSizeHid);

											HIDP_VALUE_CAPS vc = ValueCaps[i];
											char OutputBuffer[2 * 1024] = {};
											sprintf_s(OutputBuffer,
											          sizeof(OutputBuffer),
											          "UsageMin: %hu - UsageMax: %hu | DataIndexMin: %hu - DataIndexMax: %hu | Value: %lu\n",
											          vc.Range.UsageMin,
											          vc.Range.UsageMax,
											          vc.Range.DataIndexMin,
											          vc.Range.DataIndexMax,
											          Value);
											OutputDebugStringA(OutputBuffer);

											switch(ValueCaps[i].Range.UsageMin)
											{
												// NOTE(Douglas): Valores do controle do Rafa (ver quais são os do meu, depois)
												
												// case 0x30:
												// {
												// 		LONG LAxisX = (LONG)Value - 128;

												// 		r32 NormalizedX = 0;
												// 		if (LAxisX < 0)
												// 		{
												// 				NormalizedX = LAxisX / 128;
												// 		}
												// 		else
												// 		{
												// 				NormalizedX = LAxisX / 127;
												// 		}

												// 		GlobalInput.Controllers[1].LeftAxisX = NormalizedX;
												// } break;

												// case 0x31:
												// {
												// 		LONG LAxisY = (LONG)Value - 128;

												// 		r32 NormalizedY = 0;
												// 		if (LAxisY < 0)
												// 		{
												// 				NormalizedY = LAxisY / -128;
												// 		}
												// 		else
												// 		{
												// 				NormalizedY = LAxisY / -127;
												// 		}

												// 		GlobalInput.Controllers[1].LeftAxisY = NormalizedY;
												// } break;

												// case 53:
												// {
												// 		LONG RAxisX = (LONG)Value - 128;

												// 		r32 NormalizedX = 0;
												// 		if (RAxisX < 0)
												// 		{
												// 				NormalizedX = RAxisX / 128;
												// 		}
												// 		else
												// 		{
												// 				NormalizedX = RAxisX / 127;
												// 		}

												// 		GlobalInput.Controllers[1].RightAxisX = NormalizedX;
												// } break;

												// case 50:
												// {
												// 		LONG RAxisY = (LONG)Value - 128;

												// 		r32 NormalizedY = 0;
												// 		if (RAxisY < 0)
												// 		{
												// 				NormalizedY = RAxisY / -128;
												// 		}
												// 		else
												// 		{
												// 				NormalizedY = RAxisY / -127;
												// 		}

												// 		GlobalInput.Controllers[1].RightAxisY = NormalizedY;
												// } break;
												
											}
										}
									}
									else
									{
										// NOTE(Douglas): Não conseguiu obter as capacidades do dispositivo
										OutputDebugStringA("Não conseguiu obter as capacidades do dispositivo\n");
									}
								}
								else
								{
									// NOTE(Douglas): "PreparsedData" não é grande o suficiente
									OutputDebugStringA("\"PreparsedData\" não é grande o suficiente\n");
								}
							}
							else
							{
								// NOTE(Douglas): "Preparsed data" foi zero
							}
						}
						else
						{
							// NOTE(Douglas): Dispositivo não é um Joystick
						}
					}
					else
					{
						// TODO(Douglas): "GetRawInputData" falhou
					}
					
					HeapFree(HeapHandle, 0, RawInputPointer);
				}
			}
			else
			{
				// TODO(Douglas): "GetRawInputData" falhou
			}
		} break;
		*/

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
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;
						BlueOffset += StickX / 10000;
						GreenOffset += StickY / 10000;
					}
					else
					{
						// NOTE(Douglas): Controle não está disponível
					}
				}

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
