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

struct canonical_position
{
	// TODO(douglas): pegar os dois conjuntos abaixo e empacotar eles em um único valor
	// de 32-bits onde há bits na parte mais insignificante que nos informa os índices
	// dos azulejos, e na parte mais significante que nos informa a "pagina" (desses).
	int32 TileMapX;
	int32 TileMapY;

	int32 TileX;
	int32 TileY;

	// TODO(douglas): Converter esses caras para representação de medidas do mundo real
	// para facilitar os calculos matemáticos.
	real32 X;
	real32 Y;
};

// TODO(douglas): será que isso é necessário?
struct raw_position
{
	int32 TileMapX;
	int32 TileMapY;

	// NOTE(douglas): Valores (abaixo) relativos ao mapa de azulejos
	real32 X;
	real32 Y;
};

struct tile_map
{
	uint32 *Tiles;
};

struct world
{
	real32 TileSideInMeters;
	int32 TileSideInPixels;
	
	int32 CountX;
	int32 CountY;

	real32 UpperLeftX;
	real32 UpperLeftY;

	int32 TileMapCountX;
	int32 TileMapCountY;
	
	tile_map *TileMaps;
};

struct game_state
{
	// TODO(douglas): isso deveria ser posições canonicas?
	int32 PlayerTileMapX;
	int32 PlayerTileMapY;

	real32 PlayerX;
	real32 PlayerY;
};

#define HANDMADE_H
#endif
