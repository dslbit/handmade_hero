#if !defined(HANDMADE_TILE_H)

struct tile_map_difference
{
	v2 dXY;
	real32 dZ;
};

struct tile_map_position
{
	// NOTE(douglas): Essas posições de azulejos são fixas. Os bits mais significantes
	// representão os índices dos chunks e os bits menos significantes representão os
	// índices dos azulejos dentro do chunk.
	uint32 AbsTileX;
	uint32 AbsTileY;
	uint32 AbsTileZ;

	// NOTE: São ajustes, do centro do azulejo
	v2 Offset;
};


struct tile_chunk_position
{
	uint32 TileChunkX;
	uint32 TileChunkY;
	uint32 TileChunkZ;

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

	uint32 TileChunkCountX;
	uint32 TileChunkCountY;
	uint32 TileChunkCountZ;

	// TODO: Fazer um armazenamento esparso melhor, para que qualquer lugar no mundo
	// possa ser representado sem precisar do conjunto de ponteiros em "tile_chunk".
	tile_chunk *TileChunks;
};


#define HANDMADE_TILE_H
#endif
