
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
