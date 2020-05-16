#pragma once
#include "KBase/Interface/IKReflection.h"
#include "Property/KEPropertyBaseView.h"
#include <unordered_set>

struct KEReflectionObjectItem;
typedef std::shared_ptr<KEReflectionObjectItem> KEReflectionObjectItemPtr;

struct KEReflectionObjectItem
{
public:
	enum ObjectMemberType
	{
		OBJECT_MEMBER_TYPE_PROPERTY,
		OBJECT_MEMBER_TYPE_SUB_OBJECT,
		OBJECT_MEMBER_TYPE_SUB_NONE
	};
protected:
	std::string m_Name;
	ObjectMemberType m_Type;
	size_t m_Index;
	size_t m_NumChildren;

	KReflectionObjectBase* m_Object;
	std::unordered_set<KReflectionObjectBase*> m_ParentObjects;

	KEReflectionObjectItem* m_Parent;
	KEReflectionObjectItem** m_Children;

	KEReflectionObjectItem(const KEReflectionObjectItem& rhs) = delete;
	KEReflectionObjectItem& operator=(const KEReflectionObjectItem& rhs) = delete;

	KEReflectionObjectItem* FindChild(const std::string& name);
public:
	KEReflectionObjectItem();
	KEReflectionObjectItem(KEReflectionObjectItem* parent, KReflectionObjectBase* object, const std::string& name);
	KEReflectionObjectItem(KEReflectionObjectItem* parent, const std::string& name);
	~KEReflectionObjectItem();

	KEReflectionObjectItem* GetChild(size_t childIndex);

	inline KEReflectionObjectItem* GetParent() { return m_Parent; }
	inline size_t GetChildCount() const { return m_NumChildren; }
	inline const std::string& GetName() const { return m_Name; }
	inline size_t GetIndex() const { return m_Index; }
	inline ObjectMemberType GetType() const { return m_Type; }

	void Merge(KReflectionObjectBase* object);
	KEPropertyBaseView::BasePtr CreateView();
};