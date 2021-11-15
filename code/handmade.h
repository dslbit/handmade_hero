#if !defined(HANDMADE_H)

/*
	NOTE(Douglas):

	HANDMADE_INTERNAL:
		0 - Build para lançamento público
		1 - Build apenas para desenvolvedor

	HANDMADE_SLOW:
		0 - Nenhum código lento permitido!
		1 - Código lento é bem-vindo.
*/

#include "handmade_platform.h"

#define internal 		static
#define global_variable static
#define local_persist 	static

#define Pi32 (3.14159265359f)

// TODO(Douglas): Completar o macro "Assert"
#if HANDMADE_SLOW
	#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0; }
#else
	#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32
SafeTruncateUInt64(uint64 Value)
{
	uint32 Result;
	// TODO(Douglas): "Defines" para valores máximos
	Assert(Value <= 0xFFFFFFFF); // UInt32Max
	Result = (uint32) Value;
	return(Result);
}

// TODO(Douglas): Swap, min, max ... macros??

inline game_controller_input *GetController(game_input *Input, uint32 ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input *Result = &Input->Controllers[ControllerIndex];
	return(Result);
}

//
//
//

struct tile_chunk_position
{
	uint32 TileChunkX;
	uint32 TileChunkY;

	uint32 RelTileX;
	uint32 RelTileY;
};

// NOTE(douglas): Agora isso pode ser apenas "world_position"?
struct world_position
{
	// TODO(douglas): pegar os dois conjuntos abaixo e empacotar eles em um único valor
	// de 32-bits onde há bits na parte mais insignificante que nos informa os índices
	// dos azulejos, e na parte mais significante que nos informa a "pagina" (desses).
	uint32 AbsTileX;
	uint32 AbsTileY;

	// TODO(douglas): Essas medidas deveriam ser ajustadas do centro do azulejo?
	// TODO(douglas): renomear para "offset" X e Y
	real32 TileRelX;
	real32 TileRelY;
};

struct tile_chunk
{
	uint32 *Tiles;
};

struct world
{
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDim;

	real32 TileSideInMeters;
	int32 TileSideInPixels;
	real32 MetersToPixel;

	int32 TileChunkCountX;
	int32 TileChunkCountY;
	
	tile_chunk *TileChunks;
};

struct game_state
{
	world_position PlayerP;
};

#define HANDMADE_H
#endif
