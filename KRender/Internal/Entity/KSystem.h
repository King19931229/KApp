#include "KECS.h"

struct KSystemBase
{
public:
	virtual ~KSystemBase() {}
	virtual void Update(float dt, KComponentBasePtr components) = 0;
};