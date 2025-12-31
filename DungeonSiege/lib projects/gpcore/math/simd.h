#pragma once
#ifndef _SIMD_H_
#define _SIMD_H_

/************************************************************************************
**
**						SIMD optimized math functions
**
**
**	Author:		James Loe
**	Date:		03/09/00
**
************************************************************************************/

#include "vector_3.h"
#include "matrix_3x3.h"


// Matrix functions
inline void matrix3x3::Transform_SIMD( vector_3* out, const vector_3* src )
{
	__asm
	{
		mov		esi, src
		mov		edi, this
		mov		ebx, out_

		movss	xmm1, dword ptr [edi]
		movhps	xmm1, qword ptr [edi+4]
		movaps	xmm5, xmm1
		movss	xmm3, dword ptr [edi+12]
		movhps	xmm3, qword ptr [edi+24]
		movss	xmm4, dword ptr [esi]
		shufps	xmm5, xmm3, 128
		movlps	xmm0, qword ptr [edi+16]
		shufps	xmm4, xmm4, 0
		movhps	xmm0, qword ptr [edi+28]
		shufps	xmm1, xmm0, 219
		movss	xmm2, dword ptr [esi+4]
		movaps	xmm3, xmm1
		shufps	xmm1, xmm0, 129
		shufps	xmm2, xmm2, 0
		movss	xmm0, dword ptr [esi+8]
		mulps	xmm4, xmm5
		mulps	xmm2, xmm1
		shufps	xmm0, xmm0, 0
		addps	xmm4, xmm2
		mulps	xmm0, xmm3
		addps	xmm4, xmm0
		movss	dword ptr [ebx], xmm4
		movhps	qword ptr [ebx+4], xmm4
	}
}

inline void Multiply_SIMD( matrix_3x3& out, const matrix_3x3& l, const matrix_3x3& r )
{
	__asm
	{
		mov		ecx, l
		mov		ebx, r
		mov		eax, out

		movss	xmm2, dword ptr [ecx+32]
		movhps	xmm2, qword ptr [ecx+24]

		movss	xmm3, dword ptr [ebx  ]
		movss	xmm4, dword ptr [ebx+4]

		movss	xmm0, dword ptr [ecx]
		movhps	xmm0, qword ptr [ecx+4]
		shufps	xmm2, xmm2, 0x36
		shufps	xmm3, xmm3, 0

		movss	xmm1, dword ptr [ecx+12]
		movhps	xmm1, qword ptr [ecx+16]

		shufps	xmm4, xmm4, 0
		mulps	xmm3, xmm0
		movss	xmm5, dword ptr [ebx+8]
		movss	xmm6, dword ptr [ebx+12]
		mulps	xmm4, xmm1
		shufps	xmm5, xmm5, 0
		mulps	xmm5, xmm2
		shufps	xmm6, xmm6, 0
		mulps	xmm6, xmm0
		addps	xmm3, xmm4


		movss	xmm7, dword ptr [ebx+16]
		movss	xmm4, dword ptr [ebx+28]

		shufps	xmm7, xmm7, 0
		addps	xmm3, xmm5
		mulps	xmm7, xmm1

		shufps	xmm4, xmm4, 0

		movss	xmm5, dword ptr [ebx+20]
		shufps	xmm5, xmm5, 0
		mulps	xmm4, xmm1

		mulps	xmm5, xmm2
		addps	xmm6, xmm7

		movss	xmm1, dword ptr [ebx+24]

		movss	dword ptr [eax  ], xmm3		
		movhps	qword ptr [eax+4], xmm3

		addps	xmm6, xmm5
		shufps	xmm1, xmm1, 0

		movss	xmm5, dword ptr [ebx+32]
		mulps	xmm1, xmm0
		shufps	xmm5, xmm5, 0
		
		mulps	xmm5, xmm2
		addps	xmm1, xmm4

		addps	xmm1, xmm5

		shufps	xmm1, xmm1, 0x8F

		movss	dword ptr [eax+12], xmm6
		movhps	qword ptr [eax+16], xmm6
		movhps	qword ptr [eax+24], xmm1
		movss	dword ptr [eax+32], xmm1
	}
}


// Vector functions
inline float DotProduct_SIMD( const vector_3& v1, const vector_3& v2 )
{
	vector_3	out( DoNotInitialize );
	__asm
	{
		mov		edi, v1
		mov		esi, v2
		mov		ebx, out

		movss	xmm1, dword ptr [edi]
		movhps	xmm1, qword ptr [edi+4]

		movss	xmm2, dword ptr [esi]
		movhps	xmm2, qword ptr [esi+4]
		mulps	xmm2, xmm1

		movss	dword ptr [ebx], xmm2
		movhps	qword ptr [ebx+4], xmm2
	}
	return		(out.x + out.y + out.z);
}

inline float vector_3::DotProduct_SIMD( const vector_3& v ) const
{
	vector_3	out( DoNotInitialize );
	__asm
	{
		mov		edi, this
		mov		esi, v
		mov		ebx, out

		movss	xmm1, dword ptr [edi]
		movhps	xmm1, qword ptr [edi+4]

		movss	xmm2, dword ptr [esi]
		movhps	xmm2, qword ptr [esi+4]
		mulps	xmm2, xmm1

		movss	dword ptr [ebx], xmm2
		movhps	qword ptr [ebx+4], xmm2
	}
	return		(out.x + out.y + out.z);
}

inline vector_3	vector_3::Multiply_SIMD( const vector_3& r )
{
	vector_3	out( DoNotInitialize );
	__asm
	{
		mov		edi, this
		mov		esi, r
		mov		ebx, out

		movss	xmm1, dword ptr [edi]
		movhps	xmm1, qword ptr [edi+4]

		movss	xmm2, dword ptr [esi]
		movhps	xmm2, qword ptr [esi+4]
		mulps	xmm2, xmm1

		movss	dword ptr [ebx], xmm2
		movhps	qword ptr [ebx+4], xmm2
	}
	return		out;
}

inline vector_3 vector_3::Multiply_SIMD( float r )
{
	vector_3	out( DoNotInitialize );
	__asm
	{
		mov		edi, this
		mov		ebx, out

		movss	xmm1, dword ptr [edi]
		movhps	xmm1, qword ptr [edi+4]

		movss	xmm2, r
		shufps	xmm2, xmm2, 0
		mulps	xmm2, xmm1

		movss	dword ptr [ebx], xmm2
		movhps	qword ptr [ebx+4], xmm2
	}
	return		out;
}


#endif

