
#ifndef _GEOMETRUTILS_H_
#define _GEOMETRUTILS_H_

#include <cstdint>

struct SCommonVertex
{
	float x, y, z;
	float u, v;
	float nx, ny, nz;
};

class CGeometryUtils
{
private:
	CGeometryUtils();
	~CGeometryUtils();

public:
	static void CreateSphere(SCommonVertex* outvdata, uint32_t* outidata, float radius, uint16_t vsegments, uint16_t hsegments);
	static void NumVerticesIndicesSphere(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments);
};

#endif
