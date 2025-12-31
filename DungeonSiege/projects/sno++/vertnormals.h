// This code was lifted straight out of the MAX 2.5 documentation
#include "max.h"

// Linked list of vertex normals
class VNormal {
	public:
		Point3 norm;
		DWORD smooth;
		VNormal *next;
		bool init;
		bool used;

		VNormal() {smooth=0;next=NULL;init=FALSE;norm=Point3(0,0,0);}
		VNormal(Point3 &n,DWORD s, bool u=false) {next=NULL;init=TRUE;norm=n;smooth=s;used=u;}
		~VNormal() {delete next;}
		void AddNormal(Point3 &n,DWORD s,bool used);
		Point3 &GetNormal(DWORD s);
		void Normalize();
};

VNormal* Prune(VNormal*);

