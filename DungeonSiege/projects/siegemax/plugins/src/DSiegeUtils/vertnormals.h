#pragma once
// This code was lifted straight out of the MAX 2.5 documentation
// I added support for weighting the normals by face angle --biddle
#include "max.h"
#include <vector>

// Linked list of weighted vertex normals
class VNormal {
	public:
		Point3 norm;
		float weight;
		DWORD smooth;
		VNormal *next;
		bool init;
		bool used;

		VNormal() {
			smooth=0;
			next=NULL;
			init=FALSE;
			norm=Point3(0,0,0);
			weight=0.0f;
			used=false;
			}

		VNormal(Point3 &n,float w,DWORD s, bool u);
		~VNormal();

		void AddNormal(Point3 &n,float w, DWORD s,bool used);
		Point3 &GetNormal(DWORD s);
		void Normalize();
};

VNormal* Prune(VNormal*);

int CalcVertNormals(TriObject* obj, const Matrix3& normXform, std::vector<VNormal>& vnorms, bool neg_parity);

inline float ComputeAngleBetweenVectors(const Point3& a, const Point3& b)
{
	float denom = Length(a)*Length(b);

	if (denom < 0.0001f)
	{
		return 0;
	}

	float dp = DotProd(a,b);

	if (dp > denom)
	{
		return (dp < 0) ? PI : 0;
	}

	return acos(dp/denom);

}

