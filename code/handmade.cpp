#include "handmade.h"

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
			uint8 Red = 0;
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);

			*Pixel++ = (Red << 16) | (Green << 8) | Blue;
		}
		
		Row += Buffer->Pitch; // (no momento) é o mesmo que dizer "Row = (uint8 *)Pixel;" pois a memória não tem espaçamento.
	}
}

internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer,
                    int BlueOffset,
                    int GreenOffset)
{
	//int BlueOffset = 0;
	//int GreenOffset = 0;
	RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}
