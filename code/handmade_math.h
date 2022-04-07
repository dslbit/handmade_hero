#if !defined(HANDMADE_MATH_H)
/*
struct v2
{
	union {
		struct
		{
			real32 X, Y;
		};
		real32 E[2];
	};
	
	//real32 &operator[](int Index) {return((&X)[Index]);}
	//inline v2 &operator*=(real32 A);
	//inline v2 &operator+=(v2 A);
};
*/

union v2 {
	struct
	{
		real32 X, Y;
	};
	real32 E[2];
};

// TODO: Considerar: v2 FOO = v2{5,4}; ?
inline v2
V2(real32 X, real32 Y)
{
	v2 Result;

	Result.X = X;
	Result.Y = Y;

	return(Result);
}

// B = -A;
inline v2
operator-(v2 A)
{
	v2 Result;

	Result.X = -A.X;
	Result.Y = -A.Y;

	return(Result);
}

// C = A + B;
inline v2
operator+(v2 A, v2 B)
{
	v2 Result;

	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;

	return(Result);
}

/*
inline v2
operator+(v2 A, real32 B)
{
	v2 Result;

	Result.X = A.X + B;
	Result.Y = A.Y + B;

	return(Result);
}
*/

inline v2 &
operator+=(v2 &B, v2 A)
{
	B = A + B;

	return(B);
}

// C = A - B
inline v2
operator-(v2 A, v2 B)
{
	v2 Result;

	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;

	return(Result);
}

/*
inline v2
operator-(v2 A, real32 B)
{
	v2 Result;

	Result.X = A.X - B;
	Result.Y = A.Y - B;

	return(Result);
}
*/

inline v2
operator*(real32 A, v2 B)
{
	v2 Result;

	Result.X = A*B.X;
	Result.Y = A*B.Y;

	return(Result);
}

inline v2
operator*(v2 B, real32 A)
{
	v2 Result = A * B;

	return(Result);
}

inline v2 &
operator*=(v2 &B, real32 A)
{
	B = A * B;

	return(B);
}

inline real32
Square(real32 A)
{
	real32 Result = A*A;
	
	return(Result);
}

inline real32
Inner(v2 A, v2 B)
{
	real32 Result = A.X * B.X + A.Y * B.Y;

	return(Result);
}

inline real32
LengthSq(v2 A)
{
	real32 Result = Inner(A, A);

	return(Result);
}

#define HANDMADE_MATH_H
#endif
