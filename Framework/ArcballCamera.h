
#ifndef _ARCBALLCAMERA_H_
#define _ARCBALLCAMERA_H_

#include <glm/glm.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/constants.hpp>

class CArcballCamera
{
private:
	glm::vec3	mPosition;	// lookat point
	glm::vec3	mAngles;	// Euler angles
	float		mDistance;	// distance from lookat
	float		mFov;
	float		mAspect;
	float		mNear;
	float		mFar;

public:
	CArcballCamera();
	virtual ~CArcballCamera();

	void OrbitRight(float angle);
	void OrbitUp(float angle);
	void PanRight(float offset);
	void PanUp(float offset);

	void GetViewMatrixAndEyePosition(glm::mat4& outview, glm::vec3& outeye) const;
	void GetProjectionMatrix(glm::mat4& outproj) const;

	inline float GetDistance() const					{ return mDistance; }

	inline void SetPosition(const glm::vec3& pos)		{ mPosition = pos; }
	inline void SetOrientation(const glm::vec3& ypr)	{ mAngles = ypr; }
	inline void SetAspect(float value)					{ mAspect = value; }
	inline void SetFov(float value)						{ mFov = value; }
	inline void SetClipPlanes(float pnear, float pfar)	{ mNear = pnear; mFar = pfar; }
	inline void SetDistance(float value)				{ mDistance = value; }
};

#endif
