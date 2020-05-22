#pragma once
#include "Command/KECommandInvoker.h"
#include "Resource/KEResourcePorter.h"
#include "Manipulator/KEEntityManipulator.h"
#include "Manipulator/KEEntitySelector.h"
#include "Reflection/KEReflectionManager.h"

namespace KEditorGlobal
{
	extern KECommandInvoker CommandInvoker;
	extern KEResourcePorter ResourcePorter;
	extern KEEntityManipulator EntityManipulator;
	extern KEEntitySelector EntitySelector;
	extern KEReflectionManager ReflectionManager;
}