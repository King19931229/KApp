#pragma once
#include "KEGraphConfig.h"

struct KEGraphPort
{
	PortType type;
	PortIndexType index;

	KEGraphPort()
		: type(PT_NONE),
		index(INVALID_PORT_INDEX)
	{}
	KEGraphPort(PortType t, PortIndexType i)
		: type(t),
		index(i)
	{}
	inline bool IndexIsValid() { return index != INVALID_PORT_INDEX; }
	inline bool PortTypeIsValid() { return type != PT_NONE; }
};

inline PortType OppositePort(PortType port)
{
	PortType result = PT_NONE;

	switch (port)
	{
	case PT_IN:
		result = PT_OUT;
		break;
	case PT_OUT:
		result = PT_IN;
		break;
	default:
		break;
	}

	return result;
}