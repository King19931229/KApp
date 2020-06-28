#include "KEditorGlobal.h"

namespace KEditorGlobal
{
	QMainWindow* MainWindow = nullptr;

	KECommandInvoker CommandInvoker;
	KEResourcePorter ResourcePorter;
	KEEntityManipulator EntityManipulator;
	KEEntitySelector EntitySelector;
	KEEntityNamePool EntityNamePool;
	KEReflectionManager ReflectionManager;
}