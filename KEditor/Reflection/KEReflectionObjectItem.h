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

	KEPropertyBaseView::BasePtr m_PropertyView;

	KEReflectionObjectItem(const KEReflectionObjectItem& rhs) = delete;
	KEReflectionObjectItem& operator=(const KEReflectionObjectItem& rhs) = delete;

	KEReflectionObjectItem* FindChild(const std::string& name);
	KEPropertyBaseView::BasePtr CreatePropertyView();
	void RefreshPropertyView();
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
	inline KEPropertyBaseView::BasePtr GetPropertyView() { return m_PropertyView; }

	void Merge(KReflectionObjectBase* object);	
	void Refresh();
	void RefreshAccuraetly(KReflectionObjectBase* object);
};