#pragma once
#include "ReplicationCommand.h"

class ReplicationManagerClient
{
public:
	void read(const InputMemoryStream& packet);
};