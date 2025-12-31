// This code was lifted straight out of the MAX 2.5 documentation
// I added support for weighting the normals by face angle --biddle

#include "vertnormals.h"

VNormal::VNormal(Point3 &n,float w, DWORD s, bool u)
{
	next=NULL;
	init=TRUE;
	norm=n;
	weight= (w<0) ? 0 : w;
	smooth=s;
	used=u;
}
// Add a normal to the list if the smoothing group bits overlap, 
// otherwise create a new vertex normal in the list
void VNormal::AddNormal(Point3 &n,float w,DWORD s,bool u)
{
	if (!(s&smooth) && init)
	{
		if (next) 
		{
			next->AddNormal(n,w,s,u);
		}
		else
		{
			next = new VNormal(n,w,s,u);
		}
	} 
	else
	{
		float sum_w = (weight + w);
		if (sum_w > 0.0001f) 
		{
			norm = ((norm * weight) + (n * w)) / sum_w;
			weight = sum_w;
		}
		smooth |= s;
		used = used || u;
		init = true;
	}
}

VNormal::~VNormal()
{
	delete next;
}

// Retrieves a normal if the smoothing groups overlap or there is 
// only one in the list
Point3& VNormal::GetNormal(DWORD s)
{
	try
	{
		if ((smooth&s) || !next)
		{
			return norm;
		}
		else 
		{
			return next->GetNormal(s);
		}
	}
	catch(...)
	{
		static Point3 zerop(0,0,0);
		return zerop;
	}
}

// Normalize each normal in the list
void VNormal::Normalize()
{
	if (init)
	{
		VNormal *ptr = next, *prev = this;
		while (ptr)
		{
			if (ptr->smooth&smooth)
			{
				float sum_w = weight + ptr->weight;
				if (sum_w > 0.0001f) 
				{
					norm = ((norm * weight) + (ptr->norm * ptr->weight)) / sum_w;
					weight = sum_w;
				}
				used |= ptr->used;
				smooth |= ptr->smooth;
				prev->next = ptr->next;
				ptr->next = NULL;		// Make sure that 'ptr' is unlinked before we delete
				delete ptr;
				ptr = prev->next;
			} 
			else {
				prev = ptr;
				ptr  = ptr->next;
			}
		}
		if (this != prev)
		{
			if (prev->smooth&smooth)
			{
				float sum_w = weight + prev->weight;
				if (sum_w > 0.0001f) 
				{
					norm = ((norm * weight) + (prev->norm * prev->weight)) / sum_w;
					weight = sum_w;
				}
				used |= prev->used;
				smooth |= prev->smooth;
	
				VNormal *ptr = this;
				while (ptr && ptr->next && (ptr->next != prev))
				{
					ptr = ptr->next;
				}
				if (ptr && ptr->next)
				{
					ptr->next->next = NULL;	// Make sure that 'ptr->next' is unlinked before we delete
					delete ptr->next;
					ptr->next = NULL;
				}
				
			} 		
		}
		norm = ::Normalize(norm);
	}
	if (next) next->Normalize();
}

// Remove normals that are not used in the mesh
VNormal* Prune(VNormal *head) {

	VNormal* curr = head;
	VNormal* prev = NULL;
	VNormal* dead = NULL;

	while (curr) {
		if (curr->used) {
			prev = curr;
			curr = curr->next;
		} else {
			dead = curr;
			if (prev == NULL) {
				head = curr->next;
			} else {
				prev->next = curr->next;
			}
			curr = curr->next;
			dead->next = NULL;
			delete dead;
		}
	}

	return head;
}

// Build the vertex normals for a mesh using smoothing groups & weighted by corner angle 
// Utility function derived from the one I originally wrote for the SNO exporter
// The SNO code has to take into account 'locked' normals -- I suppose that I should merge the two.
int CalcVertNormals(TriObject* obj, const Matrix3& normXForm, std::vector<VNormal>& vnorms,bool neg_par)
{
	vnorms.clear();

	int totalNorms=0;

	Point3 pt[3];
	float  av[3];
	Point3 fnorm;

	Mesh& theMesh = obj->GetMesh();

	int i;

	// Compute the vertex surface normals for normals we are NOT ignoring	
	for (i=0; i < theMesh.getNumVerts(); i++) 
	{
		vnorms.push_back(VNormal());
	}

	for (i=0; i < theMesh.getNumFaces(); i++)
	{

		Face& face = theMesh.faces[i];

		pt[0] = normXForm * theMesh.verts[face.v[0]];
		if (neg_par)
		{
			pt[2] = normXForm * theMesh.verts[face.v[1]];
			pt[1] = normXForm * theMesh.verts[face.v[2]];
		}
		else
		{
			pt[1] = normXForm * theMesh.verts[face.v[1]];
			pt[2] = normXForm * theMesh.verts[face.v[2]];
		}

		av[0] = ComputeAngleBetweenVectors((pt[1] - pt[0]),(pt[2] - pt[0]));
		av[1] = ComputeAngleBetweenVectors((pt[2] - pt[1]),(pt[0] - pt[1]));
		av[2] = ComputeAngleBetweenVectors((pt[0] - pt[2]),(pt[1] - pt[2]));

		// Calculate the face normal

		fnorm = CrossProd((pt[1] - pt[0]),(pt[2] - pt[0])).Normalize();

		for (int j=0; j<3; j++)
		{
			int mappedvert = face.v[j];
			int sm = face.getSmGroup();

			vnorms[mappedvert].AddNormal(fnorm,av[j],face.getSmGroup(),true);
		}

	}

	// Now we have to re-normalize everything
	for (i=0; i < vnorms.size(); i++)
	{
		vnorms[i].Normalize();
	}

	// Count all of the norms, storing the position of the first normal used by each vert
	for (i=0; i < vnorms.size(); i++) 
	{
		totalNorms++;

		VNormal* vn = vnorms[i].next;

		while (vn)
		{
			totalNorms++;
			vn = vn->next;
		}
	}

	return totalNorms;
}

