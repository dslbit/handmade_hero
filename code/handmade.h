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

// TODO(Douglas): Essas coisas deveriam ser 64bits?
#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// TODO(Douglas): Swap, min, max ... macros??

//
// NOTE(Douglas): Serviços que a plataforma fornece para o jogo
//



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
	int32 HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input
{
	bool32 IsAnalog;

	real32 StartX;
	real32 StartY;

	real32 MinX;
	real32 MinY;

	real32 MaxX;
	real32 MaxY;

	real32 EndX;
	real32 EndY;

	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};
};

struct game_input
{
	// TODO(Douglas): Inserir o valor do clock aqui (Timers).
	game_controller_input Controllers[4];
};

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
                                  game_offscreen_buffer *Buffer,
                                  game_output_sound_buffer *SoundBuffer);


#define HANDMADE_H
#endif
