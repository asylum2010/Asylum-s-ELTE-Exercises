
#include <cassert>
#include <cmath>
#include <algorithm>
#include <glm/gtc/constants.hpp>

#include "GeometryUtils.h"

CGeometryUtils::CGeometryUtils()
{
}

CGeometryUtils::~CGeometryUtils()
{
}

void CGeometryUtils::CreateBox(SCommonVertex* outvdata, uint32_t* outidata, float width, float height, float depth)
{
	outvdata[0] = { width * -0.5f, height * -0.5f, depth * 0.5f,	1, 0,	0, -1, 0 };
	outvdata[1] = { width * -0.5f, height * -0.5f, depth * -0.5f,	1, 1,	0, -1, 0 };
	outvdata[2] = { width * 0.5f, height * -0.5f, depth * -0.5f,	0, 1,	0, -1, 0 };
	outvdata[3] = { width * 0.5f, height * -0.5f, depth * 0.5f,		0, 0,	0, -1, 0 };

	outvdata[4] = { width * -0.5f, height * 0.5f, depth * 0.5f,		0, 0,	0, 1, 0 };
	outvdata[5] = { width * 0.5f, height * 0.5f, depth * 0.5f,		1, 0,	0, 1, 0 };
	outvdata[6] = { width * 0.5f, height * 0.5f, depth * -0.5f,		1, 1,	0, 1, 0 };
	outvdata[7] = { width * -0.5f, height * 0.5f, depth * -0.5f,	0, 1,	0, 1, 0 };

	outvdata[8] = { width * -0.5f, height * -0.5f, depth * 0.5f,	0, 0,	0, 0, 1 };
	outvdata[9] = { width * 0.5f, height * -0.5f, depth * 0.5f,		1, 0,	0, 0, 1 };
	outvdata[10] = { width * 0.5f, height * 0.5f, depth * 0.5f,		1, 1,	0, 0, 1 };
	outvdata[11] = { width * -0.5f, height * 0.5f, depth * 0.5f,	0, 1,	0, 0, 1 };

	outvdata[12] = { width * 0.5f, height * -0.5f, depth * 0.5f,	0, 0,	1, 0, 0 };
	outvdata[13] = { width * 0.5f, height * -0.5f, depth * -0.5f,	1, 0,	1, 0, 0 };
	outvdata[14] = { width * 0.5f, height * 0.5f, depth * -0.5f,	1, 1,	1, 0, 0 };
	outvdata[15] = { width * 0.5f, height * 0.5f, depth * 0.5f,		0, 1,	1, 0, 0 };

	outvdata[16] = { width * 0.5f, height * -0.5f, depth * -0.5f,	0, 0,	0, 0, -1 };
	outvdata[17] = { width * -0.5f, height * -0.5f, depth * -0.5f,	1, 0,	0, 0, -1 };
	outvdata[18] = { width * -0.5f, height * 0.5f, depth * -0.5f,	1, 1,	0, 0, -1 };
	outvdata[19] = { width * 0.5f, height * 0.5f, depth * -0.5f,	0, 1,	0, 0, -1 };

	outvdata[20] = { width * -0.5f, height * -0.5f, depth * -0.5f,	0, 0,	-1, 0, 0 };
	outvdata[21] = { width * -0.5f, height * -0.5f, depth * 0.5f,	1, 0,	-1, 0, 0 };
	outvdata[22] = { width * -0.5f, height * 0.5f, depth * 0.5f,	1, 1,	-1, 0, 0 };
	outvdata[23] = { width * -0.5f, height * 0.5f, depth * -0.5f,	0, 1,	-1, 0, 0 };

	uint32_t indices[36] = {
		0, 1, 2, 2, 3, 0, 
		4, 5, 6, 6, 7, 4,
		8, 9, 10, 10, 11, 8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20
	};

	memcpy(outidata, indices, 36 * sizeof(uint32_t));
}

void CGeometryUtils::CreateSphere(SCommonVertex* outvdata, uint32_t* outidata, float radius, uint16_t vsegments, uint16_t hsegments)
{
	assert(radius > 0.0f);

	vsegments = std::max<uint16_t>(vsegments, 4);
	hsegments = std::max<uint16_t>(hsegments, 4);

	float theta, phi;

	// body vertices
	for (int j = 1; j < hsegments - 1; ++j) {
		for (int i = 0; i < vsegments; ++i) {
			outvdata->u = ((float)i / (float)(vsegments - 1));
			outvdata->v = ((float)j / (float)(hsegments - 1));

			theta = outvdata->v * glm::pi<float>();
			phi = outvdata->u * glm::two_pi<float>();

			outvdata->x = sinf(theta) * cosf(phi) * radius;
			outvdata->y = cosf(theta) * radius;
			outvdata->z = -sinf(theta) * sinf(phi) * radius;

			outvdata->nx = outvdata->x / radius;
			outvdata->ny = outvdata->y / radius;
			outvdata->nz = outvdata->z / radius;

			++outvdata;
		}
	}

	// top vertices
	for (int i = 0; i < vsegments - 1; ++i) {
		outvdata->x = outvdata->z = 0;
		outvdata->y = radius;

		outvdata->nx = outvdata->nz = 0;
		outvdata->ny = 1;

		outvdata->u = ((float)i / (float)(vsegments - 1));
		outvdata->v = 0;

		++outvdata;
	}

	// bottom vertices
	for (int i = 0; i < vsegments - 1; ++i) {
		outvdata->x = outvdata->z = 0;
		outvdata->y = -radius;

		outvdata->nx = outvdata->nz = 0;
		outvdata->ny = -1;

		outvdata->u = ((float)i / (float)(vsegments - 1));
		outvdata->v = 1;

		++outvdata;
	}

	// indices
	for (int j = 0; j < hsegments - 3; ++j) {
		for (int i = 0; i < vsegments - 1; ++i) {
			outidata[0] = j * vsegments + i;
			outidata[1] = (j + 1) * vsegments + i + 1;
			outidata[2] = j * vsegments + i + 1;

			outidata[3] = j * vsegments + i;
			outidata[4] = (j + 1) * vsegments + i;
			outidata[5] = (j + 1) * vsegments + i + 1;

			outidata += 6;
		}
	}

	for (int i = 0; i < vsegments - 1; ++i) {
		// top
		outidata[0] = (hsegments - 2) * vsegments + i;
		outidata[1] = i;
		outidata[2] = i + 1;

		// bottom
		outidata[3] = (hsegments - 2) * vsegments + (vsegments - 1) + i;
		outidata[4] = (hsegments - 3) * vsegments + i + 1;
		outidata[5] = (hsegments - 3) * vsegments + i;

		outidata += 6;
	}
}

void CGeometryUtils::NumVerticesIndicesSphere(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments)
{
	outnumverts = (hsegments - 2) * vsegments + 2 * (vsegments - 1);
	outnuminds = (hsegments - 2) * (vsegments - 1) * 6;
}
