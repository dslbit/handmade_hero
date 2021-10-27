#if !defined(HANDMADE_H)

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
	game_controller_input Controllers[4];
};

// Esta função precisa de três coisas:
// - Cronometragem (timing)
// - Entrada (controle e teclado)
// - Buffer do Mapa de Bits (Bitmap) (para usar)
// - Buffer de Áudio (para usar)
internal void GameUpdateAndRender(game_input *Input,
                                  game_offscreen_buffer *Buffer,
                                  game_output_sound_buffer *SoundBuffer);


#define HANDMADE_H
#endif
