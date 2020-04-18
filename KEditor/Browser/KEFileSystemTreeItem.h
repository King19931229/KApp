#pragma once
#include "KBase/Interface/IKFileSystem.h"

class KEFileSystemTreeItem
{
protected:
	IKFileSystem* m_System;
	std::string m_Name;
	std::string m_FullPath;

	std::vector<KEFileSystemTreeItem*> m_Files;
	std::vector<KEFileSystemTreeItem*> m_Dirs;

	KEFileSystemTreeItem* m_Parent;
	int m_Index;
	bool m_IsDir;
	bool m_NeedListDir;

	void Clear()
	{
		for (KEFileSystemTreeItem* item : m_Files)
		{
			SAFE_DELETE(item);
		}
		m_Files.clear();

		for (KEFileSystemTreeItem* item : m_Dirs)
		{
			SAFE_DELETE(item);
		}
		m_Dirs.clear();

		m_NeedListDir = true;
	}
public:
	KEFileSystemTreeItem(IKFileSystem* system,
		const std::string& name,
		const std::string& fullPath,
		KEFileSystemTreeItem* parent,
		int index,
		bool isDir)
		: m_System(system),
		m_Name(name),
		m_FullPath(fullPath),
		m_Parent(parent),
		m_Index(index),
		m_IsDir(isDir),
		m_NeedListDir(true)
	{
	}

	~KEFileSystemTreeItem()
	{
		Clear();
	}

	void Refresh()
	{
		Clear();
	}

	void ListDirInNeed()
	{
		if (m_NeedListDir)
		{
			if (m_IsDir)
			{
				std::vector<std::string> listDir;
				m_System->ListDir(m_FullPath, listDir);

				int index = 0;
				for (const std::string& subPath : listDir)
				{
					KEFileSystemTreeItem* newItem = nullptr;

					std::string fullSubPath;
					m_System->FullPath(m_FullPath, subPath, fullSubPath);

					bool isDir = m_System->IsDir(fullSubPath);

					newItem = new KEFileSystemTreeItem(m_System,
						subPath,
						fullSubPath,
						this,
						isDir ? index++ : 0,
						isDir);

					if (isDir)
					{
						m_Dirs.push_back(newItem);
					}
					else
					{
						m_Files.push_back(newItem);
					}
				}
			}
			m_NeedListDir = false;
		}
	}

	KEFileSystemTreeItem* GetDir(size_t index)
	{
		ListDirInNeed();
		if (index < m_Dirs.size())
		{
			return m_Dirs[index];
		}
		return nullptr;
	}

	KEFileSystemTreeItem* GetFile(size_t index)
	{
		ListDirInNeed();
		if (index < m_Files.size())
		{
			return m_Files[index];
		}
		return nullptr;
	}

	size_t GetNumDir()
	{
		ListDirInNeed();
		return m_Dirs.size();
	}

	size_t GetNumFile()
	{
		ListDirInNeed();
		return m_Files.size();
	}

	inline KEFileSystemTreeItem* GetParent() const { return m_Parent; }
	inline int GetIndex() const { return m_Index; }
	inline const std::string& GetName() const { return m_Name; }
	inline const std::string& GetFullPath() const { return m_FullPath; }
};