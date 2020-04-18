#pragma once
#include "KBase/Interface/IKFileSystem.h"
#include <assert.h>

class KEResourcePathItem
{
protected:
	KEResourcePathItem* m_Parent;
	KEResourcePathItem* m_Child;
	KEFileSystemTreeItem* m_TreeItem;
	int m_Depth;
public:
	KEResourcePathItem(KEResourcePathItem* parent,
		KEFileSystemTreeItem* treeItem,
		int depth)
		: m_Parent(parent),
		m_TreeItem(treeItem),
		m_Child(nullptr),
		m_Depth(depth)
	{
	}
	~KEResourcePathItem()
	{
		SAFE_DELETE(m_Child);
	}

	void SetChild(KEResourcePathItem* child)
	{
		m_Child = child;
		assert(m_Child->m_Parent == this);
	}

	inline KEResourcePathItem* GetParent() const { return m_Parent; }
	inline KEResourcePathItem* GetChild() const { return m_Child; }
	inline int GetDepth() const { return m_Depth; }
	inline const std::string& GetName() const { return m_TreeItem->GetName(); }
	inline const std::string& GetFullPath() const { return m_TreeItem->GetFullPath(); }
	inline KEFileSystemTreeItem* GetTreeItem() const { return m_TreeItem; }
};