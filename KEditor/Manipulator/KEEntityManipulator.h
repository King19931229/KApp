#pragma once

#include "KBase/Interface/Entity/IKEntity.h"
#include "KRender/Interface/IKGizmo.h"
#include <set>

enum class SelectType
{
	SELECT_TYPE_SINGLE,
	SELECT_TYPE_MULTI,
	SELECT_TYPE_NONE
};

class KEEntityManipulator
{
public:
	typedef std::set<IKEntityPtr> EntityCollectionType;
protected:
	EntityCollectionType m_Entities;
	IKGizmoPtr m_Gizmo;
	SelectType m_SelectMode;
public:
	KEEntityManipulator();
	~KEEntityManipulator();

	bool Init();
	bool UnInit();

	SelectType GetSelectType() const;
	bool SetSelectType(SelectType type);

	GizmoType GetGizmoType() const;
	bool SetGizmoType(GizmoType type);

	GizmoManipulateMode GetManipulateMode() const;
	void SetManipulateMode(GizmoManipulateMode mode);

	void TiggerEntity(IKEntityPtr entity);
	
	template<typename ContainerType>
	void TiggerEntities(const ContainerType& container)
	{
		for (IKEntityPtr entity : container)
		{
			TiggerEntity(entity);
		}
	}

	inline const EntityCollectionType& GetEntites() const { return m_Entities; }
};