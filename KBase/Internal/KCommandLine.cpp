#include "Publish/KCommandLine.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <sstream>

const KCommandLine::Option* KCommandLine::Find(const char* name) const
{
	if (name)
	{
		for (const Option& option : m_Options)
		{
			if (option.name == name) return &option;
		}
	}
	return nullptr;
}

bool KCommandLine::Add(const char* name, OptionType type, const std::string& defaultValue,
	bool requiresValue, const char* description)
{
	if (!name || !*name || *name == '-' || std::string(name).find_first_of("= \t\r\n") != std::string::npos)
	{
		m_Error = "Invalid option name";
		return false;
	}
	if (Find(name))
	{
		m_Error = std::string("Duplicate option --") + name;
		return false;
	}
	Option option;
	option.name = name;
	option.type = type;
	option.defaultValue = option.value = defaultValue;
	option.requiresValue = requiresValue;
	option.description = description ? description : "";
	m_Options.push_back(std::move(option));
	return true;
}

bool KCommandLine::AddBool(const char* name, bool defaultValue, bool requiresValue, const char* description)
{
	return Add(name, OT_BOOL, defaultValue ? "true" : "false", requiresValue, description);
}

bool KCommandLine::AddInt(const char* name, int defaultValue, const char* description)
{
	return Add(name, OT_INT, std::to_string(defaultValue), true, description);
}

bool KCommandLine::AddString(const char* name, const char* defaultValue, const char* description)
{
	return Add(name, OT_STRING, defaultValue ? defaultValue : "", true, description);
}

bool KCommandLine::SetValue(Option& option, const std::string& value)
{
	if (option.type == OT_BOOL)
	{
		std::string lower = value;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		if (lower == "true" || lower == "1") option.value = "true";
		else if (lower == "false" || lower == "0") option.value = "false";
		else
		{
			m_Error = "Invalid boolean for --" + option.name + ": " + value;
			return false;
		}
	}
	else if (option.type == OT_INT)
	{
		const size_t begin = !value.empty() && (value[0] == '+' || value[0] == '-') ? 1 : 0;
		bool digits = begin < value.size() && std::all_of(value.begin() + begin, value.end(), [](unsigned char c) { return c >= '0' && c <= '9'; });
		errno = 0;
		char* end = nullptr;
		const long number = std::strtol(value.c_str(), &end, 10);
		if (!digits || errno == ERANGE || *end || number < (std::numeric_limits<int>::min)() || number > (std::numeric_limits<int>::max)())
		{
			m_Error = "Invalid integer for --" + option.name + ": " + value;
			return false;
		}
		option.value = value;
	}
	else option.value = value;
	option.provided = true;
	return true;
}

bool KCommandLine::Parse(int argc, const char* const* argv)
{
	m_Error.clear();
	m_Program = argc > 0 && argv && argv[0] ? argv[0] : "program";
	for (Option& option : m_Options)
	{
		option.value = option.defaultValue;
		option.provided = false;
	}
	if (argc < 0 || (argc > 0 && !argv))
	{
		m_Error = "Invalid argc/argv";
		return false;
	}
	for (int index = 1; index < argc; ++index)
	{
		const std::string argument = argv[index] ? argv[index] : "";
		if (argument.compare(0, 2, "--") != 0)
		{
			m_Error = "Unexpected argument: " + argument;
			return false;
		}
		const size_t equal = argument.find('=');
		const std::string name = argument.substr(2, equal == std::string::npos ? equal : equal - 2);
		auto it = std::find_if(m_Options.begin(), m_Options.end(), [&](const Option& option) { return option.name == name; });
		if (it == m_Options.end())
		{
			m_Error = "Unknown option --" + name;
			return false;
		}
		std::string value;
		if (equal != std::string::npos) value = argument.substr(equal + 1);
		else if (!it->requiresValue) value = "true";
		else
		{
			if (index + 1 >= argc || !argv[index + 1] || std::string(argv[index + 1]).compare(0, 2, "--") == 0)
			{
				m_Error = "Missing value for --" + name;
				return false;
			}
			value = argv[++index];
		}
		if (!SetValue(*it, value)) return false;
	}
	return true;
}

bool KCommandLine::Has(const char* name) const
{
	const Option* option = Find(name);
	return option && option->provided;
}

bool KCommandLine::GetBool(const char* name) const
{
	const Option* option = Find(name);
	assert(option && option->type == OT_BOOL);
	return option && option->type == OT_BOOL && option->value == "true";
}

int KCommandLine::GetInt(const char* name) const
{
	const Option* option = Find(name);
	assert(option && option->type == OT_INT);
	return option && option->type == OT_INT ? (int)std::strtol(option->value.c_str(), nullptr, 10) : 0;
}

const std::string& KCommandLine::GetString(const char* name) const
{
	const Option* option = Find(name);
	assert(option && option->type == OT_STRING);
	static const std::string empty;
	return option && option->type == OT_STRING ? option->value : empty;
}

std::string KCommandLine::GetHelp() const
{
	std::ostringstream help;
	help << "Usage: " << m_Program << " [options]\n";
	for (const Option& option : m_Options)
	{
		const char* type = option.type == OT_BOOL ? "bool" : option.type == OT_INT ? "int" : "string";
		help << "  --" << option.name;
		if (option.requiresValue) help << " <" << type << ">";
		else help << "[=<bool>]";
		help << " (default: " << (option.type == OT_STRING ? "\"" + option.defaultValue + "\"" : option.defaultValue) << ")\n    " << option.description << '\n';
	}
	return help.str();
}
