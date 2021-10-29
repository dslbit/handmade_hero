#include "handmade.h"

internal void
GameOutputSound(game_output_sound_buffer *SoundBuffer,
                int32 ToneHz)
{
  local_persist real32 tSine;
  int16 ToneVolume = 3000;
  int32 WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  int16 *SampleOut = SoundBuffer->Samples;
  for(uint32 SampleIndex = 0;
      SampleIndex < SoundBuffer->SampleCount;
      ++SampleIndex)
  {
    real32 SineValue = sinf(tSine);
    int16 SampleValue = (int16) (SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    tSine += (2.0f * Pi32) / (real32) WavePeriod;
  }
}

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;

	for(int Y = 0;
	    Y < Buffer->Height;
	    Y++)
	{
		uint32 *Pixel = (uint32 *)Row;

		for(int X = 0;
		    X < Buffer->Width;
		    X++)
		{
			uint8 Red = 0;
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);

			*Pixel++ = (Red << 16) | (Green << 8) | Blue;
		}

		Row += Buffer->Pitch; // (no momento) é o mesmo que dizer "Row = (uint8 *)Pixel;" pois a memória não tem espaçamento.
	}
}

internal void
GameUpdateAndRender(game_memory *Memory,
                    game_input *Input,
                    game_offscreen_buffer *Buffer,
                    game_output_sound_buffer *SoundBuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *) Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		char *FileName = __FILE__;
		debug_read_file_result File = DEBUGPlatformReadEntireFile(FileName);
		if(File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.txt", File.ContentsSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}

		GameState->ToneHz = 256;

		// TODO(Douglas): Talvez seja mais apropriado fazer isso aqui na plataforma
		Memory->IsInitialized = true;
	}



	game_controller_input *Input0 = &Input->Controllers[0];
	if(Input0->IsAnalog)
	{
		// NOTE(Douglas): Movimentação analógica
		GameState->BlueOffset += (int) (4.0f * (Input0->EndX));
		GameState->ToneHz = 256 + (int) (128.0f * (Input0->EndY));
	}
	else
	{
		// NOTE(Douglas): Movimentação digital
	}

	// Input.AButtonEndedDown;
	// Input.AButtonHalfTransitionCount;
	if(Input0->Down.EndedDown)
	{
		GameState->GreenOffset += 1;
	}

  // TODO(Douglas): Permitir índices de amostras de som aqui, no futuro, para ter opções de plataformas robusta
  GameOutputSound(SoundBuffer, GameState->ToneHz);
	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}
