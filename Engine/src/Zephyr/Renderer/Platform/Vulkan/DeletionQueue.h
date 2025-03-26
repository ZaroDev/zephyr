#pragma once
#include <deque>

struct DeletionQueue
{
	std::deque<std::function<void()>> Deletors;

	void PushFunction(std::function<void()>&&function)
	{
		Deletors.push_back(function);
	}

	void Flush()
	{
		for(auto it = Deletors.rbegin(); it != Deletors.rend(); ++it)
		{
			(*it)();
		}

		Deletors.clear();
	}
};
