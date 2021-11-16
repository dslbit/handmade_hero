#if !defined(HANDMADE_TILE_H)

struct tile_map_position
{
	// NOTE(douglas): Essas posições de azulejos são fixas. Os bits mais significantes
	// representão os índices dos chunks e os bits menos significantes representão os
	// índices dos azulejos dentro do chunk.
	uint32 AbsTileX;
	uint32 AbsTileY;

	// TODO(douglas): renomear para "offset" X e Y
	real32 TileRelX;
	real32 TileRelY;
};


struct tile_chunk_position
{
	uint32 TileChunkX;
	uint32 TileChunkY;

	uint32 RelTileX;
	uint32 RelTileY;
};

struct tile_chunk
{
	uint32 *Tiles;
};

struct tile_map
{
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDim;

	real32 TileSideInMeters;
	int32 TileSideInPixels;
	real32 MetersToPixel;

	uint32 TileChunkCountX;
	uint32 TileChunkCountY;
	
	tile_chunk *TileChunks;
};


#define HANDMADE_TILE_H
#endif
