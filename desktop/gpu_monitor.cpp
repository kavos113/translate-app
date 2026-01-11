#include "gpu_monitor.h"

#include <format>
#include <ranges>

gpu_monitor::gpu_monitor()
{
    PDH_STATUS status = PdhOpenQuery(nullptr, 0, &m_query);
    if (status != ERROR_SUCCESS || m_query == nullptr)
    {
        return;
    }

    status = PdhAddCounter(m_query, GPU_MEMORY_QUERY, 0, &m_counter);
    if (status != ERROR_SUCCESS || m_counter == nullptr)
    {
        return;
    }

    QueryAdapters();
}

gpu_monitor::~gpu_monitor()
{
    if (m_query)
    {
        PdhCloseQuery(m_query);
    }
}

std::vector<gpu_monitor::gpu_memory_info> gpu_monitor::QueryMemoryInfo()
{
    if (!m_query || !m_counter)
    {
        return {};
    }

    PDH_STATUS status = PdhCollectQueryData(m_query);
    if (status != ERROR_SUCCESS)
    {
        return {};
    }

    DWORD bufferSize = 0;
    DWORD count = 0;
    PdhGetFormattedCounterArray(m_counter, PDH_FMT_LARGE, &bufferSize, &count, nullptr);

    if (bufferSize == 0)
    {
        return {};
    }

    std::vector<std::byte> buf(bufferSize);
    PPDH_FMT_COUNTERVALUE_ITEM items = reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM_W>(buf.data());
    PdhGetFormattedCounterArray(m_counter, PDH_FMT_LARGE, &bufferSize, &count, items);

    std::vector<gpu_memory_info> result;

    for (DWORD i = 0; i < count; i++)
    {
        gpu_memory_info info;

        bool flag = false;
        for (int j = 0; j < m_adapters.size(); j++)
        {
            if (winrt::to_hstring(items[i].szName) == m_adapters[j].luid)
            {
                info.adapterIndex = j;
                flag = true;
                break;
            }
        }

        if (!flag)
        {
            continue;
        }

        info.value = items[i].FmtValue.largeValue;
        result.push_back(info);
    }

    return result;
}

void gpu_monitor::QueryAdapters()
{
    winrt::com_ptr<IDXGIFactory7> factory;
    HRESULT hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
    if (FAILED(hr))
    {
        return;
    }

    winrt::com_ptr<IDXGIAdapter4> adapter;
    for (
        UINT i = 0;
        factory->EnumAdapterByGpuPreference(
            i,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter)
        ) != DXGI_ERROR_NOT_FOUND;
        i++
    )
    {
        DXGI_ADAPTER_DESC3 desc;
        hr = adapter->GetDesc3(&desc);
        if (FAILED(hr))
        {
            continue;
        }

        if (desc.DedicatedVideoMemory == 0)
        {
            continue;
        }

        gpu_adapter_info adapterInfo;
        adapterInfo.name = winrt::to_hstring(desc.Description);
        adapterInfo.luid = winrt::to_hstring(std::format("luid_0x{:08X}_0x{:08X}_phys_0", desc.AdapterLuid.HighPart, desc.AdapterLuid.LowPart));
        adapterInfo.totalMemory = desc.DedicatedVideoMemory;

        m_adapters.push_back(adapterInfo);
    }
}