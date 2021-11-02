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

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *FileName, uint32 MemorySize, void *Memory);
#endif



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
	// TODO(Douglas): Inserir o valor do clock aqui (Timers).
	game_controller_input Controllers[5];
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
};

struct game_state
{
	int32 GreenOffset;
	int32 BlueOffset;
	int32 ToneHz;
};

// Esta função precisa de três coisas:
// - Cronometragem (timing)
// - Entrada (controle e teclado)
// - Buffer do Mapa de Bits (Bitmap) (para usar)
// - Buffer de Áudio (para usar)
internal void GameUpdateAndRender(game_memory *Memory,
                                  game_input *Input,
                                  game_offscreen_buffer *Buffer);

// NOTE(Douglas): No momento esta função precisa ser muito rápida, não pode ser mais de 1ms.
// TODO(Douglas): Reduzir a pressão na performace dessa função (medindo ou pedindo informações, etc.).
internal void GameGetSoundSamples(game_memory *Memory, game_output_sound_buffer *SoundBuffer);


#define HANDMADE_H
#endif
