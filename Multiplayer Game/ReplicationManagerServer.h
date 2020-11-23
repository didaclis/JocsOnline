#pragma once
#include "ReplicationCommand.h"
#include <map>

class ReplicationManagerServer
{
public:
	void create(uint32 networkId);
	void update(uint32 networkId);
	void destroy(uint32 networkId);

	void write(OutputMemoryStream& packet);

public:
	std::map<uint32, ReplicationAction> commands;
};