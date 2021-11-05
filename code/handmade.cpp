#include "handmade.h"

internal void
GameOutputSound(game_output_sound_buffer *SoundBuffer,
                game_state *GameState,
                int32 ToneHz)
{
  int16 ToneVolume = 5000;
  int32 WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  int16 *SampleOut = SoundBuffer->Samples;
  for(int32 SampleIndex = 0;
      SampleIndex < SoundBuffer->SampleCount;
      ++SampleIndex)
  {
  	#if 1
    real32 SineValue = sinf(GameState->tSine);
    int16 SampleValue = (int16) (SineValue * ToneVolume);
    #else
    int16 SampleValue = 0;
    #endif
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    
    GameState->tSine += (2.0f * Pi32) / (real32) WavePeriod;
    if(GameState->tSine > (2.0f * Pi32))
    {
    	GameState->tSine -= (2.0f * Pi32);
    }
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
			uint8 Red = 0;//(uint8) (X*Y/Buffer->Width);
			uint8 Blue = (uint8) (X + BlueOffset);
			uint8 Green = (uint8) (Y + GreenOffset);

			*Pixel++ = (Red << 16) | (Green << 16) | Blue;
		}

		Row += Buffer->Pitch; // (no momento) é o mesmo que dizer "Row = (uint8 *)Pixel;" pois a memória não tem espaçamento.
	}
}

internal void
RenderPlayer(game_offscreen_buffer *Buffer,
           int PlayerX, int PlayerY)
{
	uint8 *EndOfBuffer = (uint8 *) Buffer->Memory +	Buffer->BytesPerPixel * Buffer->Width * Buffer->Height;

	uint32 Color = 0xFFFFFFFF;
	int Top = PlayerY;
	int Bottom = PlayerY+10;
	for(int X = PlayerX;
	    X < PlayerX+10;
	    ++X)
	{
		uint8 *Pixel = ((uint8 *)Buffer->Memory +
		                X * Buffer->BytesPerPixel +
		                Top * Buffer->Pitch);
		for(int Y = Top;
		    Y < Bottom;
		    ++Y)
		{
			if((Pixel >= Buffer->Memory) && (Pixel <= EndOfBuffer))
			{
				*(uint32 *)Pixel = Color;
			}
			Pixel += Buffer->Pitch;
		}
	}
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(ArrayCount(Input->Controllers[0].Buttons) == 
	       (&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *) Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		char *FileName = __FILE__;
		debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(FileName);
		if(File.Contents)
		{
			Memory->DEBUGPlatformWriteEntireFile("test.txt", File.ContentsSize, File.Contents);
			Memory->DEBUGPlatformFreeFileMemory(File.Contents);
		}

		GameState->ToneHz = 512;
		GameState->tSine = 0.0f;
		GameState->PlayerX = 100;
		GameState->PlayerY = 100;

		// TODO(Douglas): Talvez seja mais apropriado fazer isso aqui na plataforma
		Memory->IsInitialized = true;
	}


	for(int ControllerIndex = 0;
	    ControllerIndex < ArrayCount(Input->Controllers);
	    ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if(Controller->IsAnalog)
		{
			// NOTE(Douglas): Movimentação analógica
			GameState->BlueOffset += (int) (4.0f * (Controller->StickAverageX));
			GameState->ToneHz = 512 + (int) (256.0f * (Controller->StickAverageY));
		}
		else
		{
			if(Controller->MoveLeft.EndedDown)
			{
				//GameState->BlueOffset -= 3;
			}
			if(Controller->MoveRight.EndedDown)
			{
				//GameState->BlueOffset += 3;
			}
			// NOTE(Douglas): Movimentação digital
		}

		GameState->PlayerX += (int) (14.0f * Controller->StickAverageX);
		GameState->PlayerY -= (int) (14.0f * Controller->StickAverageY);
		if(GameState->tJump > 0)
		{
			GameState->PlayerY += (int) (5.0f * sinf(Pi32*GameState->tJump));
		}

		// Input.AButtonEndedDown;
		// Input.AButtonHalfTransitionCount;
		if(Controller->ActionDown.EndedDown)
		{
			//GameState->GreenOffset += 1;
			GameState->tJump = 2.0f;
		}
		GameState->tJump -= 0.025f;
	}

  // TODO(Douglas): Permitir índices de amostras de som aqui, no futuro, para ter opções de plataformas robusta
  
	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
	RenderPlayer(Buffer, GameState->PlayerX, GameState->PlayerY);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *) Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState, GameState->ToneHz);
}
