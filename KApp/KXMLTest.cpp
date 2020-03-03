#define MEMORY_DUMP_DEBUG
#include "KBase/Interface/IKXML.h"
#include <iostream>
int main()
{
	{
		auto document = GetXMLDocument();
		document->NewDeclaration(R"(xml version="1.0" encoding="utf-8")");
		auto ele = document->NewElement("element");
		ele->SetAttribute("attribute", "val");
		document->SaveFile("test.xml");
	}

	{
		auto document = GetXMLDocument();
		if (document->ParseFromFile("test.xml"))
		{
			auto ele = document->FirstChildElement("element");
			if (ele)
			{
				auto attr = ele->FindAttribute("attribute");
				if (attr)
				{
					printf("%s\n", attr->Value().c_str());
				}
			}
		}
	}
}