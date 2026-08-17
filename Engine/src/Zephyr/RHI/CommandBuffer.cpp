#include <pch.h>
#include "CommandBuffer.h"

namespace Zephyr
{
	CommandList::CommandList(nvrhi::DeviceHandle device)
	{
		CORE_ASSERT(device, "Trying to create a CommandList from a null device");
		m_CommandList = device->createCommandList();
	}

	void CommandList::Open()
	{
		m_CommandList->open();
	}

	void CommandList::Close()
	{
		m_CommandList->close();
	}
}
