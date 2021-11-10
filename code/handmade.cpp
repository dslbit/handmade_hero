#include "handmade.h"

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

inline int32
RoundReal32ToInt32(real32 Real32)
{
	// TODO(douglas): intrinsica??? Isso aqui precisa de anteção do futuro.
	int32 Result = (int32) (Real32 + 0.5f);
	return(Result);
}

inline uint32
RoundReal32ToUInt32(real32 Real32)
{
	uint32 Result = (uint32) (Real32 + 0.5f);
	return(Result);
}

inline int32
TruncateReal32ToInt32(real32 Real32)
{
	int32 Result = (int32) Real32;
	return(Result);
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
GetTileValueUnchecked(tile_map *TileMap, int32 TileX, int32 TileY)
{
	uint32 TileMapValue = TileMap->Tiles[ (TileY*TileMap->CountX) + TileX];
	return(TileMapValue);
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, real32 TestX, real32 TestY)
{
	bool32 Empty = false;

	int32 PlayerTileX = TruncateReal32ToInt32((TestX - TileMap->UpperLeftX) / TileMap->TileWidth);
	int32 PlayerTileY = TruncateReal32ToInt32((TestY - TileMap->UpperLeftY) / TileMap->TileHeight);
	
	if(PlayerTileX >= 0 && PlayerTileX < TileMap->CountX &&
	   PlayerTileY >= 0 && PlayerTileY < TileMap->CountY)
	{
		int32 TileMapValue = GetTileValueUnchecked(TileMap, PlayerTileX, PlayerTileY);
		Empty = (TileMapValue == 0);
	}

	return(Empty);
}

internal bool32
IsWorldPointEmpty(world *World, int32 TileMapX, int32 TileMapY, int32 TestX, int32 TestY)
{
	bool32 Empty = false;

	tile_map *TileMap = GetTileMap(World, TileMapX, TileMapY);
	if(TileMap)
	{
		int32 PlayerTileX = TruncateReal32ToInt32((TestX - TileMap->UpperLeftX) / TileMap->TileWidth);
		int32 PlayerTileY = TruncateReal32ToInt32((TestY - TileMap->UpperLeftY) / TileMap->TileHeight);
		
		if(PlayerTileX >= 0 && PlayerTileX < TileMap->CountX &&
		   PlayerTileY >= 0 && PlayerTileY < TileMap->CountY)
		{
			int32 TileMapValue = GetTileValueUnchecked(TileMap, PlayerTileX, PlayerTileY);
			Empty = (TileMapValue == 0);
		}
	}

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
	TileMaps[0][0].CountX = TILE_MAP_COUNT_X;
	TileMaps[0][0].CountY = TILE_MAP_COUNT_Y;
	TileMaps[0][0].UpperLeftX = -30;
	TileMaps[0][0].UpperLeftY = 0;
	TileMaps[0][0].TileWidth = 60;
	TileMaps[0][0].TileHeight = 60;
	TileMaps[0][0].Tiles = (uint32 *) Tiles00;

	TileMaps[0][1] = TileMaps[0][0];
	TileMaps[0][1].Tiles = (uint32 *) Tiles01;

	TileMaps[1][0] = TileMaps[0][0];
	TileMaps[1][0].Tiles = (uint32 *) Tiles10;

	TileMaps[1][1] = TileMaps[0][0];
	TileMaps[1][1].Tiles = (uint32 *) Tiles11;

	tile_map *TileMap = &TileMaps[0][0];

	world World;
	World.TileMapCountX = 2;
	World.TileMapCountY = 2;
	World.TileMaps = (tile_map *) TileMaps;

	real32 PlayerWidth = 0.75f * TileMap->TileWidth;
	real32 PlayerHeight = TileMap->TileHeight;

	game_state *GameState = (game_state *) Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		GameState->PlayerX = 150;
		GameState->PlayerY = 150;

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

			dPlayerX *= 64.0f;
			dPlayerY *= 64.0f;

			// TODO(douglas): Movimentos na diagonal serão mais rápidos, arrumar isso quando tivermos vetores :)
			real32 NewPlayerX = GameState->PlayerX + (Input->dtForFrame * dPlayerX);
			real32 NewPlayerY = GameState->PlayerY + (Input->dtForFrame * dPlayerY);

			if(IsTileMapPointEmpty(TileMap, NewPlayerX, NewPlayerY) &&
			   IsTileMapPointEmpty(TileMap, NewPlayerX - (0.5f * PlayerWidth), NewPlayerY) &&
			   IsTileMapPointEmpty(TileMap, NewPlayerX + (0.5f * PlayerWidth), NewPlayerY))
			{
				GameState->PlayerX = NewPlayerX;
				GameState->PlayerY = NewPlayerY;
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
			int TileID = GetTileValueUnchecked(TileMap, Column, Row);
			real32 gray = 0.5f;
			if(TileID == 1)
			{
				gray = 1.0f;
			}

			real32 MinX = TileMap->UpperLeftX + ((real32) Column * TileMap->TileWidth);
			real32 MinY = TileMap->UpperLeftY + ((real32) Row * TileMap->TileHeight);
			real32 MaxX = MinX + TileMap->TileWidth;
			real32 MaxY = MinY + TileMap->TileHeight;

			DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, gray, gray, gray);
		}
	}

	real32 PlayerR = 0.0f;
	real32 PlayerG = 1.0f;
	real32 PlayerB = 0.7f;
	real32 PlayerLeft = GameState->PlayerX - (0.5f * PlayerWidth);
	real32 PlayerTop = GameState->PlayerY - PlayerHeight;

	DrawRectangle(Buffer, PlayerLeft, PlayerTop,
	              PlayerLeft + PlayerWidth,
	              PlayerTop + PlayerHeight,
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
