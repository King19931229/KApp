#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "KBase/Publish/KPlane.h"
#include "KBase/Publish/KAABBBox.h"
#include <utility>

class KCamera
{
public:
	enum ProjectiveMode
	{
		PM_PERSPECTIVE = 0,
		PM_ORTHO
	};

	enum FrustrmPlane
	{
		FP_NEAR,
		FP_FAR,

		FP_LEFT,
		FP_RIGHT,

		FP_BOTTOM,
		FP_TOP,

		FP_COUNT
	};

	enum FrustrmCorner
	{
		FC_NEAR_TOP_LEFT,
		FC_NEAR_TOP_RIGHT,
		FC_NEAR_BOTTOM_RIGHT,
		FC_NEAR_BOTTOM_LEFT,

		FC_FAR_BOTTOM_LEFT,
		FC_FAR_TOP_LEFT,
		FC_FAR_TOP_RIGHT,
		FC_FAR_BOTTOM_RIGHT,

		FC_COUNT
	};

protected:
	ProjectiveMode m_Mode;

	glm::vec3 m_Pos;

	glm::vec3 m_Forward;
	glm::vec3 m_Up;
	glm::vec3 m_Right;

	KAABBBox m_Box;
	KPlane m_Planes[FP_COUNT];
	glm::vec3 m_Corners[FC_COUNT];

	glm::mat4 m_View;
	glm::mat4 m_Proj;

	glm::vec3 m_LockX;
	glm::vec3 m_LockY;
	glm::vec3 m_LockZ;

	float m_Near;
	float m_Far;
	float m_Fov;
	float m_Aspect;

	float m_Width;
	float m_Height;

	bool m_EnableLockX;
	bool m_EnableLockY;
	bool m_EnableLockZ;

	bool m_InvaildView;
	bool m_InvaildProj;

	void UpdateProperty()
	{
		if(m_InvaildView)
		{
			m_View		= glm::lookAtRH(m_Pos, m_Forward + m_Pos, m_Up);

			m_Right		= glm::vec3(m_View[0][0], m_View[1][0], m_View[2][0]);
			m_Up		= glm::vec3(m_View[0][1], m_View[1][1], m_View[2][1]);
			m_Forward	= -glm::vec3(m_View[0][2], m_View[1][2], m_View[2][2]);

			glm::vec3 calcUp = glm::cross(m_Right, m_Forward);

			assert(abs(m_Up.x - calcUp.x) <= 0.00001f);
			assert(abs(m_Up.y - calcUp.y) <= 0.00001f);
			assert(abs(m_Up.z - calcUp.z) <= 0.00001f);	
		}

		if(m_InvaildProj)
		{
			if(m_Mode == PM_PERSPECTIVE)
			{
				float tanHalfFovy = tan(m_Fov * 0.5f);
				m_Height = 2.0f * tanHalfFovy * m_Near;
				m_Width = m_Height * m_Aspect;
				m_Proj = glm::perspectiveRH(m_Fov, m_Aspect, m_Near, m_Far);
			}
			else
			{
				m_Proj = glm::orthoRH(-m_Width * 0.5f, m_Width * 0.5f, -m_Height * 0.5f, m_Height * 0.5f, m_Near, m_Far);
			}
			m_Proj[1][1] *= -1;
		}

		if(m_InvaildView || m_InvaildProj)
		{
			float ratio = m_Far / m_Near;

			float nearHalfWidth = 0.5f * m_Width;
			float nearHalfHeight = 0.5f * m_Height;

			float farHalfWidth = nearHalfWidth * ratio;
			float farHalfHeight = nearHalfHeight * ratio;

			m_Corners[FC_NEAR_TOP_LEFT] = glm::vec3(-nearHalfWidth, nearHalfHeight, -m_Near);
			m_Corners[FC_NEAR_TOP_RIGHT] = glm::vec3(nearHalfWidth, nearHalfHeight, -m_Near);
			m_Corners[FC_NEAR_BOTTOM_RIGHT] = glm::vec3(nearHalfWidth, -nearHalfHeight, -m_Near);
			m_Corners[FC_NEAR_BOTTOM_LEFT] = glm::vec3(-nearHalfWidth, -nearHalfHeight, -m_Near);

			m_Corners[FC_FAR_BOTTOM_LEFT] = glm::vec3(-farHalfWidth, -farHalfHeight, -m_Far);
			m_Corners[FC_FAR_TOP_LEFT] = glm::vec3(-farHalfWidth, farHalfHeight, -m_Far);
			m_Corners[FC_FAR_TOP_RIGHT] = glm::vec3(farHalfWidth, farHalfHeight, -m_Far);
			m_Corners[FC_FAR_BOTTOM_RIGHT] = glm::vec3(farHalfWidth, -farHalfHeight, -m_Far);

			glm::mat4 viewInv = glm::inverse(m_View);

			KAABBBox tempBoxResult;
			tempBoxResult.SetNull();
			m_Box.SetNull();
			for(int i = 0; i < FC_COUNT; ++i)
			{
				m_Corners[i] = viewInv * glm::vec4(m_Corners[i], 1.0f);
				tempBoxResult.Merge(m_Corners[i], tempBoxResult);
			}
			m_Box = tempBoxResult;

			m_Planes[FP_NEAR].Init(m_Corners[FC_NEAR_TOP_LEFT], m_Corners[FC_NEAR_TOP_RIGHT], m_Corners[FC_NEAR_BOTTOM_RIGHT]);
			m_Planes[FP_FAR].Init(m_Corners[FC_FAR_TOP_LEFT], m_Corners[FC_FAR_BOTTOM_LEFT], m_Corners[FC_FAR_BOTTOM_RIGHT]);

			m_Planes[FP_LEFT].Init(m_Corners[FC_FAR_BOTTOM_LEFT], m_Corners[FC_FAR_TOP_LEFT], m_Corners[FC_NEAR_BOTTOM_LEFT]);
			m_Planes[FP_RIGHT].Init(m_Corners[FC_FAR_TOP_RIGHT], m_Corners[FC_FAR_BOTTOM_RIGHT], m_Corners[FC_NEAR_BOTTOM_RIGHT]);

			m_Planes[FP_BOTTOM].Init(m_Corners[FC_NEAR_BOTTOM_LEFT], m_Corners[FC_NEAR_BOTTOM_RIGHT], m_Corners[FC_FAR_BOTTOM_RIGHT]);
			m_Planes[FP_TOP].Init(m_Corners[FC_NEAR_TOP_LEFT], m_Corners[FC_FAR_TOP_LEFT], m_Corners[FC_FAR_TOP_RIGHT]);
		}

		m_InvaildView = false;
		m_InvaildProj = false;
	}

	inline bool CheckLockX(const glm::vec3& newRight) { return m_EnableLockX ? glm::dot(newRight, m_LockX) * glm::dot(m_Right, m_LockX) > 0.0f : true; }
	inline bool CheckLockY(const glm::vec3& newUp) { return m_EnableLockY ? glm::dot(newUp, m_LockY) * glm::dot(m_Up, m_LockY) > 0.0f : true; }
	inline bool CheckLockZ(const glm::vec3& newForward) { return m_EnableLockZ ? glm::dot(newForward, m_LockZ) * glm::dot(m_Forward, m_LockZ) > 0.0f : true; }
public:
	KCamera()
		: m_Near(1.0f),
		m_Far(1000.0f),
		m_Fov(glm::radians(45.0f)),
		m_Aspect(1.0f),
		m_Mode(PM_PERSPECTIVE),
		m_EnableLockX(false),
		m_EnableLockY(false),
		m_EnableLockZ(false)
	{
		m_Pos		= glm::vec3(0.0f);
		m_Up		= glm::vec3(0.0f, 1.0f, 0.0f);
		m_Forward	= glm::vec3(0.0f, 0.0f, -1.0f);
		m_LockX		= glm::vec3(1.0f, 0.0f, 0.0f);
		m_LockY		= glm::vec3(0.0f, 1.0f, 0.0f);
		m_LockZ		= glm::vec3(0.0f, 0.0f, 1.0f);		

		m_InvaildView = true;
		m_InvaildProj = true;

		UpdateProperty();
	}

	inline void SetCustomLockXAxis(const glm::vec3& axis) { m_LockX = axis; }
	inline void SetCustomLockYAxis(const glm::vec3& axis) { m_LockY = axis; }
	inline void SetCustomLockZAxis(const glm::vec3& axis) { m_LockZ = axis; }
	inline void SetLockXEnable(bool enable) { m_EnableLockX = enable; }
	inline void SetLockYEnable(bool enable) { m_EnableLockY = enable; }
	inline void SetLockZEnable(bool enable) { m_EnableLockZ = enable; }

	inline const glm::vec3& GetPosition() const { return m_Pos; }
	inline const glm::vec3& GetUp() const { return m_Up; }
	inline const glm::vec3& GetForward() const { return m_Forward; }
	inline const glm::vec3& GetRight() const { return m_Right; }
	inline const glm::mat4& GetViewMatrix() const { return m_View; }
	inline const glm::mat4& GetProjectiveMatrix() const { return m_Proj; }

	inline float GetNear() const { return m_Near; }
	inline float GetFar() const { return m_Far; }
	inline float GetAspect() const { return m_Aspect; }

	void SetViewMatrix(const glm::mat4& viewMat)
	{
		glm::vec3 right = glm::vec3(viewMat[0][0], viewMat[1][0], viewMat[2][0]);
		glm::vec3 up = glm::vec3(viewMat[0][1], viewMat[1][1], viewMat[2][1]);
		glm::vec3 forward = -glm::vec3(viewMat[0][2], viewMat[1][2], viewMat[2][2]);

		assert(fabs(1.0f - glm::dot(forward, -glm::cross(right, up))) < 0.001f);

		glm::mat4 invView = glm::inverse(viewMat);
		glm::vec3 pos = invView * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		m_Pos = pos;
		m_Right = right;
		m_Up = up;
		m_Forward = forward;

		m_InvaildView = true;
		UpdateProperty();
	}

	void SetPosition(const glm::vec3& pos)
	{
		m_Pos = pos;
		m_InvaildView = true;
		UpdateProperty();
	}

	bool LookAt(const glm::vec3& center, const glm::vec3 up)
	{
		bool bReturn = false;

		glm::vec3 newForward = glm::normalize(center - m_Pos);
		glm::vec3 newUp = glm::normalize(up);
		glm::vec3 newRight = glm::cross(newForward, newUp);

		bReturn = CheckLockX(newRight) && CheckLockY(newUp) && CheckLockZ(newForward);

		if(bReturn)
		{
			m_Forward = newForward;
			m_Up = newUp;
			m_InvaildView = true;
			UpdateProperty();
		}
		return bReturn;
	}

	void Move(const glm::vec3& offset)
	{
		m_Pos += offset;
		m_InvaildView = true;
		UpdateProperty();
	}

	bool MoveForward(float dist)
	{
		m_Pos += dist * m_Forward;
		m_InvaildView = true;
		UpdateProperty();
		return true;
	}

	bool MoveRight(float dist)
	{
		m_Pos += dist * m_Right;
		m_InvaildView = true;
		UpdateProperty();
		return true;
	}

	bool MoveUp(float dist)
	{
		m_Pos += dist * m_Up;
		m_InvaildView = true;
		UpdateProperty();
		return true;
	}

	bool Rotate(const glm::vec3& axis, float radian)
	{
		bool bReturn = false;

		glm::mat4 rotate = glm::transpose(glm::mat4(glm::mat3(m_View)));
		glm::mat4 newRotate = glm::rotate(glm::mat4(1.0f), radian, axis) * rotate;

		glm::vec3 newRight		= glm::vec3(newRotate[0][0], newRotate[0][1], newRotate[0][2]);
		glm::vec3 newUp			= glm::vec3(newRotate[1][0], newRotate[1][1], newRotate[1][2]);
		glm::vec3 newForward	= -glm::vec3(newRotate[2][0], newRotate[2][1], newRotate[2][2]);

		bReturn = CheckLockX(newRight) && CheckLockY(newUp) && CheckLockZ(newForward);

		if(bReturn)
		{
			m_Forward = newForward;
			m_Up = newUp;
			m_InvaildView = true;
			UpdateProperty();
		}

		return bReturn;
	}

	bool RotateForward(float radian)
	{
		bool bReturn = false;
		glm::vec3 newUp = glm::rotate(glm::mat4(1.0f), radian, m_Forward) * glm::vec4(m_Up, 0.0f);
		glm::vec3 newRight = glm::cross(m_Forward, newUp);
		bReturn = CheckLockY(newUp) && CheckLockX(newRight);

		if(bReturn)
		{
			m_Up = newUp;
			m_Right = newRight;
			m_InvaildView = true;
			UpdateProperty();
		}
		return bReturn;
	}

	bool RotateUp(float radian)
	{
		bool bReturn = false;
		glm::vec3 newRight = glm::rotate(glm::mat4(1.0f), radian, m_Up) * glm::vec4(m_Right, 0.0f);
		glm::vec3 newForward = glm::cross(m_Up, newRight);
		bReturn = CheckLockX(newRight) && CheckLockZ(newForward);

		if(bReturn)
		{
			m_Right = newRight;
			m_Forward = newForward;
			m_InvaildView = true;
			UpdateProperty();
		}
		return bReturn;
	}

	bool RotateRight(float radian)
	{
		bool bReturn = false;
		glm::vec3 newForward = glm::rotate(glm::mat4(1.0f), radian, m_Right) * glm::vec4(m_Forward, 0.0f);
		glm::vec3 newUp = glm::cross(m_Right, newForward);
		bReturn = CheckLockY(newUp) && CheckLockZ(newForward);

		if(bReturn)
		{
			m_Forward = newForward;
			m_Up = newUp;
			m_InvaildView = true;
			UpdateProperty();
		}
		return bReturn;
	}

	bool SetPerspective(float fov, float aspect, float near, float far)
	{
		assert(fov > 0.0f && "fov must bigger than 0");
		assert(aspect > 0.0f && "aspect must bigger than 0");
		assert(near > 0.0f && "near must bigger than 0");
		assert(far > 0.0f && "far must bigger than 0");

		m_Fov = fov;
		m_Aspect = aspect;
		m_Near = near;
		m_Far = far;
		m_Mode = PM_PERSPECTIVE;

		m_InvaildProj = true;
		UpdateProperty();
		return true;
	}

	bool SetOrtho(float width, float height, float near, float far)
	{
		assert(width > 0.0f && "width must bigger than 0");
		assert(height > 0.0f && "height must bigger than 0");
		assert(far >= near && "near must bigger than far");

		m_Width = width;
		m_Height = height;
		m_Near = near;
		m_Far = far;
		m_Mode = PM_ORTHO;

		m_InvaildProj = true;
		UpdateProperty();
		return true;
	}

	bool CheckVisible(const KAABBBox& box) const
	{
		glm::vec3 halfSize = box.GetExtend() * 0.5f;
		glm::vec3 center = box.GetCenter();
		for(int i = 0; i < FP_COUNT; ++i)
		{
			if(m_Planes[i].GetSide(center, halfSize) == KPlane::PS_NEGATIVE)
			{
				return false;
			}
		}
		return true;
	}

	bool CheckVisibleFast(const KAABBBox& box) const
	{
		return m_Box.Intersect(box);
	}

	// [x] [y] in extend of [-1,1]
	bool CalcPickRay(float x, float y, glm::vec3& origin, glm::vec3& dir) const
	{
		glm::vec4 near = glm::vec4(x, y,
#ifdef GLM_FORCE_DEPTH_ZERO_TO_ONE
			0.0f,
#else
			- 1.0f,
#endif
			1.0f
		);
		glm::vec4 far = glm::vec4(near.x, near.y, 1.0f, 1.0f);

		glm::mat4 vp = GetProjectiveMatrix() * GetViewMatrix();
		glm::mat4 inv_vp = glm::inverse(vp);

		near = inv_vp * near;
		far = inv_vp * far;

		glm::vec3 nearPos = glm::vec3(near.x, near.y, near.z) / near.w;
		glm::vec3 farPos = glm::vec3(far.x, far.y, far.z) / far.w;

		origin = nearPos;
		dir = glm::normalize(farPos - nearPos);
		return true;
	}

	bool CalcPickRay(size_t x, size_t y, size_t screenWidth, size_t screenHeight,
		glm::vec3& origin, glm::vec3& dir) const
	{
		if (screenWidth && screenHeight)
		{
			return CalcPickRay(
				2.0f * ((float)x / (float)screenWidth) - 1.0f,
				2.0f * ((float)y / (float)screenHeight) - 1.0f,
				origin, dir);
		}
		return false;
	}
};