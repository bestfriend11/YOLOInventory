/* ========================================================================
   Source File: triangle_3.cpp
   Defines: Useful triangle functions
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#include "precomp_gpcore.h"
#include "triangle_3.h"
#include "vector_3.h"

/* ************************************************************************
   Function: RayIntersectsTriangle
   Description: Intersects a ray with a triangle
   ************************************************************************ */

inline float SAME_SIGN(const float a,const float b)
{
	return ((!IsPositive(a) && !IsPositive(b)) || (!IsNegative(a) && !IsNegative(b))); 
}

bool math::
RayIntersectsTriangle(
	vector_3 const &RayOrigin, vector_3 const &RayDirection,
	vector_3 const &Vertex0, vector_3 const &Vertex1,
	vector_3 const &Vertex2, real &TCoordinate, real &UCoordinate,
	real &VCoordinate)
{
	// Relaxed the tolerances in this routine
	// and added support for coplanar rays & triangles --biddle

	// Calculate edge boundaries
	vector_3 const &Edge1 = Vertex1 - Vertex0;
	vector_3 const &Edge2 = Vertex2 - Vertex0;

	// Find the determinant of the matrix formed by the direction
	// and the two edges (by means of the triple product)
	// then the ray is in the same plane as the triangle
	vector_3 const P		= CrossProduct( RayDirection, Edge2 );
	float const Determinant	= P.DotProduct( Edge1 );

	// Since the determinant measures the area suggested by the matrix,
	// if it is near zero then the ray in the plane of the triangle


	//if((Determinant > RealTolerance) || (Determinant < -RealTolerance))
	if (!IsZero(Determinant))
	{
		float const InverseDeterminant	= 1.0f / Determinant;
		vector_3 const T				= RayOrigin - Vertex0;
		UCoordinate						= T.DotProduct( P ) * InverseDeterminant;
		//if((UCoordinate > RealZero) && (UCoordinate < RealIdentity))
		if(!IsNegative(UCoordinate,0.001f) && !IsPositive(UCoordinate-RealIdentity,0.001f))
		{
			vector_3 const Q			= CrossProduct( T, Edge1 );
			VCoordinate					= RayDirection.DotProduct( Q ) * InverseDeterminant;
			//if((VCoordinate > RealZero) && ((UCoordinate + VCoordinate) < RealIdentity))
			if(!IsNegative(VCoordinate,0.001f) && !IsPositive(UCoordinate+VCoordinate-RealIdentity,0.001f))
			{				
				TCoordinate				= Q.DotProduct( Edge2 ) * InverseDeterminant;
				return true;
			}
			else
			{
				TCoordinate				= 0.0f;	// Need these to set BPoints
			}
		} 
		else
		{
			TCoordinate				= 0.0f;		// Need these to set BPoints
		}

	}
	else
	{

		// Find normal of plane containing RayDirection and parallel to triangle

		const vector_3 TriNorm = CrossProduct(Edge1,Edge2);

		if (IsZero(TriNorm)) {
			// This is a line, not a triangle!
			return false;
		}

		const vector_3 V0 = Vertex0-RayOrigin;

		float planar_check = TriNorm.DotProduct(V0);

		if (!IsZero(planar_check,0.001f)) {
			// The ray is PARALLEL  to the triangle, but does not intersect it
			return false;
		}

		const vector_3 RayNorm = Normalize(CrossProduct(TriNorm,RayDirection));

		const vector_3 V1 = Vertex1-RayOrigin;
		const vector_3 V2 = Vertex2-RayOrigin;

		const float dist0 = RayNorm.DotProduct(V0);
		const float dist1 = RayNorm.DotProduct(V1);
		const float dist2 = RayNorm.DotProduct(V2);

		float t0,t1,t2;

		vector_3 i0, i1, i2;

		if (!SAME_SIGN(dist0,dist1)) 
		{
			i0 = ( V0*dist1 - V1*dist0 ) / (dist1-dist0);
			t0 = RayDirection.DotProduct(i0)/RayDirection.Length2();
		}
		else
		{
			t0 = FLOAT_INFINITE;
		}

		if (!SAME_SIGN(dist1,dist2)) 
		{
			i1 = ( V1*dist2 - V2*dist1 ) / (dist2-dist1);
			t1 = RayDirection.DotProduct(i1)/RayDirection.Length2();
		}
		else
		{
			t1 = FLOAT_INFINITE;
		}

		if (!SAME_SIGN(dist2,dist0)) 
		{
			i2 = ( V2*dist0 - V0*dist2 ) / (dist0-dist2);
			t2 = RayDirection.DotProduct(i2)/RayDirection.Length2();
		}
		else
		{
			t2 = FLOAT_INFINITE;
		}

		TCoordinate = Minimum(t0, Minimum(t1,t2));

		return TCoordinate != FLOAT_INFINITE;

/*
		// U and V coordinates are not set!!!

		const vector_3 T = Vertex0-RayOrigin;

		if (!IsZero(TriNorm.DotProduct(T))) 
		{
			const vector_3 RayNorm = CrossProduct(RayDirection,TriNorm);
			const float dist0 = RayNorm.DotProduct(T);
			const float dist1 = RayNorm.DotProduct(Vertex1-RayOrigin);
			const float dist2 = RayNorm.DotProduct(Vertex2-RayOrigin);

			if (!SAME_SIGN(dist0,dist1)) 
			{
				const vector_3 p0 = ((Vertex0*dist0) - (Vertex1*dist1)) / (dist0-dist1);
				if (!SAME_SIGN(dist1,dist2))
				{
					vector_3 p1 = ((Vertex1*dist1) - (Vertex2*dist2)) / (dist1-dist2);
					const float len0 = (p0-RayOrigin).Length2();
					const float len1 = (p1-RayOrigin).Length2();
					if (len0 < len1)
					{
						TCoordinate = SQRTF(len0/RayDirection.Length2());
					}
					else
					{
						TCoordinate = SQRTF(len1/RayDirection.Length2());
					}
				}
				else 
				{
					TCoordinate = SQRTF((p0-RayOrigin).Length2()/RayDirection.Length2());
				}
				return true;
			}
			else 
			{
				if (!SAME_SIGN(dist1,dist2))
				{
					const vector_3 p1 = ((Vertex1*dist1) - (Vertex2*dist2)) / (dist1-dist2);
					TCoordinate = SQRTF((p1-RayOrigin).Length2()/RayDirection.Length2());
					return true;
				}
			}

		}
*/
	}

	return false;

}

/* ************************************************************************
   Function: RayIntersectsCulledTriangle
   Description: Intersects a ray with a triangle if the triangle is facing
                towards the origin of the ray
   ************************************************************************ */
bool math::
RayIntersectsCulledTriangle(
	vector_3 const &RayOrigin, vector_3 const &RayDirection,
	vector_3 const &Vertex0, vector_3 const &Vertex1,
	vector_3 const &Vertex2, real &TCoordinate, real &UCoordinate,
	real &VCoordinate)
{
	bool Intersected = false;
	
	vector_3 const &Edge1 = Vertex1 - Vertex0;
	vector_3 const &Edge2 = Vertex2 - Vertex0;

	// Find the determinant of the matrix formed by the direction
	// and the two edges (by means of the triple product)
	// then the ray is in the same plane as the triangle
	vector_3 const P = CrossProduct(RayDirection, Edge2);
	real const Determinant = InnerProduct(P, Edge1);

	// Since the determinant measures the area suggested by the matrix,
	// if it is not positive the area is "upside down" and thus we're
	// actually looking at a backface - so we only consider positive
	// determinants as front-facing
	if(Determinant > RealTolerance)
	{
		vector_3 const T = RayOrigin - Vertex0;
		UCoordinate = InnerProduct(T, P);
		if((UCoordinate > RealZero) && (UCoordinate < Determinant))
		{
			vector_3 const Q = CrossProduct(T, Edge1);

			VCoordinate = InnerProduct(RayDirection, Q);
			if((VCoordinate > RealZero) &&
			   ((UCoordinate + VCoordinate) < Determinant))
			{				
				Intersected = true;

				TCoordinate = InnerProduct(Q, Edge2);
				real const InverseDeterminant = RealInverse(Determinant);
				TCoordinate *= InverseDeterminant;
				UCoordinate *= InverseDeterminant;
				VCoordinate *= InverseDeterminant;
			}
		}		
	}
	else if(Determinant > RealZero)
	{
		// TODO: Shouldn't this do an intersection with the boundary
		// of the two edges?
	}

	return(Intersected);
}
