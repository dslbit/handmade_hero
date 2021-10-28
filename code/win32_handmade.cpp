// TODO(Douglas): Esta não é uma camada de plataforma final!!!!
// - Salvar locais do jogo
// - Obter um identificador (handle) para o nosso executável
// - Carregamento de recursos
// - Threading
// - Raw Input (suporte para múltiplos teclados / controles)
// - Sleep/timeBeginPeriod (para não "derreter" CPU e bateria (no caso de um notebook))
// - ClipCursor() (para o suporte de múltiplos monitores)
// - Suporte para Fullscreen
// - WM_SETCURSOR para controlar a visibilidade do cursor
// - QueryCancelAutoPlay
// - WM_ACTIVEAPP para quando o jogo não ser a aplicação ativa
// - Melhorias no método de Blit (talvez usar BitBlt)
// - Aceleração de Hardware (OpenGL ou Direct3D ou Ambos???)
// - GetKeyboardLayout (suporte para teclados francêses, suporte internacional para WASD)

#include <stdint.h>

// TODO(Douglas): Implementar, nós mesmos, a função seno
#include <math.h>

#define Pi32 (3.14159265359f)

#define internal 		static
#define global_variable static
#define local_persist 	static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32_t bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include "handmade.cpp"

#include <windows.h>
#include <hidsdi.h> // NOTE(Douglas): Para "Raw Input" do meu controle
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>
#include <malloc.h>
#include <assert.h>

#include "win32_handmade.h"

// TODO(Douglas): Por enquanto isso vai ser global.
global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer; // Pixels
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer; // Áudio



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
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable xinput_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(Douglas): Suporte para "XInputSetState"
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable xinput_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");

	if(!XInputLibrary)
	{
		// TODO(Douglas): Diagnóstico
		HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	}

	if(!XInputLibrary)
	{
		// TODO(Douglas): Diagnóstico
		HMODULE XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}

	if(XInputLibrary)
	{
		XInputGetState = (xinput_get_state *) GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (xinput_set_state *) GetProcAddress(XInputLibrary, "XInputSetState");

		// TODO(Douglas): Diagnóstico
	}
	else
	{
		// TODO(Douglas): Diagnóstico
	}
}

// ~

// ~

// NOTE(Douglas): Carregando DirectSound manualmente, porque mesmo que o usuário
// não tenha como escutar nada, ele ainda pode abrir o programa sem nenhum
// problema.

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	// NOTE(Douglas): Carregar a biblioteca
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if(DSoundLibrary)
	{
		// NOTE(Douglas): Pegar um DirectSound Object
		direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		// NOTE(Douglas): Verificar se isso aqui funciona no WinXP - DirectSound8 ou 7??
		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = (WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign);
			WaveFormat.cbSize = 0;

			if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				// NOTE(Douglas): Por algum motivo a convenção é limpar pra zero antes de usar...
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				// NOTE(Douglas): Criar um bloco de memória primário (para configurar o modo, antes do D.S. ler)
				// TODO(Douglas): DSBCAPS_GLOBALFOCUS?
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
					if(SUCCEEDED(Error))
					{
						// NOTE(Douglas): Agora nós finalmentes configuramos o formato!
						OutputDebugStringA("Formato do buffer primario configurado!\n");
					}
					else
					{
						// TODO(Douglas): Diagnóstico
					}
				}
				else
				{
					// TODO(Douglas): Diagnóstico
				}
			}
			else
			{
				// TODO(Douglas): Diagnóstico
			}


			// NOTE(Douglas): Criar um bloco de memória segundário (onde iremos escrever o som)
			// TODO(Douglas): DSBCAPS_GETCURRENTPOSITION2 ?
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if(SUCCEEDED(Error))
			{
				OutputDebugStringA("Buffer secundario criado com sucesso!\n");
			}
			else
			{
				// TODO(Douglas): Diagnóstico
			}


			// NOTE(Douglas): TOCAR!
		}
		else
		{
			// TODO(Douglas): Diagnóstico
		}
	}
	else
	{
		// TODO(Douglas): Diagnóstico
	}
}

// ~

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
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
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
			bool32 WasDown = ((LParam & (1 << 30)) != 0);
			bool32 IsDown = ((LParam & (1 << 31)) == 0);

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

			bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);
			if(VKCode == VK_F4 && AltKeyWasDown)
			{
				GlobalRunning = false;
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
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;
	}

	return(Result);
}

internal void
Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0,
	                                         SoundOutput->SecondaryBufferSize,
	                                         &Region1, &Region1Size,
	                                         &Region2, &Region2Size,
	                                         0)))
	{
		uint8 *DestSample = (uint8 *)Region1;
		for(DWORD ByteIndex = 0;
		    ByteIndex < Region1Size;
		    ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		DestSample = (uint8 *)Region2;
		for(DWORD ByteIndex = 0;
		    ByteIndex < Region2Size;
		    ++ByteIndex)
		{
			*DestSample++ = 0;
		}
		
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size,
		                              Region2, Region2Size);
	}
	else
	{
		// TODO(Douglas): Diagnóstico (buffer não conseguiu "dar lock")
	}
}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput,
                     DWORD ByteToLock,
                     DWORD BytesToWrite,
                     game_output_sound_buffer *Source)
{
	// TODO(Douglas): Fazer um teste mais árduo aqui pra ver se tem algo de errado.
	//  int16 int16  ...
	// [LEFT  RIGHT] LEFT RIGHT LEFT RIGHT ...
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock,
	                                         BytesToWrite,
	                                         &Region1, &Region1Size,
	                                         &Region2, &Region2Size,
	                                         0)))
	{
		// TODO(Douglas): "assert" que Region1Size/Region2Size é valido

		// TODO(Douglas): Colapsar esse dois "loops" porque eles são "o mesmo".
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;

		int16 *DestSample = (int16 *)Region1;
		int16 *SourceSample = Source->Samples;
		for(DWORD SampleIndex = 0;
		    SampleIndex < Region1SampleCount;
		    ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;

		DestSample = (int16 *)Region2;
		for(DWORD SampleIndex = 0;
		    SampleIndex < Region2SampleCount;
		    ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size,
		                              Region2, Region2Size);
	}
	else
	{
		// TODO(Douglas): Diagnóstico (buffer não conseguiu "dar lock")
	}
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
                                game_button_state *OldState,
                                DWORD ButtonBit,
                                game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

int WINAPI
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR CommandLine,
		int ShowCode)
{
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

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

			// TODO(Douglas): Talvez tornar o tamanho desse buffer em 1 minuto?
			// (Pra ficar mais fácil de debugar?)
			win32_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			// Juntar com o "VirtualAlloc" do Bitmap
			int16 *Samples = (int16 *) VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
			                                        (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);

#if HANDMADE_INTERNAL
			LPVOID BaseAddress = (LPVOID) Terabytes(2);
#else
			LPVOID BaseAddress = 0;
#endif

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Megabytes(512);
			
			// TODO(Douglas): Tratar vários tamanhos mínimos de memória dependendo das máquina do usuário
			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize,
			                                           (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);

			//GameMemory.TransientStorageSize = Gigabytes((uint64)4);
			GameMemory.TransientStorage = (uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;

			// NOTE(Douglas): Se falhar alguma dessas alocações, nem precisa iniciar o jogo.
			if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				GlobalRunning = true;

				LARGE_INTEGER LastCounter;
				QueryPerformanceCounter(&LastCounter);
				uint64 LastCycleCount = __rdtsc();
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

					//
					// NOTE(Douglas): Entrada, atualização e renderização
					//

					int32 MaxControllerCount = XUSER_MAX_COUNT;
					if(MaxControllerCount > ArrayCount(NewInput->Controllers))
					{
						MaxControllerCount = ArrayCount(NewInput->Controllers);
					}

					// TODO(Douglas): Deveríamos puxar essas informações com mais frequência?
					for(DWORD ControllerIndex = 0;
					    ControllerIndex < MaxControllerCount;
					    ++ControllerIndex)
					{
						XINPUT_STATE ControllerState;
						game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
						game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];

						if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							// NOTE(Douglas): Controle está conectado
							// TODO(Douglas): Ver se "ControllerState.dwPacketNumber" incrementa rápido demais

							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
							
							// TODO(Douglas): Dpad
							bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
							bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
							bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
							bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

							//bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
							//bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

							NewController->IsAnalog = true;
							NewController->StartX = OldController->EndX;
							NewController->StartY = OldController->EndY;

							// TODO(Douglas): Depois tratar a "deadzone" do controle propriamente com
							// #define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
							// #define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE

							// TODO(Douglas): min/max macros!!!
							// TODO(Douglas): Junto isso em uma função
							real32 X;
							if(Pad->sThumbLX < 0)
							{
								X = (real32) Pad->sThumbLX / 32768.0f;
							}
							else
							{
								X = (real32) Pad->sThumbLX / 32767.0f;
							}
							NewController->MinX = NewController->MaxX = NewController->EndX = X;

							real32 Y;
							if(Pad->sThumbLY < 0)
							{
								Y = (real32) Pad->sThumbLY / 32768.0f;
							}
							else
							{
								Y = (real32) Pad->sThumbLY / 32767.0f;
							}
							NewController->MinY = NewController->MaxY = NewController->EndY = Y;
							
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->Down,
							                                XINPUT_GAMEPAD_A,
							                                &NewController->Down);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->Up,
							                                XINPUT_GAMEPAD_Y,
							                                &NewController->Up);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->Left,
							                                XINPUT_GAMEPAD_X,
							                                &NewController->Left);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->Right,
							                                XINPUT_GAMEPAD_B,
							                                &NewController->Right);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->LeftShoulder,
							                                XINPUT_GAMEPAD_LEFT_SHOULDER,
							                                &NewController->LeftShoulder);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->RightShoulder,
							                                XINPUT_GAMEPAD_RIGHT_SHOULDER,
							                                &NewController->RightShoulder);
						}
						else
						{
							// TODO(Douglas): Diagnóstico (controle não está disponível)
						}
					}

					DWORD ByteToLock = 0;
					DWORD TargetCursor = 0;
					DWORD BytesToWrite = 0; 
					DWORD PlayCursor = 0;
					DWORD WriteCursor = 0;
					bool32 SoundIsValid = false;
					// TODO(Douglas): Melhorar a lógica do áudio para sabermos onde vamos estar escrevendo
					// e para conseguirmos antecipar o tempo gasto no "GameUpdate".
					if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
					{
						ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
						TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;

						if(ByteToLock > TargetCursor)
						{
							BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - ByteToLock;
						}

						SoundIsValid = true;
					}
					else
					{
						// TODO(Douglas): Diagnóstico (não conseguiu pegar a posição do ponteiro de escrita do buffer)
					}

					game_output_sound_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;
					Buffer.Pitch = GlobalBackBuffer.Pitch;

					GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);
					//RenderWeirdGradient(&GlobalBackBuffer, BlueOffset, GreenOffset);
					
					if(SoundIsValid)
					{
						Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
					}

					win32_window_dimensions Dimension = Win32GetWindowDimension(Window);
					Win32DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer, Dimension.Width, Dimension.Height);

					//
					// NOTE(Douglas): Cronometragem
					//
					uint64 EndCycleCount = __rdtsc();
					LARGE_INTEGER EndCounter;
					QueryPerformanceCounter(&EndCounter);

					uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
					real32 MegaCyclesPerFrame = (real32) CyclesElapsed / (1000.0f * 1000.0f);
					int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
					real32 MSPerFrame = ((1000.0f * (real32) CounterElapsed) / (real32) PerfCountFrequency); // CounterElapsed / PerfCountFrequency = segundos por frame
					real32 FPS = (real32) PerfCountFrequency / (real32) CounterElapsed; // counts_per_second / counts_elapsed
					real32 CurrentCPUGhz = (FPS * MegaCyclesPerFrame) / 1000.0f;

	#if 0
					static uint64 DebugStringDelay = 0;
					if(DebugStringDelay % 128 == 0)
					{
						int8 Buffer[256];
						sprintf((LPSTR) Buffer, "%.02f (ms/f) | %.02f (f/s) | %.02f (million instructions - MCPF) | %.02f Ghz\n", MSPerFrame, FPS, MegaCyclesPerFrame, CurrentCPUGhz);
						OutputDebugStringA((LPSTR) Buffer);
					}
					++DebugStringDelay;
	#endif

					LastCounter = EndCounter;
					LastCycleCount = EndCycleCount;

					//
					// NOTE(Douglas): Trocando os estados dos controles
					//
					game_input *InputTemp;
					InputTemp = NewInput;
					NewInput = OldInput;
					OldInput = InputTemp;
					// TODO(Douglas): Deveríamos limpar isso aqui?
				}
			}
			else
			{
				// TODO(Douglas): Diagnóstico (a plataforma não conseguiu armazenar memória para o jogo)
			}
		}
		else
		{
			//TODO(Douglas): Diagnóstico
		}
	}
	else
	{
		//TODO(Douglas): Diagnóstico
	}

	return(0);
}
