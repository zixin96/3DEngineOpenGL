#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class CameraPositionerInterface
{
public:
	virtual ~CameraPositionerInterface() = default;

	virtual glm::mat4 getViewMatrix() const = 0;
	virtual glm::vec3 getPosition() const = 0;
};

class CameraPositionerFirstPerson : public CameraPositionerInterface
{
public:
	CameraPositionerFirstPerson() = default;
	CameraPositionerFirstPerson(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up);

	void update(double deltaSeconds, const glm::vec2& mousePos, bool mousePressed);

	glm::mat4 getViewMatrix() const override;
	glm::vec3 getPosition() const override;

	void setPosition(const glm::vec3& pos);
	void setUpVector(const glm::vec3& up);

	void resetMousePosition(const glm::vec2& p);

	struct Movement
	{
		bool forward   = false;
		bool backward  = false;
		bool left      = false;
		bool right     = false;
		bool up        = false;
		bool down      = false;
		bool fastSpeed = false;
	}        mMovement;

	// how responsive the camera will be to acceleration and damping.
	// tweak these as you see fit
	float mMouseSpeed = 4.0f;
	float mAccel      = 50.0f;
	float mDamping    = 0.1f;
	float mMaxSpeed   = 10.0f;
	float mFastCoef   = 10.0f;

private:
	glm::vec2 mMousePos          = glm::vec2(0.f);
	glm::vec3 mCameraPosition    = glm::vec3(0.0f, 10.f, 10.0f);
	glm::quat mCameraOrientation = glm::quat(glm::vec3(0.f));
	glm::vec3 mMoveSpeed         = glm::vec3(0.f);
	glm::vec3 mUp                = glm::vec3(0.0f, 1.0f, 0.0f);
};

class Camera
{
public:
	explicit  Camera(CameraPositionerInterface& positioner);
	glm::mat4 getViewMatrix() const;
	glm::vec3 getPosition() const;
private:
	CameraPositionerInterface& mPositioner;
};
