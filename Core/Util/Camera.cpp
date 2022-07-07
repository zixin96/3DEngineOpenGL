#include "Camera.h"

CameraPositionerFirstPerson::CameraPositionerFirstPerson(const glm::vec3& pos,
                                                         const glm::vec3& target,
                                                         const glm::vec3& up)
	: mCameraPosition(pos), mCameraOrientation(glm::lookAt(pos, target, up)), mUp(up)
{
}

void CameraPositionerFirstPerson::update(double deltaSeconds, const glm::vec2& mousePos, bool mousePressed)
{
	if (mousePressed)
	{
		const glm::vec2 delta     = mousePos - mMousePos;
		const glm::quat deltaQuat = glm::quat(glm::vec3(mMouseSpeed * delta.y, mMouseSpeed * delta.x, 0.f));
		mCameraOrientation        = glm::normalize(deltaQuat * mCameraOrientation);
		//!? Note: this line is important. It prevents unwanted camera roll from happening. 
		setUpVector(mUp);
	}
	mMousePos = mousePos;

	const glm::mat4 v = glm::mat4_cast(mCameraOrientation);

	// extract forward, right, and up vector from view matrix
	const glm::vec3 forward = -glm::vec3(v[0][2], v[1][2], v[2][2]);
	const glm::vec3 right   = glm::vec3(v[0][0], v[1][0], v[2][0]);
	const glm::vec3 up      = glm::cross(right, forward);

	glm::vec3 accel(0.0f);

	if (mMovement.forward) accel += forward;
	if (mMovement.backward) accel -= forward;

	if (mMovement.left) accel -= right;
	if (mMovement.right) accel += right;

	if (mMovement.up) accel += up;
	if (mMovement.down) accel -= up;

	if (mMovement.fastSpeed) accel *= mFastCoef;

	if (accel == glm::vec3(0))
	{
		// decelerate naturally according to the damping value
		mMoveSpeed -= mMoveSpeed * std::min((1.0f / mDamping) * static_cast<float>(deltaSeconds), 1.0f);
	}
	else
	{
		// acceleration
		mMoveSpeed += accel * mAccel * static_cast<float>(deltaSeconds);
		const float maxSpeed = mMovement.fastSpeed ? mMaxSpeed * mFastCoef : mMaxSpeed;
		if (glm::length(mMoveSpeed) > maxSpeed) mMoveSpeed = glm::normalize(mMoveSpeed) * maxSpeed;
	}

	mCameraPosition += mMoveSpeed * static_cast<float>(deltaSeconds);
}

glm::mat4 CameraPositionerFirstPerson::getViewMatrix() const
{
	const glm::mat4 t = glm::translate(glm::mat4(1.0f), -mCameraPosition);
	const glm::mat4 r = glm::mat4_cast(mCameraOrientation);
	return r * t;
}

glm::vec3 CameraPositionerFirstPerson::getPosition() const
{
	return mCameraPosition;
}

void CameraPositionerFirstPerson::setPosition(const glm::vec3& pos)
{
	mCameraPosition = pos;
}

void CameraPositionerFirstPerson::setUpVector(const glm::vec3& up)
{
	const glm::mat4 view = getViewMatrix();
	const glm::vec3 dir  = -glm::vec3(view[0][2], view[1][2], view[2][2]);
	mCameraOrientation   = glm::lookAt(mCameraPosition, mCameraPosition + dir, up);
}

void CameraPositionerFirstPerson::resetMousePosition(const glm::vec2& p)
{
	mMousePos = p;
}

Camera::Camera(CameraPositionerInterface& positioner)
	: mPositioner(positioner)
{
}

glm::mat4 Camera::getViewMatrix() const
{
	return mPositioner.getViewMatrix();
}

glm::vec3 Camera::getPosition() const
{
	return mPositioner.getPosition();
}
