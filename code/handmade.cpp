#include "handmade.h"
#include "handmade_tile.cpp"
#include "handmade_random.h"

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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(ArrayCount(Input->Controllers[0].Buttons) == 
	       (&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	real32 PlayerHeight = 1.4f;
	real32 PlayerWidth = 0.75f * PlayerHeight;

	uint32 RandomNumberIndex = 0;

	game_state *GameState = (game_state *) Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		GameState->PlayerP.AbsTileX = 3;
		GameState->PlayerP.AbsTileY = 3;
		GameState->PlayerP.TileRelX = 0;
		GameState->PlayerP.TileRelY = 0;

		InitializeArena(&GameState->WorldArena,
		                Memory->PermanentStorageSize - sizeof(game_state),
		                (uint8 *) Memory->PermanentStorage + sizeof(game_state));

		GameState->World = PushStruct(&GameState->WorldArena, world);
		world *World = GameState->World;
		World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

		tile_map *TileMap = World->TileMap;

		// NOTE(douglas): está configurado para os chunks dos azulejos serem 256x256
		TileMap->ChunkShift = 4;
		TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
		TileMap->ChunkDim = (1 << TileMap->ChunkShift);


		TileMap->TileChunkCountX = 128;
		TileMap->TileChunkCountY = 128;
		TileMap->TileChunkCountZ = 2;

		TileMap->TileChunks = PushArray(&GameState->WorldArena,
		                                (TileMap->TileChunkCountX * TileMap->TileChunkCountY * TileMap->TileChunkCountZ),
		                                tile_chunk);

		TileMap->TileSideInMeters = 1.4f;

		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 ScreenX = 0;
		uint32 ScreenY = 0;
		uint32 AbsTileZ = 0;

		// TODO: Trocar isso por uma geração de mundo real, porque isso aqui tá um lixo.
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;

		for(uint32 Screen = 0;
		    Screen < 100;
		    ++Screen)
		{
			// TODO(douglas): Números Aleatórios!!!
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
			uint32 RandomChoice;
			if(DoorUp || DoorDown)
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
			}
			else
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
			}

			if(RandomChoice == 2)
			{
				if(AbsTileZ == 0)
				{
					DoorUp = true;
				}
				else
				{
					DoorDown = true;
				}
			}
			else if(RandomChoice == 1)
			{
				DoorRight = true;
			}
			else 
			{
				DoorTop = true;
			}

			for(uint32 TileY = 0;
			    TileY < TilesPerHeight;
			    ++TileY)
			{
				for(uint32 TileX = 0;
				    TileX < TilesPerWidth;
				    ++TileX)
				{
					uint32 AbsTileX = (ScreenX * TilesPerWidth) + TileX;
					uint32 AbsTileY = (ScreenY * TilesPerHeight) + TileY;

					uint32 TileValue = 1;
					if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
					{
						TileValue = 2;
					}

					if( (TileX == (TilesPerWidth - 1)) &&  (!DoorRight || (TileY != (TilesPerHeight/2))))
					{
						TileValue = 2;
					}

					if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
					{
						TileValue = 2;
					}

					if( (TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
					{
						TileValue = 2;
					}

					if(TileX == 10 && TileY == 6)
					{
						if(DoorUp)
						{
							TileValue = 3;
						}

						if(DoorDown)
						{
							TileValue = 4;
						}
					}

					SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
				}
			}
			
			DoorBottom = DoorTop;
			DoorLeft = DoorRight;
			
			if(DoorUp)
			{
				DoorDown = true;
				DoorUp = false;
			}
			else if(DoorDown)
			{
				DoorUp = true;
				DoorDown = false;
			}
			else
			{
				DoorUp = false;
				DoorDown = false;
			}

			DoorTop = false;
			DoorRight = false;

			if(RandomChoice == 2)
			{
				if(AbsTileZ == 0)
				{
					AbsTileZ = 1;
				}
				else if(AbsTileZ == 1)
				{
					AbsTileZ = 0;
				}
			}
			else if(RandomChoice == 1)
			{
				ScreenX += 1;
			}
			else
			{
				ScreenY += 1;
			}
		}

		Memory->IsInitialized = true;
	}


	world *World = GameState->World;
	tile_map *TileMap = World->TileMap;
	
	// NOTE: Isso é um conceito do renderizador, o TileMap não precisa reter essa informação.
	int32 TileSideInPixels = 60;
	real32 MetersToPixel = ((real32) TileSideInPixels / (real32) TileMap->TileSideInMeters);

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
			real32 PlayerSpeed = 2.0f;

			if(Controller->ActionUp.EndedDown)
			{
				PlayerSpeed = 15.0f;
			}

			dPlayerX *= PlayerSpeed;
			dPlayerY *= PlayerSpeed;

			// TODO(douglas): Movimentos na diagonal serão mais rápidos, arrumar isso quando tivermos vetores :)
			tile_map_position NewPlayerP = GameState->PlayerP;
			NewPlayerP.TileRelX += (Input->dtForFrame * dPlayerX);
			NewPlayerP.TileRelY += (Input->dtForFrame * dPlayerY);
			NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);

			// para hit test na esquerda
			tile_map_position PlayerLeft = NewPlayerP;
			PlayerLeft.TileRelX -= (0.5f * PlayerWidth);
			PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);

			// para hit test na direita
			tile_map_position PlayerRight = NewPlayerP;
			PlayerRight.TileRelX += (0.5f * PlayerWidth);
			PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

			if(IsWorldPointEmpty(TileMap, NewPlayerP) &&
			   IsWorldPointEmpty(TileMap, PlayerLeft) &&
			   IsWorldPointEmpty(TileMap, PlayerRight))
			{
				GameState->PlayerP = NewPlayerP;
			}
		}
	}

	DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height,
	              1.0f, 0.0f, 1.0f);

	real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
	real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;

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
			
			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);
			
			if(TileID > 0)
			{
				real32 gray = 1.0f;
				if(TileID == 1)
				{
					gray = 0.5f;
				}

				if(TileID > 2)
				{
					gray = 0.25f;
				}

				if(Column == GameState->PlayerP.AbsTileX &&
				   Row == GameState->PlayerP.AbsTileY)
				{
					gray = 0;
				}

				real32 CenX = ScreenCenterX - MetersToPixel * GameState->PlayerP.TileRelX + ((real32) RelColumn * TileSideInPixels);
				real32 CenY = ScreenCenterY + MetersToPixel * GameState->PlayerP.TileRelY - ((real32) RelRow * TileSideInPixels);

				real32 MinX = CenX - (0.5f * TileSideInPixels);
				real32 MinY = CenY - (0.5f * TileSideInPixels);
				real32 MaxX = CenX + (0.5f * TileSideInPixels);
				real32 MaxY = CenY + (0.5f * TileSideInPixels);

				DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, gray, gray, gray);
			}
		}
	}

	real32 PlayerR = 0.0f;
	real32 PlayerG = 1.0f;
	real32 PlayerB = 0.7f;
	real32 PlayerLeft = ScreenCenterX - (0.5f * PlayerWidth * MetersToPixel);
	real32 PlayerTop = ScreenCenterY - PlayerHeight * MetersToPixel;

	DrawRectangle(Buffer, PlayerLeft, PlayerTop,
	              PlayerLeft + PlayerWidth * MetersToPixel,
	              PlayerTop + PlayerHeight * MetersToPixel,
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
