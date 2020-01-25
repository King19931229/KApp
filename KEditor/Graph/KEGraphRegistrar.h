#pragma once

#include "Node/KEGraphNodeModel.h"
#include "Utility/QStringStdHash.hpp"
#include <unordered_map>
#include <functional>

class KEGraphRegistrar
{
	typedef std::function<KEGraphNodeModelPtr(void)> GraphNodeModelCreateFunc;
	typedef std::unordered_map<QString, GraphNodeModelCreateFunc> CreateFuncMap;
	static CreateFuncMap ms_CreateFuncMap;
public:
	static bool Init();
	static bool UnInit();
	static bool RegisterGraphModel(const QString& name, GraphNodeModelCreateFunc func);
	static bool UnRegisterGraphModel(const QString& name);
	static KEGraphNodeModelPtr GetNodeModel(const QString& name);
};