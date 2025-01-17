#include "handmade_tile.h"

inline tile_chunk *
GetTileChunk(tile_map *TileMap,
             uint32 TileChunkX,
             uint32 TileChunkY,
             uint32 TileChunkZ)
{
	tile_chunk *TileChunk = 0;
	if(TileChunkX >= 0 && TileChunkX < TileMap->TileChunkCountX &&
	   TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY &&
	   TileChunkZ >= 0 && TileChunkZ < TileMap->TileChunkCountZ)
	{
		TileChunk = &TileMap->TileChunks[
		                                 (TileChunkZ * TileMap->TileChunkCountY * TileMap->TileChunkCountX) +
		                                 (TileChunkY * TileMap->TileChunkCountX) +
		                                 TileChunkX
		                                ];
	}
	return(TileChunk);
}


inline uint32
GetTileValueUnchecked(tile_map *TileMap,
                      tile_chunk *TileChunk,
                      uint32 TileX,
                      uint32 TileY)
{
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);
	uint32 TileChunkValue = TileChunk->Tiles[ (TileY*TileMap->ChunkDim) + TileX];
	return(TileChunkValue);
}

inline void
SetTileValueUnchecked(tile_map *TileMap,
                      tile_chunk *TileChunk,
                      uint32 TileX,
                      uint32 TileY,
                      uint32 TileValue)
{
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);

	TileChunk->Tiles[ (TileY*TileMap->ChunkDim) + TileX] = TileValue;
}

internal uint32
GetTileValue(tile_map *TileMap,
             tile_chunk *TileChunk,
             uint32 TestTileX,
             uint32 TestTileY)
{
	uint32 TileChunkValue = 0;

	if(TileChunk && TileChunk->Tiles)
	{
		TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
	}

	return(TileChunkValue);
}

inline void
SetTileValue(tile_map *TileMap,
             tile_chunk *TileChunk,
             uint32 TestTileX,
             uint32 TestTileY,
             uint32 TileValue)
{
	if(TileChunk && TileChunk->Tiles)
	{
		SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY,TileValue);
	}
}


inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap,
                    uint32 AbsTileX,
                    uint32 AbsTileY,
                    uint32 AbsTileZ)
{
	tile_chunk_position Result;

	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.TileChunkZ = AbsTileZ;
	Result.RelTileX = AbsTileX & TileMap->ChunkMask;
	Result.RelTileY = AbsTileY & TileMap->ChunkMask;

	return(Result);
}

inline bool32
GetTileValue(tile_map *TileMap,
             uint32 AbsTileX,
             uint32 AbsTileY,
             uint32 AbsTileZ)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
 	uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

	return(TileChunkValue);
}

internal uint32
GetTileValue(tile_map *TileMap,
             tile_map_position Pos)
{
	uint32 TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
	return(TileChunkValue);
}

inline uint32
IsTileMapPointEmpty(tile_map *TileMap,
                  tile_map_position Pos)
{
 	uint32 TileChunkValue = GetTileValue(TileMap, Pos);
	bool32 Empty = (TileChunkValue == 1 ||
	                TileChunkValue == 3 ||
	                TileChunkValue == 4);
	
	return(Empty);
}

internal void
SetTileValue(memory_arena *Arena,
             tile_map *TileMap,
             uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ,
             uint32 TileValue)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);

	// TODO(douglas): Criação de chunk
	Assert(TileChunk);

	if(!TileChunk->Tiles)
	{
		uint32 TileCount = (TileMap->ChunkDim * TileMap->ChunkDim);

		TileChunk->Tiles = PushArray(Arena, TileCount, uint32);

		for(uint32 TileIndex = 0;
		    TileIndex < TileCount;
		    ++TileIndex)
		{
			TileChunk->Tiles[TileIndex] = 1;
		}
	}
	
	SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}


//
// TODO: Essas funções realmente pertecem a um arquivo de "posicionamento" ou "geometria"?
//

inline void
RecanonicalizeCoord(tile_map *TileMap,
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
	int32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * TileMap->TileSideInMeters;

	// TODO(douglas): Arrumar o calculo com ponto flutuante.
	Assert(*TileRel >= -0.5f * TileMap->TileSideInMeters);	
	Assert(*TileRel <= 0.5f * TileMap->TileSideInMeters);
}

inline tile_map_position
RecanonicalizePosition(tile_map *TileMap,
                       tile_map_position Pos)
{
	tile_map_position Result = Pos;

	RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset.X);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset.Y);

	return(Result);
}

inline bool32
AreOnSameTile(tile_map_position *A,
              tile_map_position *B)
{
	bool32 Result = (A->AbsTileX == B->AbsTileX &&
	                 A->AbsTileY == B->AbsTileY &&
	                 A->AbsTileZ == B->AbsTileZ);
	return(Result);
}

inline tile_map_difference
Subtract(tile_map *TileMap,
         tile_map_position *A,
         tile_map_position *B)
{
	tile_map_difference Result;

	v2 dTileXY = {(real32)A->AbsTileX - (real32)B->AbsTileX,
	                  (real32)A->AbsTileY - (real32)B->AbsTileY};
	real32 dTileZ = (real32)(A->AbsTileZ - (real32)B->AbsTileZ);

	Result.dXY = TileMap->TileSideInMeters * dTileXY + (A->Offset - B->Offset);

	// TODO: Pensar sobre o que fazer com Z
	Result.dZ = 0.0f;

	return(Result);
}
