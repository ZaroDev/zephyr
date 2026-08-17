#pragma once

#include <nvrhi/nvrhi.h>

namespace Zephyr
{
	// Thin wrapper over nvrhi::ICommandList - the only way engine code above nvrhi
	// records and submits GPU work.
	class CommandList
	{
	public:
		explicit CommandList(nvrhi::DeviceHandle device);

		void Open();
		void Close();

		nvrhi::ICommandList* GetNvrhiCommandList() const { return m_CommandList; }

	private:
		nvrhi::CommandListHandle m_CommandList;
	};
}
