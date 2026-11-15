#pragma once
#include "KConfig.h"
#include <vector>

class EXPORT_DLL KCommandLine
{
protected:
	enum OptionType { OT_BOOL, OT_INT, OT_STRING };
	struct Option
	{
		std::string name, defaultValue, value, description;
		OptionType type;
		bool requiresValue;
		bool provided = false;
	};
	std::vector<Option> m_Options;
	std::string m_Error;
	std::string m_Program = "program";

	bool Add(const char* name, OptionType type, const std::string& defaultValue,
		bool requiresValue, const char* description);
	bool SetValue(Option& option, const std::string& value);
	const Option* Find(const char* name) const;

public:
	bool AddBool(const char* name, bool defaultValue, bool requiresValue, const char* description);
	bool AddInt(const char* name, int defaultValue, const char* description);
	bool AddString(const char* name, const char* defaultValue, const char* description);
	bool Parse(int argc, const char* const* argv);
	bool Has(const char* name) const;
	bool GetBool(const char* name) const;
	int GetInt(const char* name) const;
	const std::string& GetString(const char* name) const;
	const std::string& GetError() const { return m_Error; }
	std::string GetHelp() const;
};
