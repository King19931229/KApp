#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <utility>

class KCamera
{
public:
	enum PROJECTIVE_MODE
	{
		PM_PERSPECTIVE = 0,
		PM_ORTHO
	};
protected:
	glm::vec3 m_Pos;

	glm::vec3 m_Forward;
	glm::vec3 m_Up;
	glm::vec3 m_Right;

	glm::vec3 m_LockX;
	glm::vec3 m_LockY;
	glm::vec3 m_LockZ;

	glm::mat4 m_View;
	glm::mat4 m_Proj;

	float m_Near;
	float m_Far;
	float m_Fov;
	float m_Aspect;

	float m_Width;
	float m_Height;

	PROJECTIVE_MODE m_Mode;

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

			m_InvaildView = false;
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

			m_InvaildProj = false;
		}
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

	inline const glm::vec3& GetPostion() const { return m_Pos; }
	inline const glm::vec3& GetUp() const { return m_Up; }
	inline const glm::vec3& GetForward() const { return m_Forward; }
	inline const glm::vec3& GetRight() const { return m_Right; }
	inline const glm::mat4& GetViewMatrix() const { return m_View; }
	inline const glm::mat4& GetProjectiveMatrix() const { return m_Proj; }

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
		assert(near > 0.0f && "near must bigger than 0");
		assert(far > 0.0f && "far must bigger than 0");

		m_Width = width;
		m_Height = height;
		m_Near = near;
		m_Far = far;
		m_Mode = PM_ORTHO;

		m_InvaildProj = true;
		UpdateProperty();
		return true;
	}
};