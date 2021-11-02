#if !defined(WIN32_HANDMADE_H)

struct win32_offscreen_buffer
{
	// NOTE(Douglas): Pixels são sempre 32-bits, Ordem de memória: bb gg rr xx
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimensions
{
	int Width;
	int Height;
};

struct win32_sound_output
{
	int SamplesPerSecond;
	uint32 RunningSampleIndex;
	int BytesPerSample;
	uint32 SecondaryBufferSize;
	real32 tSine;
	int LatencySampleCount;
	uint32 SafetyBytes;
	// TODO(Douglas): "RunningSampleIndex" também deveria ser em bytes?
	// TODO(Douglas): A matemática ficaria mais simples se nós adicionassemos um campo "bytes per second"?
};

struct win32_debug_time_marker
{
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;
	DWORD OutputLocation;
	DWORD OutputByteCount;
	DWORD ExpectedFlipPlayCursor;

	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};

#define WIN32_HANDMADE_H
#endif
