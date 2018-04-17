
#include "ManifoldUtils.h"

CManifoldUtils::CManifoldUtils()
{
}

CManifoldUtils::~CManifoldUtils()
{
}

void CManifoldUtils::Create2MBox(SPositionVertex* outvdata, uint32_t* outidata, float width, float height, float depth)
{
	outvdata[0] = { width * -0.5f, height * -0.5f, depth * -0.5f };
	outvdata[1] = { width * -0.5f, height * -0.5f, depth * 0.5f };
	outvdata[2] = { width * -0.5f, height * 0.5f, depth * -0.5f };
	outvdata[3] = { width * -0.5f, height * 0.5f, depth * 0.5f };

	outvdata[4] = { width * 0.5f, height * -0.5f, depth * -0.5f };
	outvdata[5] = { width * 0.5f, height * -0.5f, depth * 0.5f };
	outvdata[6] = { width * 0.5f, height * 0.5f, depth * -0.5f };
	outvdata[7] = { width * 0.5f, height * 0.5f, depth * 0.5f };

	// you don't have to understand...
	outidata[0] = outidata[3] = outidata[11] = outidata[31]						= 0;
	outidata[1] = outidata[8] = outidata[10] = outidata[24] = outidata[27]		= 2;
	outidata[2] = outidata[4] = outidata[13] = outidata[29]						= 6;
	outidata[5] = outidata[12] = outidata[15] = outidata[32] = outidata[34]		= 4;
	outidata[6] = outidata[9] = outidata[23] = outidata[30] = outidata[33]		= 1;
	outidata[7] = outidata[20] = outidata[22] = outidata[25]					= 3;
	outidata[14] = outidata[16] = outidata[19] = outidata[26] = outidata[28]	= 7;
	outidata[17] = outidata[18] = outidata[21] = outidata[35]					= 5;
}

void CManifoldUtils::Create2MSphere(SPositionVertex* outvdata, uint32_t* outidata, float radius, uint16_t vsegments, uint16_t hsegments)
{
}

void CManifoldUtils::NumVerticesIndices2MSphere(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments)
{
}
