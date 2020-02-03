#pragma once
#include <unordered_map>
#include <functional>

#include "Node/KEGraphNodeModel.h"
#include "Utility/QStringStdHash.hpp"

class KEGraphRegistrar
{
protected:
	typedef std::unordered_map<QString, KEGraphNodeModelCreateFunc> CreateFuncMap;
	CreateFuncMap m_CreateFuncMap;
public:
	KEGraphRegistrar();
	~KEGraphRegistrar();

	bool Init();
	bool UnInit();

	bool RegisterGraphModel(const QString& name, KEGraphNodeModelCreateFunc func);
	bool UnRegisterGraphModel(const QString& name);

	typedef std::function<void(const QString& name, KEGraphNodeModelCreateFunc func)> ModelVisitFunc;
	void VisitModel(ModelVisitFunc func);

	KEGraphNodeModelPtr GetNodeModel(const QString& name);
};