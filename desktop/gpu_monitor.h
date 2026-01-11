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
		winrt::hstring deviceName;
		uint64_t value;
	};

	std::vector<gpu_memory_info> QueryMemoryInfo();

private:
	HQUERY m_query = nullptr;
	HCOUNTER m_counter = nullptr;

	struct GPUAdapter
	{
		winrt::hstring luid;
		winrt::hstring name;
	};

	std::vector<GPUAdapter> m_adapters;

	const wchar_t* GPU_MEMORY_QUERY = L"\\GPU Adapter Memory(*)\\Dedicated Usage";
};

