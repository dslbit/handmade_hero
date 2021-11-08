#if !defined(HANDMADE_H)

/*
	NOTE(Douglas):

	HANDMADE_INTERNAL:
		0 - Build para lançamento público
		1 - Build apenas para desenvolvedor

	HANDMADE_SLOW:
		0 - Nenhum código lento permitido!
		1 - Código lento é bem-vindo.
*/

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

// TODO(Douglas): Completar o macro "Assert"
#if HANDMADE_SLOW
	#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0; }
#else
	#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32
SafeTruncateUInt64(uint64 Value)
{
	uint32 Result;
	// TODO(Douglas): "Defines" para valores máximos
	Assert(Value <= 0xFFFFFFFF); // UInt32Max
	Result = (uint32) Value;
	return(Result);
}

// TODO(Douglas): Swap, min, max ... macros??

// NOTE(Douglas): Estrutura informacional que será passada a cada iteração do jogo.
// Ela será passada sempre que precisarmos de algum serviço do sistema operacional,
// porque alguns S.O. não fazem um bom trabalho ao informar em qual "thread" está a
// execução do programa em um determinado momento.
struct thread_context
{
	int Placeholder;
};

//
// NOTE(Douglas): Serviços que a plataforma fornece para o jogo
//
#if HANDMADE_INTERNAL

/*
	IMPORTANT(Douglas):

	Isso não vai fazer nada no código do lançamento - eles estão bloqueando e
	a escrita não é protegida contra perda de dados.
*/

struct debug_read_file_result
{
	uint32 ContentsSize;
	void *Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *FileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *Thread, char *FileName, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif //HANDMADE_INTERNAL



//
// NOTE(Douglas): Serviços que o jogo fornece para a plataforma
// (Isso pode expandir no futuro, por exemplo quando quisermos,
// separar o áudio em um Thread diferente.)
//

// TODO(Douglas): No futuro, a renderização _especificamente_ vai se tornar uma
// abstração de três partes.
struct game_offscreen_buffer
{
	// NOTE(Douglas): Pixels são sempre 32-bits, Ordem de memória: bb gg rr xx
	void *Memory;
	int32 Width;
	int32 Height;
	int32 BytesPerPixel;
	int32 Pitch;
};

struct game_output_sound_buffer
{
  int32 SampleCount;
  int32 SamplesPerSecond;
  int16 *Samples;
};

struct game_button_state
{
	int32 HalfTransitionCount; // NOTE(Douglas): Revisar isso
	bool32 EndedDown;
};

struct game_controller_input
{
	bool32 IsConnected;
	bool32 IsAnalog;

	real32 StickAverageX;
	real32 StickAverageY;

	union
	{
		game_button_state Buttons[12];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Start;
			game_button_state Back;

			// NOTE(Douglas): Adicionar novos botões acima desse comentário
			game_button_state Terminator;
		};
	};
};

struct game_input
{
	game_controller_input Controllers[5];
	
	game_button_state MouseButtons[5];
	int32 MouseX, MouseY, MouseZ;

	real32 SecondsToAdvanceOverUpdate;
};
inline game_controller_input *GetController(game_input *Input, uint32 ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input *Result = &Input->Controllers[ControllerIndex];
	return(Result);
}

struct game_memory
{
	bool32 IsInitialized;
	
	uint64 PermanentStorageSize;
	void *PermanentStorage; // NOTE(Douglas): é NECESSÁRIO ser limpada com zeros

	uint64 TransientStorageSize;
	void *TransientStorage; // NOTE(Douglas): é NECESSÁRIO ser limpada com zeros

	debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
	debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
	debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// NOTE(Douglas): No momento esta função precisa ser muito rápida, não pode ser mais de 1ms.
// TODO(Douglas): Reduzir a pressão na performace dessa função (medindo ou pedindo informações, etc.).
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_output_sound_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

struct game_state
{
};

#define HANDMADE_H
#endif
