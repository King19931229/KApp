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

	inline const glm::vec3& GetPostion() const { return m_Pos; }

	inline const glm::vec3& GetUp() const { return m_Up; }
	inline const glm::vec3& GetForward() const { return m_Forward; }
	inline const glm::vec3& GetRight() const { return m_Right; }

	inline const glm::mat4& GetViewMatrix() const { return m_View; }
	inline const glm::mat4& GetProjectiveMatrix() const { return m_Proj; }

	bool SetPosition(const glm::vec3& pos)
	{
		m_Pos = pos;
		UpdateProperty();
		return true;
	}

	bool LookAt(const glm::vec3& center, const glm::vec3 up)
	{
		bool bReturn = false;

		glm::vec3 newForward = glm::normalize(center - m_Pos);
		glm::vec3 newUp = glm::normalize(up);
		glm::vec3 newRight = glm::cross(newForward, newUp);

		if(m_EnableLockZ)
		{
			if(glm::dot(m_LockZ - newRight, m_LockZ - m_Right) <= 0.0f ||
				glm::dot(-m_LockZ - newRight, -m_LockZ - m_Right) <= 0.0f)
			{
				bReturn = false;
			}
		}
		else if(m_EnableLockY)
		{
			if(glm::dot(m_LockY - newForward, m_LockY - m_Forward) <= 0.0f ||
				glm::dot(-m_LockY - newForward, -m_LockY - m_Forward) <= 0.0f)
			{
				bReturn = false;
			}
		}
		else if(m_EnableLockY)
		{
			if(glm::dot(m_LockY - newForward, m_LockY - m_Forward) <= 0.0f ||
				glm::dot(-m_LockY - newForward, -m_LockY - m_Forward) <= 0.0f)
			{
				bReturn = false;
			}
		}
		else
		{
			bReturn = true;
		}

		if(bReturn)
		{
			m_Forward = newForward;
			m_Up = newUp;
			m_InvaildView = true;
			UpdateProperty();
		}

		return bReturn;
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

	bool RotateForward(float radian)
	{
		bool bReturn = false;
		glm::vec3 newUp = glm::rotate(glm::mat4(1.0f), radian, m_Forward) * glm::vec4(m_Up, 0.0f);
		if(m_EnableLockX)
		{
			if(glm::dot(m_LockX - newUp, m_LockX - m_Up) <= 0.0f ||
				glm::dot(-m_LockX - newUp, -m_LockX - m_Up) <= 0.0f)
			{
				bReturn = false;
			}
		}
		else
		{
			bReturn = true;
		}
		if(bReturn)
		{
			m_Up = std::move(newUp);
			m_InvaildView = true;
			UpdateProperty();
		}
		return bReturn;
	}

	bool RotateUp(float radian)
	{
		bool bReturn = false;
		glm::vec3 newRight = glm::rotate(glm::mat4(1.0f), radian, m_Up) * glm::vec4(m_Right, 0.0f);
		if(m_EnableLockZ)
		{
			if(glm::dot(m_LockZ - newRight, m_LockZ - m_Right) <= 0.0f ||
				glm::dot(-m_LockZ - newRight, -m_LockZ - m_Right) <= 0.0f)
			{
				bReturn = false;
			}
		}
		else
		{
			bReturn = true;
		}
		if(bReturn)
		{
			m_Right = newRight;
			m_Forward = -glm::cross(newRight, m_Up);
			m_InvaildView = true;
			UpdateProperty();
		}
		return bReturn;
	}

	bool RotateRight(float radian)
	{
		bool bReturn = false;
		glm::vec3 newForward = glm::rotate(glm::mat4(1.0f), radian, m_Right) * glm::vec4(m_Forward, 0.0f);
		if(m_EnableLockY)
		{
			if(glm::dot(m_LockY - newForward, m_LockY - m_Forward) <= 0.0f ||
				glm::dot(-m_LockY - newForward, -m_LockY - m_Forward) <= 0.0f)
			{
				bReturn = false;
			}
		}
		else
		{
			bReturn = true;
		}
		if(bReturn)
		{
			m_Forward = newForward;
			m_Up = glm::cross(m_Right, newForward);
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