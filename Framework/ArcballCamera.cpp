
#include "ArcballCamera.h"

#define ROTATIONAL_SPEED	0.75f	// rad/s

CArcballCamera::CArcballCamera()
{
	mPosition	= glm::vec3(0);
	mAngles		= glm::vec3(0);
	mDistance	= 1;
	mFov		= glm::pi<float>() / 2;
	mAspect		= 4.0f / 3.0f;
	mNear		= 0.1f;
	mFar		= 10.0f;
}

CArcballCamera::~CArcballCamera()
{
}

void CArcballCamera::OrbitRight(float angle)
{
	mAngles[0] += angle * ROTATIONAL_SPEED;
}

void CArcballCamera::OrbitUp(float angle)
{
	mAngles[1] += angle * ROTATIONAL_SPEED;
	mAngles[1] = glm::clamp(mAngles[1], -glm::half_pi<float>(), glm::half_pi<float>());
}

void CArcballCamera::PanRight(float offset)
{
	glm::mat4 rot;
	glm::vec3 right;

	rot = glm::rotate(mAngles[1], glm::vec3(1, 0, 0)) * glm::rotate(mAngles[0], glm::vec3(0, 1, 0));

	right.x = rot[0][0];
	right.y = rot[1][0];
	right.z = rot[2][0];

	mPosition += right * offset;
}

void CArcballCamera::PanUp(float offset)
{
	glm::mat4 rot;
	glm::vec3 up;

	rot = glm::rotate(mAngles[1], glm::vec3(1, 0, 0)) * glm::rotate(mAngles[0], glm::vec3(0, 1, 0));

	up.x = rot[0][1];
	up.y = rot[1][1];
	up.z = rot[2][1];

	mPosition += up * offset;
}

void CArcballCamera::GetViewMatrixAndEyePosition(glm::mat4& outview, glm::vec3& outeye) const
{
	glm::mat4 yaw, pitch;
	glm::vec3 forward;

	yaw = glm::rotate(mAngles[0], glm::vec3(0, 1, 0));
	pitch = glm::rotate(mAngles[1], glm::vec3(1, 0, 0));

	outview = pitch * yaw;

	// don't forget that rotation matrices are different than lookAtRH
	forward.x = -outview[0][2];
	forward.y = -outview[1][2];
	forward.z = -outview[2][2];

	outeye = mPosition - mDistance * forward;

	outview[3][0] = -(outeye[0] * outview[0][0] + outeye[1] * outview[1][0] + outeye[2] * outview[2][0]);
	outview[3][1] = -(outeye[0] * outview[0][1] + outeye[1] * outview[1][1] + outeye[2] * outview[2][1]);
	outview[3][2] = -(outeye[0] * outview[0][2] + outeye[1] * outview[1][2] + outeye[2] * outview[2][2]);
}

void CArcballCamera::GetProjectionMatrix(glm::mat4& outproj) const
{
	outproj = glm::perspectiveFovRH<float>(mFov, mAspect, 1.0f, mNear, mFar);
}
