
#ifndef _MANIFOLDUTILS_H_
#define _MANIFOLDUTILS_H_

#include <cstdint>

struct SPositionVertex
{
	float x, y, z;
};

class CManifoldUtils
{
private:
	CManifoldUtils();
	~CManifoldUtils();

public:
	static void Create2MBox(SPositionVertex* outvdata, uint32_t* outidata, float width, float height, float depth);
	static void Create2MSphere(SPositionVertex* outvdata, uint32_t* outidata, float radius, uint16_t vsegments, uint16_t hsegments);

	static void GenerateGSAdjacency(uint32_t* outidata, uint32_t* idata, uint32_t numindices);

	static void NumVerticesIndices2MSphere(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments);
};

#endif
