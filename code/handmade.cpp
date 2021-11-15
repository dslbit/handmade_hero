#include "handmade.h"
#include "handmade_intrinsics.h"

internal void
GameOutputSound(game_output_sound_buffer *SoundBuffer,
                game_state *GameState,
                int32 ToneHz)
{
  int16 ToneVolume = 1000;
  int32 WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  int16 *SampleOut = SoundBuffer->Samples;
  for(int32 SampleIndex = 0;
      SampleIndex < SoundBuffer->SampleCount;
      ++SampleIndex)
  {
  	#if 0
    real32 SineValue = sinf(GameState->tSine);
    int16 SampleValue = (int16) (SineValue * ToneVolume);
    #else
    int16 SampleValue = 0;
    #endif
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    
    #if 0
    GameState->tSine += (2.0f * Pi32) / (real32) WavePeriod;
    if(GameState->tSine > (2.0f * Pi32))
    {
    	GameState->tSine -= (2.0f * Pi32);
    }
    #endif
  }
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer,
              real32 RealMinX, real32 RealMinY,
              real32 RealMaxX, real32 RealMaxY,
              real32 R, real32 G, real32 B)
{
	// NOTE(Douglas): Os retângulos serão preenchidos não incluindo o valor máximo.
	// Por esse motivo, o código da verificação do limite mínimo e máximo permite que
	// os valores máximos sejam do mesmo tamanho do buffer.

	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMaxX);
	int32 MaxY = RoundReal32ToInt32(RealMaxY);

	if(MinX < 0)
	{
		MinX = 0;
	}

	if(MinY < 0)
	{
		MinY = 0;
	}

	if(MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}

	if(MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	uint32 Color = ((RoundReal32ToUInt32(R * 255.0f) << 16) |
	                (RoundReal32ToUInt32(G * 255.0f) << 8)  |
	                (RoundReal32ToUInt32(B * 255.0f) << 0));

	// NOTE(Douglas): Pré-avançando o ponteiro para a memória do pixel. Abaixo, apontamos para o topo
	// esquerdo do retângulo.
	uint8 *Row = ((uint8 *)Buffer->Memory +
	              (MinY * Buffer->Pitch) +
	              (MinX * Buffer->BytesPerPixel));

	for(int Y = MinY;
	    Y < MaxY;
	    ++Y)
	{
		uint32 *Pixel = (uint32 *) Row;
		for(int X = MinX;
		    X < MaxX;
		    ++X)
		{
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}

inline tile_chunk *
GetTileChunk(world *World,
             int32 TileChunkX,
             int32 TileChunkY)
{
	tile_chunk *TileChunk = 0;
	if(TileChunkX >= 0 && TileChunkX < World->TileChunkCountX &&
	   TileChunkY >= 0 && TileChunkY < World->TileChunkCountY)
	{
		TileChunk = &World->TileChunks[ (TileChunkY*World->TileChunkCountX) + TileChunkX];
	}
	return(TileChunk);
}


inline uint32
GetTileValueUnchecked(world *World,
                      tile_chunk *TileChunk,
                      uint32 TileX,
                      uint32 TileY)
{
	Assert(TileChunk);
	Assert(TileX < World->ChunkDim);
	Assert(TileY < World->ChunkDim);
	uint32 TileChunkValue = TileChunk->Tiles[ (TileY*World->ChunkDim) + TileX];
	return(TileChunkValue);
}

inline uint32
GetTileValue(world *World,
             tile_chunk *TileChunk,
             int32 TestTileX,
             int32 TestTileY)
{
	uint32 TileChunkValue = 0;

	if(TileChunk)
	{
		TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);
	}

	return(TileChunkValue);
}

inline void
RecanonicalizeCoord(world *World,
                    uint32 *Tile,
                    real32 *TileRel)
{
	//
	// TODO(douglas): Precisamos de uma solução melhor do que dividir/multiplicar para
	// o método abaixo, porque o método atual pode arredondar o valor para o azulejo
	// anterior (do que está no momento).
	//

	// NOTE(Douglas): Talvez não precisamos verificar se o jogador cruzou o limite máximo
	// do mundo, assim ele vai aparecer do lado oposto do mundo.
	int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * World->TileSideInMeters;

	Assert(*TileRel >= 0);	
	// TODO(douglas): Arrumar o calculo com ponto flutuante.
	Assert(*TileRel <= World->TileSideInMeters);
}

inline world_position
RecanonicalizePosition(world *World,
                       world_position Pos)
{
	world_position Result = Pos;

	RecanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
	RecanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);

	return(Result);
}

inline tile_chunk_position
GetChunkPositionFor(world *World,
                    uint32 AbsTileX,
                    uint32 AbsTileY)
{
	tile_chunk_position Result;

	Result.TileChunkX = AbsTileX >> World->ChunkShift;
	Result.TileChunkY = AbsTileY >> World->ChunkShift;
	Result.RelTileX = AbsTileX & World->ChunkMask;
	Result.RelTileY = AbsTileY & World->ChunkMask;

	return(Result);
}

inline bool32
GetTileValue(world *World,
             uint32 AbsTileX,
             uint32 AbsTileY)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(World, AbsTileX, AbsTileY);
	tile_chunk *TileMap = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
 	uint32 TileChunkValue = GetTileValue(World, TileMap, ChunkPos.RelTileX, ChunkPos.RelTileY);

	return(TileChunkValue);
}

inline uint32
IsWorldPointEmpty(world *World,
             world_position CanPos)
{
 	uint32 TileChunkValue = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
	bool32 Empty = (TileChunkValue == 0);
	
	return(Empty);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(ArrayCount(Input->Controllers[0].Buttons) == 
	       (&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	#define TILE_MAP_COUNT_X 256
	#define TILE_MAP_COUNT_Y 256
	uint32 TempChunk[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
		{1, 0, 0, 0,  0, 0, 1, 0,  0,  0, 0, 0, 0,  0, 1, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 1, 1,  1,  1, 1, 1, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 1, 0,  0,  0, 0, 0, 0,  0, 1, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 1, 0,  1,  1, 1, 1, 0,  0, 1, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 1, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 1, 0, 0,  0, 0, 1, 0,  0,  0, 0, 1, 0,  0, 1, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  0,  0, 0, 1, 1,  1, 1, 1, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
	};

	world World;

	// NOTE(douglas): está configurado para os chunks dos azulejos serem 256x256
	World.ChunkShift = 8;
	World.ChunkMask = (1 << World.ChunkShift) - 1;
	World.ChunkDim = 256;

	tile_chunk TileChunk;
	TileChunk.Tiles = (uint32 *) TempChunk;
	World.TileChunks = &TileChunk;

	World.TileChunkCountX = 1;
	World.TileChunkCountY = 1;
	World.TileSideInMeters = 1.4f; // TODO(douglas): começar a usar isso
	World.TileSideInPixels = 60;
	World.MetersToPixel = ((real32) World.TileSideInPixels / (real32) World.TileSideInMeters);

	real32 PlayerHeight = World.TileSideInMeters;
	real32 PlayerWidth = 0.75f * PlayerHeight;

	real32 LowerLeftX =  -((real32)World.TileSideInPixels / 2);
	real32 LowerLeftY = (real32) Buffer->Height;


	game_state *GameState = (game_state *) Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		GameState->PlayerP.AbsTileX = 3;
		GameState->PlayerP.AbsTileY = 3;
		GameState->PlayerP.TileRelX = 0;
		GameState->PlayerP.TileRelY = 0;

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
		}
		else
		{
			// NOTE(Douglas): Movimentação digital
			real32 dPlayerX = 0.0f;
			real32 dPlayerY = 0.0f;

			if(Controller->MoveUp.EndedDown)
			{
				dPlayerY = 1.0f;
			}
			if(Controller->MoveDown.EndedDown)
			{
				dPlayerY = -1.0f;
			}
			if(Controller->MoveLeft.EndedDown)
			{
				dPlayerX = -1.0f;
			}
			if(Controller->MoveRight.EndedDown)
			{
				dPlayerX = 1.0f;
			}

			dPlayerX *= 4.0f;
			dPlayerY *= 4.0f;

			// TODO(douglas): Movimentos na diagonal serão mais rápidos, arrumar isso quando tivermos vetores :)
			world_position NewPlayerP = GameState->PlayerP;
			NewPlayerP.TileRelX += (Input->dtForFrame * dPlayerX);
			NewPlayerP.TileRelY += (Input->dtForFrame * dPlayerY);
			NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);

			// para hit test na esquerda
			world_position PlayerLeft = NewPlayerP;
			PlayerLeft.TileRelX -= (0.5f * PlayerWidth);
			PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);

			// para hit test na direita
			world_position PlayerRight = NewPlayerP;
			PlayerRight.TileRelX += (0.5f * PlayerWidth);
			PlayerRight = RecanonicalizePosition(&World, PlayerRight);

			if(IsWorldPointEmpty(&World, NewPlayerP) &&
			   IsWorldPointEmpty(&World, PlayerLeft) &&
			   IsWorldPointEmpty(&World, PlayerRight))
			{
				GameState->PlayerP = NewPlayerP;
			}
		}
	}

	DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height,
	              1.0f, 0.0f, 1.0f);

	real32 CenterX = 0.5f * (real32)Buffer->Width;
	real32 CenterY = 0.5f * (real32)Buffer->Height;

	for(int32 RelRow = -10;
	    RelRow < 10;
	    ++RelRow)
	{
		for(int32 RelColumn = -20;
		    RelColumn < 20;
		    ++RelColumn)
		{
			uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;
			uint32 Row = GameState->PlayerP.AbsTileY + RelRow;
			uint32 TileID = GetTileValue(&World, Column, Row);
			
			real32 gray = 0.5f;
			if(TileID == 1)
			{
				gray = 1.0f;
			}

			if(Column == GameState->PlayerP.AbsTileX &&
			   Row == GameState->PlayerP.AbsTileY)
			{
				gray = 0;
			}

			real32 MinX = CenterX + ((real32) RelColumn * World.TileSideInPixels);
			real32 MinY = CenterY - ((real32) RelRow * World.TileSideInPixels);
			real32 MaxX = MinX + World.TileSideInPixels;
			real32 MaxY = MinY - World.TileSideInPixels;

			DrawRectangle(Buffer, MinX, MaxY, MaxX, MinY, gray, gray, gray);
		}
	}

	real32 PlayerR = 0.0f;
	real32 PlayerG = 1.0f;
	real32 PlayerB = 0.7f;
	real32 PlayerLeft = CenterX + World.MetersToPixel * GameState->PlayerP.TileRelX - (0.5f * PlayerWidth * World.MetersToPixel);
	real32 PlayerTop = CenterY - World.MetersToPixel * GameState->PlayerP.TileRelY - PlayerHeight * World.MetersToPixel;

	DrawRectangle(Buffer, PlayerLeft, PlayerTop,
	              PlayerLeft + PlayerWidth * World.MetersToPixel,
	              PlayerTop + PlayerHeight * World.MetersToPixel,
	              PlayerR, PlayerG, PlayerB);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *) Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState, 400);
}


/*
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
*/
