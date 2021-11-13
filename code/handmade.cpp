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

inline tile_map *
GetTileMap(world *World, int32 TileMapX, int32 TileMapY)
{
	tile_map *TileMap = 0;
	if(TileMapX >= 0 && TileMapX < World->TileMapCountX &&
	   TileMapY >= 0 && TileMapY < World->TileMapCountY)
	{
		TileMap = &World->TileMaps[ (TileMapY*World->TileMapCountX) + TileMapX];
	}
	return(TileMap);
}


inline uint32
GetTileValueUnchecked(world *World, tile_map *TileMap, int32 TileX, int32 TileY)
{
	Assert(TileMap);
	Assert((TileX >= 0) && (TileX < World->CountX) &&
	       (TileY >= 0) && (TileY < World->CountY));
	uint32 TileMapValue = TileMap->Tiles[ (TileY*World->CountX) + TileX];
	return(TileMapValue);
}

inline bool32
IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TestTileX, int32 TestTileY)
{
	bool32 Empty = false;

	if(TileMap)
	{
		if(TestTileX >= 0 && TestTileX < World->CountX &&
		   TestTileY >= 0 && TestTileY < World->CountY)
		{
			int32 TileMapValue = GetTileValueUnchecked(World, TileMap, TestTileX, TestTileY);
			Empty = (TileMapValue == 0);
		}
	}

	return(Empty);
}

inline void
RecanonicalizeCoord(world *World,
                    int32 TileCount,
                    int32 *TileMap,
                    int32 *Tile,
                    real32 *TileRel)
{
	//
	// TODO(douglas): Precisamos de uma solução melhor do que dividir/multiplicar para
	// o método abaixo, porque o método atual pode arredondar o valor para o azulejo
	// anterior (do que está no momento).
	//

	int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * World->TileSideInMeters;

	Assert(*TileRel >= 0);	
	// TODO(douglas): Arrumar o calculo com ponto flutuante.
	Assert(*TileRel <= World->TileSideInMeters);

	if(*Tile < 0)
	{
		*Tile = *Tile + TileCount;
		--*TileMap;
	}

	if(*Tile >= TileCount)
	{
		*Tile = *Tile - TileCount;
		++*TileMap;
	}
}

inline canonical_position
RecanonicalizePosition(world *World,
                       canonical_position Pos)
{
	canonical_position Result = Pos;

	RecanonicalizeCoord(World, World->CountX, &Result.TileMapX, &Result.TileX, &Result.TileRelX);
	RecanonicalizeCoord(World, World->CountY, &Result.TileMapY, &Result.TileY, &Result.TileRelY);

	return(Result);
}

inline bool32
IsWorldPointEmpty(world *World,
                  canonical_position CanPos)
{
	bool32 Empty = false;

	tile_map *TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
	Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);
	
	return(Empty);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(ArrayCount(Input->Controllers[0].Buttons) == 
	       (&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	#define TILE_MAP_COUNT_X 17
	#define TILE_MAP_COUNT_Y 9
	uint32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
		{1, 0, 0, 0,  0, 0, 1, 0,  0,  0, 0, 0, 0,  0, 1, 0, 1},
		{1, 0, 0, 0,  0, 0, 1, 1,  1,  1, 1, 1, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 1, 0,  0,  0, 0, 0, 0,  0, 1, 0, 1},
		{1, 0, 0, 0,  0, 0, 1, 0,  1,  1, 1, 1, 0,  0, 1, 0, 0},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 1, 0, 1},
		{1, 1, 0, 0,  0, 0, 1, 0,  0,  0, 0, 1, 0,  0, 1, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  0,  0, 0, 1, 1,  1, 1, 1, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
	};

	uint32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 0},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
	};

	uint32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
	};

	uint32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
		{1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
	};

	tile_map TileMaps[2][2];
	TileMaps[0][0].Tiles = (uint32 *) Tiles00;
	TileMaps[0][1].Tiles = (uint32 *) Tiles10;
	TileMaps[1][0].Tiles = (uint32 *) Tiles01;
	TileMaps[1][1].Tiles = (uint32 *) Tiles11;

	world World;
	World.TileMapCountX = 2;
	World.TileMapCountY = 2;
	World.CountX = TILE_MAP_COUNT_X;
	World.CountY = TILE_MAP_COUNT_Y;
	World.TileSideInMeters = 1.4f; // TODO(douglas): começar a usar isso
	World.TileSideInPixels = 60;
	World.MetersToPixel = ((real32) World.TileSideInPixels / (real32) World.TileSideInMeters);
	World.UpperLeftX =  -((real32)World.TileSideInPixels / 2);
	World.UpperLeftY = 0;

	real32 PlayerHeight = World.TileSideInMeters;
	real32 PlayerWidth = 0.75f * PlayerHeight;

	World.TileMaps = (tile_map *) TileMaps;

	game_state *GameState = (game_state *) Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		GameState->PlayerP.TileMapX = 0;
		GameState->PlayerP.TileMapY = 0;
		GameState->PlayerP.TileX = 3;
		GameState->PlayerP.TileY = 3;
		GameState->PlayerP.TileRelX = 0;
		GameState->PlayerP.TileRelY = 0;

		// TODO(Douglas): Talvez seja mais apropriado fazer isso aqui na plataforma
		Memory->IsInitialized = true;
	}

	tile_map *TileMap = GetTileMap(&World, GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY);
	Assert(TileMap);

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
				dPlayerY = -1.0f;
			}
			if(Controller->MoveDown.EndedDown)
			{
				dPlayerY = 1.0f;
			}
			if(Controller->MoveLeft.EndedDown)
			{
				dPlayerX = -1.0f;
			}
			if(Controller->MoveRight.EndedDown)
			{
				dPlayerX = 1.0f;
			}

			dPlayerX *= 10.0f;
			dPlayerY *= 10.0f;

			// TODO(douglas): Movimentos na diagonal serão mais rápidos, arrumar isso quando tivermos vetores :)
			canonical_position NewPlayerP = GameState->PlayerP;
			NewPlayerP.TileRelX += (Input->dtForFrame * dPlayerX);
			NewPlayerP.TileRelY += (Input->dtForFrame * dPlayerY);
			NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);

			// TODO(douglas): DEBUGAR ISSO AQUI, PORQUE O PLAYER N CONSEGUE SE MOVIMENTAR?!?!?!?!?

			// para hit test na esquerda
			canonical_position PlayerLeft = NewPlayerP;
			PlayerLeft.TileRelX -= (0.5f * PlayerWidth);
			PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);

			// para hit test na direita
			canonical_position PlayerRight = NewPlayerP;
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

	for(int Row = 0;
	    Row < 9;
	    ++Row)
	{
		for(int Column = 0;
		    Column < 17;
		    ++Column)
		{
			int TileID = GetTileValueUnchecked(&World, TileMap, Column, Row);
			real32 gray = 0.5f;
			if(TileID == 1)
			{
				gray = 1.0f;
			}

			if(Column == GameState->PlayerP.TileX &&
			   Row == GameState->PlayerP.TileY)
			{
				gray = 0;
			}

			real32 MinX = World.UpperLeftX + ((real32) Column * World.TileSideInPixels);
			real32 MinY = World.UpperLeftY + ((real32) Row * World.TileSideInPixels);
			real32 MaxX = MinX + World.TileSideInPixels;
			real32 MaxY = MinY + World.TileSideInPixels;

			DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, gray, gray, gray);
		}
	}

	real32 PlayerR = 0.0f;
	real32 PlayerG = 1.0f;
	real32 PlayerB = 0.7f;
	real32 PlayerLeft = World.UpperLeftX + World.TileSideInPixels*GameState->PlayerP.TileX +
		World.MetersToPixel * GameState->PlayerP.TileRelX - (0.5f * PlayerWidth * World.MetersToPixel);
	real32 PlayerTop = World.UpperLeftY + World.TileSideInPixels*GameState->PlayerP.TileY +
		World.MetersToPixel * GameState->PlayerP.TileRelY - PlayerHeight * World.MetersToPixel;

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
