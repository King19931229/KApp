#pragma once
#include "Command/KECommandInvoker.h"
#include "Resource/KEResourceImporter.h"
#include "Manipulator/KEEntityManipulator.h"
#include "Manipulator/KEEntitySelector.h"
#include "Reflection/KEReflectionManager.h"

namespace KEditorGlobal
{
	extern KECommandInvoker CommandInvoker;
	extern KEResourceImporter ResourceImporter;
	extern KEEntityManipulator EntityManipulator;
	extern KEEntitySelector EntitySelector;
	extern KEReflectionManager ReflectionManager;
}