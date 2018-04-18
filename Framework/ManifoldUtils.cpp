
#include <cassert>
#include <cmath>
#include <algorithm>
#include <glm/gtc/constants.hpp>

#include "ManifoldUtils.h"

static uint32_t FindAdjacentIndex(uint32_t i1, uint32_t i2, uint32_t i3, uint32_t e1, uint32_t e2)
{
	// NOTE: can be simplified
	if (e1 == i1) {
		if (e2 == i2) {
			return i3;
		} else if (e2 == i3) {
			return i2;
		}
	} else if (e1 == i2) {
		if (e2 == i1) {
			return i3;
		} else if (e2 == i3) {
			return i1;
		}
	} else if (e1 == i3) {
		if (e2 == i2) {
			return i1;
		} else if (e2 == i1) {
			return i2;
		}
	}

	return UINT32_MAX;
}

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
	assert(radius > 0.0f);

	vsegments = std::max<uint16_t>(vsegments, 4);
	hsegments = std::max<uint16_t>(hsegments, 4);

	float theta, phi;

	// body vertices
	for (int j = 1; j < hsegments - 1; ++j) {
		for (int i = 0; i < vsegments; ++i) {
			theta = ((float)j / (float)(hsegments - 1)) * glm::pi<float>();
			phi = ((float)i / (float)vsegments) * glm::two_pi<float>();

			outvdata->x = sinf(theta) * cosf(phi) * radius;
			outvdata->y = cosf(theta) * radius;
			outvdata->z = -sinf(theta) * sinf(phi) * radius;

			++outvdata;
		}
	}

	// top vertex
	outvdata->x = outvdata->z = 0;
	outvdata->y = radius;

	++outvdata;

	// bottom vertex
	outvdata->x = outvdata->z = 0;
	outvdata->y = -radius;

	// indices
	for (int j = 0; j < hsegments - 3; ++j) {
		for (int i = 0; i < vsegments; ++i) {
			outidata[0] = j * vsegments + i;
			outidata[1] = (j + 1) * vsegments + (i + 1) % vsegments;
			outidata[2] = j * vsegments + (i + 1) % vsegments;

			outidata[3] = j * vsegments + i;
			outidata[4] = (j + 1) * vsegments + i;
			outidata[5] = (j + 1) * vsegments + (i + 1) % vsegments;

			outidata += 6;
		}
	}

	for (int i = 0; i < vsegments; ++i) {
		// top
		outidata[0] = (hsegments - 2) * vsegments;
		outidata[1] = i;
		outidata[2] = (i + 1) % vsegments;

		// bottom
		outidata[3] = (hsegments - 2) * vsegments + 1;
		outidata[4] = (hsegments - 3) * vsegments + (i + 1) % vsegments;
		outidata[5] = (hsegments - 3) * vsegments + i;

		outidata += 6;
	}
}

void CManifoldUtils::GenerateGSAdjacency(uint32_t* outidata, uint32_t* idata, uint32_t numindices)
{
	// NOTE: assumes that input is 2-manifold triangle list
	assert((numindices % 3) == 0);
	assert(outidata != idata);

	uint32_t numtriangles = numindices / 3;

	for (uint32_t i = 0; i < numtriangles; ++i) {
		// indices of base triangle
		uint32_t i1 = idata[i * 3 + 0];	// 0
		uint32_t i2 = idata[i * 3 + 1];	// 2
		uint32_t i3 = idata[i * 3 + 2];	// 4

		uint32_t adj1 = UINT32_MAX;		// 1
		uint32_t adj3 = UINT32_MAX;		// 3
		uint32_t adj5 = UINT32_MAX;		// 5

		// find adjacent indices
		for (uint32_t j = 0; j < numtriangles; ++j) {
			if (i == j)
				continue;

			uint32_t j1 = idata[j * 3 + 0];
			uint32_t j2 = idata[j * 3 + 1];
			uint32_t j3 = idata[j * 3 + 2];

			if (adj1 == UINT32_MAX)
				adj1 = FindAdjacentIndex(j1, j2, j3, i1, i2);

			if (adj3 == UINT32_MAX)
				adj3 = FindAdjacentIndex(j1, j2, j3, i2, i3);

			if (adj5 == UINT32_MAX)
				adj5 = FindAdjacentIndex(j1, j2, j3, i3, i1);
			
			if (adj1 < UINT32_MAX && adj3 < UINT32_MAX && adj5 < UINT32_MAX)
				break;
		}

		// write adjacency data
		outidata[0] = i1;
		outidata[1] = adj1;
		outidata[2] = i2;
		outidata[3] = adj3;
		outidata[4] = i3;
		outidata[5] = adj5;

		outidata += 6;
	}
}

void CManifoldUtils::NumVerticesIndices2MSphere(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments)
{
	outnumverts = (hsegments - 2) * vsegments + 2;
	outnuminds = (hsegments - 2) * vsegments * 6;
}
