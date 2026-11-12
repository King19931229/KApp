#include "KEImGuiEditor.h"

int main()
{
	KEImGuiEditor editor;
	if (!editor.Init()) return 1;
	editor.Run();
	editor.UnInit();
	return 0;
}
