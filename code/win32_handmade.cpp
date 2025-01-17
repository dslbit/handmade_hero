// TODO(Douglas): Esta não é uma camada de plataforma final!!!!
// - Tentar arrumar o atraso de WM_SETCURSOR 

// - Salvar locais do jogo
// - Obter um identificador (handle) para o nosso executável
// - Carregamento de recursos
// - Threading
// - ClipCursor() (para o suporte de múltiplos monitores)
// - QueryCancelAutoPlay
// - WM_ACTIVEAPP para quando o jogo não ser a aplicação ativa
// - Melhorias no método de Blit (talvez usar BitBlt)
// - Aceleração de Hardware (OpenGL ou Direct3D ou Ambos???)
// - GetKeyboardLayout (suporte para teclados francêses, suporte internacional para WASD)
// - ChangeDisplaySettings caso detectarmos blits lentos em tela cheia?

#include "handmade_platform.h"
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
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer; // Pixels
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer; // Áudio
global_variable int64 GlobalPerfCountFrequency;
global_variable bool32 DebugGlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};

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

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}


internal void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{
	// TODO(Douglas): Verificar o limite de "Dest"
	for(int Index = 0; Index < SourceACount; ++Index)
	{
		*Dest++ = *SourceA++;
	}

	for(int Index = 0; Index < SourceBCount; ++Index)
	{
		*Dest++ = *SourceB++;
	}

	*Dest++ = 0;
}

internal void
Win32GetEXEFileName(win32_state *State)
{
	// NOTE(Douglas): Nunca usar "MAX_PATH" em código que será entregue para o cliente,
	// porque pode ser perigoso e levar a maus resultados.
	DWORD SizeOfFileName = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
	State->OnePastLastEXEFileNameSlash = State->EXEFileName;
	for(char *Scan = State->EXEFileName; *Scan; ++Scan)
	{
		if(*Scan == '\\')
		{
			State->OnePastLastEXEFileNameSlash = Scan + 1;
		}
	}
}

internal int
StringLength(char *String)
{
	int Count = 0;
	while(*String++)
	{
		++Count;
	}
	return(Count);
}

internal void
Win32BuildEXEPathFileName(win32_state *State, char *FileName,
                          int DestCount, char *Dest)
{
	CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName,
	           StringLength(FileName), FileName,
	           DestCount, Dest);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
	debug_read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
			if(Result.Contents)
			{
				DWORD BytesRead;
				if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
				   (FileSize32 == BytesRead))
				{
					// NOTE(Douglas): Arquivo foi lido com sucesso
					Result.ContentsSize = FileSize32;
				}
				else
				{
					// TODO(Douglas): Diagnóstico (não conseguiu ler o arquivo)
					DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				// TODO(Douglas): Diagnóstico (não conseguiu armazenar memória para o arquivo)
			}
		}
		else
		{
			// TODO(Douglas): Diagnóstico (não conseguiu obter o tamanho do arquivo)
		}
		CloseHandle(FileHandle);
	}
	else
	{
		// TODO(Douglas): Diagnóstico (não conseguiu obter o handle do arquivo)
	}

	return(Result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
	bool32 Result = false;

	HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			// NOTE(Douglas): Arquivo foi escrito com sucesso
			Result = (MemorySize == BytesWritten);
		}
		else
		{
			// TODO(Douglas): Diagnóstico (não conseguiu escrever no arquivo)
		}

		CloseHandle(FileHandle);
	}
	else
	{
		// TODO(Douglas): Diagnóstico (não conseguiu obter o handle do arquivo)
	}

	return(Result);
}

inline FILETIME
Win32GetLastWriteTime(char *Filename)
{
	FILETIME LastWriteTime = {};

	#if 0
	WIN32_FIND_DATA FindData;
	HANDLE FindHandle = FindFirstFileA(Filename, &FindData);
	if(FindHandle != INVALID_HANDLE_VALUE)
	{
		LastWriteTime = FindData.ftLastWriteTime;
		FindClose(FindHandle);
	}
	#else

	WIN32_FILE_ATTRIBUTE_DATA Data;
	GetFileAttributesExA(Filename, GetFileExInfoStandard, &Data);
	LastWriteTime = Data.ftLastWriteTime;
	
	#endif
	return(LastWriteTime);
}

internal win32_game_code
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName, char *LockFileName)
{
	win32_game_code Result = {};

	WIN32_FILE_ATTRIBUTE_DATA Ignored;
	if(!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
	{
		Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

		// NOTE(Douglas): "CopyFile" pode falhar, então esse loop, no momento é necessário (pelo menos pra debug)
		for(;;)
		{
			if(CopyFile(SourceDLLName, TempDLLName, FALSE)) break; // se o valor for diferente de zero, deu certo
			if(GetLastError() == ERROR_FILE_NOT_FOUND) break;
		}

		CopyFile(SourceDLLName, TempDLLName, FALSE);
		Result.GameCodeDLL = LoadLibraryA(TempDLLName);
		if(Result.GameCodeDLL)
		{
			Result.UpdateAndRender = (game_update_and_render *) GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
			Result.GetSoundSamples = (game_get_sound_samples *) GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
			Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
		}

		if(!Result.IsValid)
		{
			Result.UpdateAndRender = 0;
			Result.GetSoundSamples = 0;
		}
	}

	return(Result);
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
	if(GameCode->GameCodeDLL)
	{
		FreeLibrary(GameCode->GameCodeDLL);
		GameCode->GameCodeDLL = 0;
	}

	GameCode->IsValid = false;
	GameCode->UpdateAndRender = 0;
	GameCode->GetSoundSamples = 0;
}

internal void
Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");

	if(!XInputLibrary)
	{
		// TODO(Douglas): Diagnóstico
		XInputLibrary = LoadLibraryA("xinput1_4.dll");
	}

	if(!XInputLibrary)
	{
		// TODO(Douglas): Diagnóstico
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
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
	Buffer->BytesPerPixel = BytesPerPixel;

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
                           int WindowWidth, int WindowHeight)
{
	// TODO: Centralizar?
	if(WindowWidth >= Buffer->Width*2 &&
	   WindowHeight >= Buffer->Height*2)
	{
		StretchDIBits(DeviceContext,
		              0, 0, Buffer->Width*2, Buffer->Height*2,
		              0, 0, Buffer->Width, Buffer->Height,
		              Buffer->Memory,
		              &Buffer->Info,
		              DIB_RGB_COLORS, SRCCOPY);
	}
	else
	{
		int XOffset = 10;
		int YOffset = 10;

		PatBlt(DeviceContext, 0, 0, WindowWidth, YOffset, BLACKNESS);
		PatBlt(DeviceContext, 0, YOffset + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
		PatBlt(DeviceContext, 0, 0, XOffset, WindowHeight, BLACKNESS);
		PatBlt(DeviceContext, XOffset + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

	// NOTE(Douglas): Para fins de protótipo, vamos "blitar" os pixels 1-para-1
	// para ter a certeza que não introduziremos nenhum artefato com o esticamento
	// enquanto estamos aprendendo a renderizar.
		StretchDIBits(DeviceContext,
		              XOffset, YOffset, Buffer->Width, Buffer->Height,
		              0, 0, Buffer->Width, Buffer->Height,
		              Buffer->Memory,
		              &Buffer->Info,
		              DIB_RGB_COLORS, SRCCOPY);
	}
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

		case WM_SETCURSOR:
		{
			if(DebugGlobalShowCursor)
			{
				Result = DefWindowProcA(Window, Message, WParam, LParam);
			}
			else
			{
				SetCursor(0);
			}
		} break;

		case WM_ACTIVATEAPP:
		{
			#if 0
			if(WParam == TRUE)
			{
				SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
			}
			else
			{
				SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 63, LWA_ALPHA);
			}
			#endif
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
			Assert(!"A entrada do teclado veio através de uma mensagem não despachada!");
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
Win32ProcessKeyboardMessage(game_button_state *NewState,
                            bool32 IsDown)
{
	if(NewState->EndedDown != IsDown)
	{
		NewState->EndedDown = IsDown;
		++NewState->HalfTransitionCount;
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

internal void
Win32GetInputFileLocation(win32_state *State,
                          bool32 InputStream,
                          int SlotIndex,
                          int DestCount, char *Dest)
{
	//Assert(SlotIndex == 1);
	char Temp[64];
	wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, InputStream ? "input" : "state");
	Win32BuildEXEPathFileName(State, Temp, DestCount, Dest);
}

internal win32_replay_buffer *
Win32GetReplayBuffer(win32_state *State, uint32 Index)
{
	Assert(Index > 0);
	Assert(Index < ArrayCount(State->ReplayBuffers));
	win32_replay_buffer *Result = &State->ReplayBuffers[Index];
	return(Result);
}

internal void
Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);
	if(ReplayBuffer->MemoryBlock)
	{
		State->InputRecordingIndex = InputRecordingIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(FileName), FileName);
		State->RecordingHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

		#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
		#endif

		CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndRecordingInput(win32_state *State)
{
	CloseHandle(State->RecordingHandle);
	State->InputRecordingIndex = 0;
}

internal void
Win32BeginInputPlayBack(win32_state *State, int InputPlayingIndex)
{
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);
	if(ReplayBuffer->MemoryBlock)
	{
		State->InputPlayingIndex = InputPlayingIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(FileName), FileName);
		State->PlaybackHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

		#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
		#endif

		CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndInputPlayback(win32_state *State)
{
	CloseHandle(State->PlaybackHandle);
	State->InputPlayingIndex = 0;
}

internal void
Win32RecordInput(win32_state *State, game_input *NewInput)
{
	DWORD BytesWritten;
	WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void
Win32PlayBackInput(win32_state *State, game_input *NewInput)
{
	DWORD BytesRead = 0;
	if(ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
	{
		if(BytesRead == 0)
		{
			int PlayingIndex = State->InputPlayingIndex;
			Win32EndInputPlayback(State);
			Win32BeginInputPlayBack(State, PlayingIndex);
			ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
		}
	}
}

internal void
ToggleFullscreen(HWND Window)
{
	// NOTE: Código do Raymond Chen para fullscreen
	// https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353

  DWORD Style = GetWindowLong(Window, GWL_STYLE);

  if(Style & WS_OVERLAPPEDWINDOW)
  {
    MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};

    if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
       GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
    {
      SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
      SetWindowPos(Window, HWND_TOP,
                   MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                   MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                   MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
  }
  else
  {
    SetWindowLong(Window, GWL_STYLE,
                  Style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(Window, &GlobalWindowPosition);
    SetWindowPos(Window, 0, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController)
{
	MSG Message;

	while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
	{
		if(Message.message == WM_QUIT)
		{
			GlobalRunning = false;
		}

		switch(Message.message)
		{
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32 VKCode = (uint32) Message.wParam;
				bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);

				if(WasDown != IsDown)
				{
					switch(VKCode)
					{
						case 'W':
						{
							Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
						} break;

						case 'A':
						{
							Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
						} break;

						case 'S':
						{
							Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
						} break;

						case 'D':
						{
							Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
						} break;

						case 'Q':
						{
							Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
						} break;

						case 'E':
						{
							Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
						} break;

						case VK_DOWN:
						{
							Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
						} break;

						case VK_LEFT:
						{
							Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
						} break;

						case VK_UP:
						{
							Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
						} break;

						case VK_RIGHT:
						{
							Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
						} break;

						case VK_SPACE:
						{
							Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
						} break;

						case VK_ESCAPE:
						{
							Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
							//GlobalRunning = false;
						} break;

						#if HANDMADE_INTERNAL
						case 'P':
						{
							if(IsDown)
							{
								GlobalPause = !GlobalPause;
							}
						} break;

						case 'L':
						{
							if(IsDown)
							{
								if(State->InputPlayingIndex == 0)
								{
									if(State->InputRecordingIndex == 0)
									{
										Win32BeginRecordingInput(State, 1);
									}
									else
									{
										Win32EndRecordingInput(State);
										Win32BeginInputPlayBack(State, 1);
									}
								}
								else
								{
									Win32EndInputPlayback(State);
								}
							}
						} break;
						#endif // HANDMADE_INTERNAL
					}

					if(IsDown)
					{
						bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
						if(VKCode == VK_F4 && AltKeyWasDown)
						{
							GlobalRunning = false;
						}
						if(VKCode == VK_RETURN && AltKeyWasDown)
						{
							if(Message.hwnd)
							{
								ToggleFullscreen(Message.hwnd);
							}
						}
					}
				}
			} break;

			default:
			{
				TranslateMessage(&Message);
				DispatchMessageA(&Message);
			} break;
		}
	}
}

internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
	real32 Result = 0;
	if(Value < -DeadZoneThreshold)
	{
		Result = (real32)(Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold);
	}
	else if(Value > DeadZoneThreshold)
	{
		Result = (real32)(Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold);
	}
	return(Result);
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return(Result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result = (real32) (End.QuadPart - Start.QuadPart) /
	                (real32) GlobalPerfCountFrequency;
	return(Result);
}

#if 0
internal void
Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer,
                       int X, int Top, int Bottom, uint32 Color)
{
	if(Top <= 0)
	{
		Top = 0;
	}

	if(Bottom > BackBuffer->Height)
	{
		Bottom = BackBuffer->Height;
	}

	if((X >= 0) && (X < BackBuffer->Width))
	{
		uint8 *Pixel = ((uint8 *)BackBuffer->Memory +
		                X * BackBuffer->BytesPerPixel +
		                Top * BackBuffer->Pitch);
		for(int Y = Top;
		    Y < Bottom;
		    ++Y)
		{
			*(uint32 *)Pixel = Color;
			Pixel += BackBuffer->Pitch;
		}
	}
}

inline void
Win32DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer,
                      		 win32_sound_output *SoundOutput,
                      		 real32 TargetSecondsPerFrame,
                      		 real32 C, int PadX, int Top, int Bottom,
                      		 DWORD Value, uint32 Color)
{
	real32 XReal32 = (C * (real32) Value);
	int X = PadX + (int) XReal32;
	Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer,
                      int MarkerCount,
                      win32_debug_time_marker *Markers,
                      int CurrentMarkerIndex,
                      win32_sound_output *SoundOutput,
                      real32 TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;

	int LineHeight = 64;

	real32 C = (real32) (BackBuffer->Width - 2*PadX) / (real32) SoundOutput->SecondaryBufferSize;
	for(int MarkerIndex = 0;
	    MarkerIndex < MarkerCount;
	    ++MarkerIndex)
	{
		win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
		Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
		//Assert(ThisMarker->ExpectedFlipPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

		DWORD PlayColor = 0xFFFFFFFF;
		DWORD WriteColor = 0xFFFF0000;
		DWORD ExpectedFlipColor = 0xFFFFFF00;
		DWORD PlayWindowColor = 0xFFFF00FF;
		
		int Top = PadY;
		int Bottom = PadY + LineHeight;
		if(MarkerIndex == CurrentMarkerIndex)
		{
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			int FirstTop = Top;

			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, TargetSecondsPerFrame, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, TargetSecondsPerFrame, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);

			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, TargetSecondsPerFrame, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, TargetSecondsPerFrame,C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);

			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;
			
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, TargetSecondsPerFrame, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
		}

		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, TargetSecondsPerFrame,
		                      		 C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);

		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, TargetSecondsPerFrame,
		                      		 C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480*SoundOutput->BytesPerSample, PlayWindowColor);
		
		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, TargetSecondsPerFrame,
		                      		 C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
	}
}
#endif

int WINAPI
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR CommandLine,
		int ShowCode)
{
	win32_state Win32State = {};

	Win32GetEXEFileName(&Win32State);

	char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "handmade.dll",
	                          sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

	char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "handmade_temp.dll",
	                          sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

	char GameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "lock.tmp",
	                          sizeof(GameCodeLockFullPath), GameCodeLockFullPath);

	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	// NOTE(Douglas): Configura a granularidade do "agendador" para 1ms
	// para que o "Sleep()" possa ser mais preciso.
	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	Win32LoadXInput();

	Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);

	#if HANDMADE_INTERNAL
	DebugGlobalShowCursor = true;
	#endif

	WNDCLASSA WindowClass = {};
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
	//WindowClass.hIcon = ;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";
	if(RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(0, //WS_EX_TOPMOST | WS_EX_LAYERED,
		                              WindowClass.lpszClassName,
		                              "Handmade Hero",
		                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		                              0,
		                              0,
		                              1280,
		                              720,
		                              0,
		                              0,
		                              Instance,
		                              0);

		if(Window)
		{
			int MonitorRefreshHz = 60;

			HDC RefreshDC = GetDC(Window);
			int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
			ReleaseDC(Window, RefreshDC);
			if(Win32RefreshRate > 1)
			{
				MonitorRefreshHz = Win32RefreshRate;
			}
			real32 GameUpdateHz = ((real32)MonitorRefreshHz / 2.0f);
			real32 TargetSecondsPerFrame = 1.0f / GameUpdateHz;

			// TODO(Douglas): Talvez tornar o tamanho desse buffer em 1 minuto?
			// (Pra ficar mais fácil de debugar?)
			win32_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.SafetyBytes = (int) (((real32)SoundOutput.SamplesPerSecond * (real32)SoundOutput.BytesPerSample / (real32)GameUpdateHz) / 3.0f);
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
				
			GlobalRunning = true;

			#if 0
			// NOTE(Douglas): Isso testa a frequencia de atualização do cursor de reprodução/escrita
			// Na minha máquina também foi igual a do Casey: 480 samples.
			while(GlobalRunning)
			{
				DWORD PlayCursor;
				DWORD WriteCursor;
				GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
				char TextBuffer[256];
				sprintf_s(TextBuffer, sizeof(TextBuffer),
				          "PC: %u | WC: %u\n", PlayCursor, WriteCursor);
				OutputDebugStringA(TextBuffer);
			}
			#endif

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
			GameMemory.TransientStorageSize = Megabytes(64);
			GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
			GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
			GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
			
			// TODO(Douglas): Tratar vários tamanhos mínimos de memória dependendo das máquina do usuário
			
			// TODO(Douglas): Usar MEM_LARGE_PAGES e chamar "adjust token privileges" quando não
			// estiver no WinXP?

			// TODO(Douglas): "TransientStorage" precisa ser quebrado em "game transient" e "cache trasient".
			Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
			                                          (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
			
			GameMemory.PermanentStorage = Win32State.GameMemoryBlock;

			//GameMemory.TransientStorageSize = Gigabytes((uint64)4);
			GameMemory.TransientStorage = (uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;

			// NOTE(Douglas): Somente para teste de build interna
			for(int ReplayIndex = 1;
			    ReplayIndex < ArrayCount(Win32State.ReplayBuffers);
			    ++ReplayIndex)
			{
				win32_replay_buffer *ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];

				Win32GetInputFileLocation(&Win32State, false,
				                          ReplayIndex, sizeof(ReplayBuffer->FileName),
				                          ReplayBuffer->FileName);

				ReplayBuffer->FileHandle = CreateFileA(ReplayBuffer->FileName, GENERIC_WRITE|GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);

				LARGE_INTEGER MaxSize;
				MaxSize.QuadPart = Win32State.TotalSize;
				ReplayBuffer->MemoryMap = CreateFileMapping(ReplayBuffer->FileHandle, 0, PAGE_READWRITE,
				                                            MaxSize.HighPart, MaxSize.LowPart, 0);
				ReplayBuffer->MemoryBlock = MapViewOfFile(ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, Win32State.TotalSize);

				if(ReplayBuffer->MemoryBlock)
				{
				}
				else
				{
					// TODO(Douglas): Diagnóstico
				}
			}

			// NOTE(Douglas): Se falhar alguma dessas alocações, nem precisa iniciar o jogo.
			if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				int DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[30] = {};

				DWORD AudioLatencyBytes = 0;
				real32 AudioLatencySeconds = 0;
				bool32 SoundIsValid = false;

				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();

				uint64 LastCycleCount = __rdtsc();

				win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
				                                         TempGameCodeDLLFullPath,
				                                         GameCodeLockFullPath);

				//
				// NOTE(Douglas): Entrada, atualização e renderização
				//
				while(GlobalRunning)
				{
					NewInput->dtForFrame = TargetSecondsPerFrame;

					POINT MouseP;
					GetCursorPos(&MouseP);
					ScreenToClient(Window, &MouseP);
					NewInput->MouseX = MouseP.x;
					NewInput->MouseY = MouseP.y;
					NewInput->MouseZ = 0; // TODO(Douglas): rodinha do mouse?
					Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0],
					                            (GetKeyState(VK_LBUTTON) & (1 << 15)));
					Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1],
					                            (GetKeyState(VK_MBUTTON) & (1 << 15)));
					Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2],
					                            (GetKeyState(VK_RBUTTON) & (1 << 15)));
					Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3],
					                            (GetKeyState(VK_XBUTTON1) & (1 << 15)));
					Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4],
					                            (GetKeyState(VK_XBUTTON2) & (1 << 15)));

					FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
					if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
					{
						Win32UnloadGameCode(&Game);
						Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
					}

					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					// TODO(Douglas): Macro para zerar
					// TODO(Douglas): Nós não podemos limpar para zero aqui porque o estado Up/Down será errado
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;
					for(int ButtonsCount = 0;
					    ButtonsCount < ArrayCount(NewKeyboardController->Buttons);
					    ++ButtonsCount)
					{
						NewKeyboardController->Buttons[ButtonsCount].EndedDown = OldKeyboardController->Buttons[ButtonsCount].EndedDown;
					}
					
					Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

					if(!GlobalPause)
					{
					// TODO(Douglas): Não tentar puxar informações de controles desconectados para evitar a
					// quedra do "xinput" em plataformas antigas.
					// TODO(Douglas): Deveríamos puxar essas informações com mais frequência?
					DWORD MaxControllerCount = 1 + XUSER_MAX_COUNT;
					if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1)) // "-1" é "menos o teclado, somente os controles"
					{
						MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
					}
					for(DWORD ControllerIndex = 0;
					    ControllerIndex < MaxControllerCount;
					    ++ControllerIndex)
					{
						DWORD OurControllerIndex = 1 + ControllerIndex;
						game_controller_input *OldController = &OldInput->Controllers[OurControllerIndex];
						game_controller_input *NewController = &NewInput->Controllers[OurControllerIndex];

						// TODO(Douglas): Ver se "ControllerState.dwPacketNumber" incrementa rápido demais
						XINPUT_STATE ControllerState;
						if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							NewController->IsConnected = true;
							NewController->IsAnalog = OldController->IsAnalog;
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

							// TODO(Douglas): Esta é uma "deadzone" quadrada, verificar a documentção
							// do "xinput" para ver se há uma "deadzone redonda".
							NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							if((NewController->StickAverageX != 0.0f) ||
							   (NewController->StickAverageY != 0.0f))
							{
								NewController->IsAnalog = true;
							}

							real32 Threshold = 0.5f;

							Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0,
							                                &OldController->MoveLeft, 1,
							                                &NewController->MoveLeft);
							Win32ProcessXInputDigitalButton((NewController->StickAverageX > Threshold) ? 1 : 0,
							                                &OldController->MoveRight, 1,
							                                &NewController->MoveRight);
							Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0,
							                                &OldController->MoveDown, 1,
							                                &NewController->MoveDown);
							Win32ProcessXInputDigitalButton((NewController->StickAverageY > Threshold) ? 1 : 0,
							                                &OldController->MoveUp, 1,
							                                &NewController->MoveUp);

							if((Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP))
							{
								NewController->StickAverageY = 1.0f;
								NewController->IsAnalog = false;
							}

							if((Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN))
							{
								NewController->StickAverageY = -1.0f;
								NewController->IsAnalog = false;
							}

							if((Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT))
							{
								NewController->StickAverageX = -1.0f;
								NewController->IsAnalog = false;
							}
							
							if((Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT))
							{
								NewController->StickAverageX = 1.0f;
								NewController->IsAnalog = false;
							}
							
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->MoveLeft, XINPUT_GAMEPAD_DPAD_LEFT,
							                                &NewController->MoveLeft);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->MoveRight, XINPUT_GAMEPAD_DPAD_RIGHT,
							                                &NewController->MoveRight);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->MoveUp, XINPUT_GAMEPAD_DPAD_UP,
							                                &NewController->MoveUp);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->MoveDown, XINPUT_GAMEPAD_DPAD_DOWN,
							                                &NewController->MoveDown);
							
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->ActionDown, XINPUT_GAMEPAD_A,
							                                &NewController->ActionDown);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->ActionUp, XINPUT_GAMEPAD_Y,
							                                &NewController->ActionUp);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->ActionLeft, XINPUT_GAMEPAD_X,
							                                &NewController->ActionLeft);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->ActionRight, XINPUT_GAMEPAD_B,
							                                &NewController->ActionRight);

							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->LeftShoulder,
							                                XINPUT_GAMEPAD_LEFT_SHOULDER,
							                                &NewController->LeftShoulder);
							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->RightShoulder,
							                                XINPUT_GAMEPAD_RIGHT_SHOULDER,
							                                &NewController->RightShoulder);

							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->Start,
							                                XINPUT_GAMEPAD_START,
							                                &NewController->Start);

							Win32ProcessXInputDigitalButton(Pad->wButtons,
							                                &OldController->Back,
							                                XINPUT_GAMEPAD_BACK,
							                                &NewController->Back);
						}
						else
						{
							// TODO(Douglas): Diagnóstico (controle não está disponível)
							NewController->IsConnected = false;
						}
					}

					thread_context Thread = {};

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;
					Buffer.BytesPerPixel = 4;
					Buffer.Pitch = GlobalBackBuffer.Pitch;

					if(Win32State.InputRecordingIndex)
					{
						Win32RecordInput(&Win32State, NewInput);
					}

					if(Win32State.InputPlayingIndex)
					{
						Win32PlayBackInput(&Win32State, NewInput);
					}

					if(Game.UpdateAndRender)
					{
						Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
					}

					LARGE_INTEGER AudioWallClock = Win32GetWallClock();
					real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

					DWORD PlayCursor;
					DWORD WriteCursor;
					if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
					{

						/* NOTE(Douglas):

							Aqui está como a computação do áudio funciona.

							Nós definimos um valor de segurança que é o número de amostras de som que nós achamos
							que o loop do jogo pode variar (por exemplo, 2ms).

							Quando acordamos para escrever o áudio, nós identificamos a posição do cursor de
							reprodução e prevemos onde nós achamos que esse cursor estará no próximo limite de quadro.
							Depois vamos ver se o cursor de escrita está antes desse limite (depois do valor de segurança),
							porque se estiver nós vamos escrever no limite do quadro seguinte até o frame seguinte, nos
							fornecendo uma sincronização de áudio perfeita no caso de uma placa de som com uma
							latência baixa. Mas se o cursor de escrita estiver depois do limite de quadro, nós assumimos que
							nunca poderemos sincronizar o quadro com o áudio perfeitamente e nós escrevemos um quadro equivalente de
							áudio mais o valor de segurança.
						*/

						if(!SoundIsValid)
						{
							SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
							SoundIsValid = true;
						}

						DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
						
						DWORD ExpectedSoundBytesPerFrame =
							(int) ((SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz);
						real32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
						DWORD ExpectedBytesUntilFlip = (DWORD) ((SecondsLeftUntilFlip/TargetSecondsPerFrame) * (real32) ExpectedSoundBytesPerFrame);
						
						DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

						DWORD SafeWriteCursor = (WriteCursor + SoundOutput.SafetyBytes);
						if(SafeWriteCursor < PlayCursor)
						{
							SafeWriteCursor += SoundOutput.SecondaryBufferSize;
						}
						Assert(SafeWriteCursor >= PlayCursor);
						SafeWriteCursor += SoundOutput.SafetyBytes;

						bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

						DWORD TargetCursor = 0;
						if(AudioCardIsLowLatency)
						{
							TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
						}
						else
						{
							TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
						}
						TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);

						DWORD BytesToWrite = 0;						
						if(ByteToLock > TargetCursor)
						{
							BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - ByteToLock;
						}

						game_output_sound_buffer SoundBuffer = {};
						SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
						SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
						SoundBuffer.Samples = Samples;
						
						if(Game.GetSoundSamples)
						{
							Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
						}

						#if 0
						//#if HANDMADE_INTERNAL
						
						win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
						Marker->OutputPlayCursor = PlayCursor;
						Marker->OutputWriteCursor = WriteCursor;
						Marker->OutputLocation = ByteToLock;
						Marker->OutputByteCount = BytesToWrite;
						Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;
						
						DWORD UnwrappedWriteCursor = WriteCursor;
						if(UnwrappedWriteCursor < PlayCursor)
						{
							UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
						}
						AudioLatencyBytes = WriteCursor - PlayCursor; // Minimum possible audio latency
						AudioLatencySeconds =
							((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) /
							(real32)SoundOutput.SamplesPerSecond;

						local_persist uint32 AudioDebugCount = 0;
						if(AudioDebugCount % 64 == 0)
						{
							char TextBuffer[256];
							sprintf_s(TextBuffer, sizeof(TextBuffer),
							          "BTL: %u | TC: %u | BTW: %u - PC: %u, WC: %u\nDELTA: %u (%fs)\n",
							          ByteToLock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor,
							          AudioLatencyBytes, AudioLatencySeconds);
							OutputDebugStringA(TextBuffer);
						}
						++AudioDebugCount;
						
						#endif // HANDMADE_INTERNAL

						Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
					}
					else
					{
						SoundIsValid = false;
					}

					//
					// NOTE(Douglas): Cronometragem
					//
					LARGE_INTEGER WorkCounter = Win32GetWallClock();
					real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

					// TODO(Douglas): Ainda não foi testado!! Provavelmente está bugado!!!!
					real32 SecondsElapsedForFrame = WorkSecondsElapsed;
					if(SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						if(SleepIsGranular)
						{
							DWORD SleepMS = (DWORD) (1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
							if(SleepMS > 0)
							{
								Sleep(SleepMS);
							}
						}

						real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
						if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							// TODO(Douglas): Diagnóstico (MISSED SLEEP)
						}

						while(SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
							                                                Win32GetWallClock());
						}
					}
					else
					{
						// TODO(Douglas): MISSED FRAME RATE!
						// TODO(Douglas): Diagnóstico (ultrapassou a taxa de atualização)
					}

					LARGE_INTEGER EndCounter = Win32GetWallClock();
					real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
					LastCounter = EndCounter;
					
					// NOTE(Douglas): Exibindo o quadro depois de esperar o memonto certo
					win32_window_dimensions Dimension = Win32GetWindowDimension(Window);

					HDC DeviceContext = GetDC(Window);
					Win32DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer, Dimension.Width, Dimension.Height);
					ReleaseDC(Window, DeviceContext);

					FlipWallClock = Win32GetWallClock();


					#if 0
					//#if HANDMADE_INTERNAL
					// NOTE(Douglas): Esse código é pra Debug!!!
					{

						DWORD DebugPlayCursor;
						DWORD DebugWriteCursor;
						if(GlobalSecondaryBuffer->GetCurrentPosition(&DebugPlayCursor, &DebugWriteCursor) == DS_OK)
						{
							Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
							win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
							Marker->FlipPlayCursor = DebugPlayCursor;
							Marker->FlipWriteCursor = DebugWriteCursor;
						}
					}
					#endif // HANDMADE_INTERNAL

					//
					// NOTE(Douglas): Trocando os estados dos controles
					//
					game_input *InputTemp = NewInput;
					NewInput = OldInput;
					OldInput = InputTemp;
					// TODO(Douglas): Deveríamos limpar isso aqui?

					uint64 EndCycleCount = __rdtsc();
					uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
					LastCycleCount = EndCycleCount;

					#if 1
					//int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;

					// CounterElapsed / GlobalPerfCountFrequency = segundos por frame
					// ((1000.0f * (real32) CounterElapsed) / (real32) GlobalPerfCountFrequency);
					real32 FPS = 0.0f;//(real32) GlobalPerfCountFrequency / (real32) CounterElapsed; // counts_per_second / counts_elapsed
					real32 MegaCyclesPerFrame = (real32) CyclesElapsed / (1000.0f * 1000.0f);
					real32 CurrentCPUGhz = (FPS * MegaCyclesPerFrame) / 1000.0f;

					static uint64 DebugStringDelay = 0;
					if(DebugStringDelay % 64 == 0)
					{
						char DebugBuffer[256];
						sprintf_s(DebugBuffer, sizeof(DebugBuffer),
						          "\n%.02f (ms/f) | %.02f (f/s) | %.02f (million instructions - MCPF) | %.02f Ghz\n\n",
						          MSPerFrame, FPS, MegaCyclesPerFrame, CurrentCPUGhz);
						OutputDebugStringA((LPSTR) DebugBuffer);
					}
					++DebugStringDelay;
					#endif

					#if 0
					// #if HANDMADE_INTERNAL
					++DebugTimeMarkerIndex;
					if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
					{
						DebugTimeMarkerIndex = 0;
					}
					#endif // HANDMADE_INTERNAL
					}
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

