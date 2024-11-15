#pragma once
#include "Command/KECommandInvoker.h"
#include "Resource/KEResourcePorter.h"
#include "Entity/KEEntityManipulator.h"
#include "Entity/KEEntitySelector.h"
#include "Entity/KEEntityNamePool.h"
#include "Reflection/KEReflectionManager.h"

class QMainWindow;

namespace KEditorGlobal
{
	extern QMainWindow* MainWindow;

	extern KECommandInvoker CommandInvoker;
	extern KEResourcePorter ResourcePorter;
	extern KEEntityManipulator EntityManipulator;
	extern KEEntitySelector EntitySelector;
	extern KEEntityNamePool EntityNamePool;
	extern KEReflectionManager ReflectionManager;
}