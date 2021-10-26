#if !defined(HANDMADE_H)

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

// Esta função precisa de três coisas:
// - Cronometragem (timing)
// - Entrada (controle e teclado)
// - Buffer do Mapa de Bits (Bitmap) (para usar)
// - Buffer de Áudio (para usar)
internal void GameUpdateAndRender(game_offscreen_buffer *Buffer,
                                  int BlueOffset,
                                  int GreenOffset);


#define HANDMADE_H
#endif
