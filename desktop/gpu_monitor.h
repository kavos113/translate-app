#pragma once

#include "pch.h"

#include <Pdh.h>
#include <vector>

class gpu_monitor
{
public:
	gpu_monitor();
	~gpu_monitor();

	struct gpu_memory_info
	{
		size_t adapterIndex;
		uint64_t value;
	};

	struct gpu_adapter_info
	{
		winrt::hstring luid;
		winrt::hstring name;
		uint64_t totalMemory;
	};

	std::vector<gpu_memory_info> QueryMemoryInfo();
	std::vector<gpu_adapter_info> GetAdapterInfo()
	{
		return m_adapters;
	}

private:
	void QueryAdapters();

	HQUERY m_query = nullptr;
	HCOUNTER m_counter = nullptr;

	std::vector<gpu_adapter_info> m_adapters;

	const wchar_t* GPU_MEMORY_QUERY = L"\\GPU Adapter Memory(*)\\Dedicated Usage";
};

