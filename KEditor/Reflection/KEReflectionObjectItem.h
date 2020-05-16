#pragma once
#include "KBase/Interface/IKReflection.h"
#include "Property/KEPropertyBaseView.h"

struct KEReflectionObjectItem;
typedef std::shared_ptr<KEReflectionObjectItem> KEReflectionObjectItemPtr;

struct KEReflectionObjectItem
{
public:
	enum ObjectMemberType
	{
		OBJECT_MEMBER_TYPE_PROPERTY,
		OBJECT_MEMBER_TYPE_SUB_OBJECT
	};
protected:
	KEReflectionObjectItem* m_Parent;
	std::string m_Name;
	KReflectionObjectBase* m_Object;
	ObjectMemberType m_Type;

	size_t m_Index;

	KEReflectionObjectItem** m_Children;
	size_t m_NumChildren;

	KEReflectionObjectItem(const KEReflectionObjectItem& rhs) = delete;
	KEReflectionObjectItem& operator=(const KEReflectionObjectItem& rhs) = delete;
public:
	KEReflectionObjectItem();
	KEReflectionObjectItem(KEReflectionObjectItem* parent, KReflectionObjectBase* object);
	KEReflectionObjectItem(KEReflectionObjectItem* parent, const std::string& name);
	~KEReflectionObjectItem();

	KEReflectionObjectItem* GetChild(size_t childIndex);

	inline KEReflectionObjectItem* GetParent() { return m_Parent; }
	inline size_t GetChildCount() const { return m_NumChildren; }
	inline const std::string& GetName() const { return m_Name; }
	inline size_t GetIndex() const { return m_Index; }
	inline ObjectMemberType GetType() const { return m_Type; }

	KEPropertyBaseView::BasePtr CreateView();
};