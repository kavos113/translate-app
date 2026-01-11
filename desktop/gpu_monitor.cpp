#include "gpu_monitor.h"

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
        return;
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
        return;
    }

    std::vector<std::byte> buf(bufferSize);
    PPDH_FMT_COUNTERVALUE_ITEM items = reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM_W>(buf.data());
    PdhGetFormattedCounterArray(m_counter, PDH_FMT_LARGE, &bufferSize, &count, items);

    std::vector<gpu_memory_info> result(count);

    for (DWORD i = 0; i < count; i++)
    {
        result[i].deviceName = items[i].szName;
        result[i].value = items[i].FmtValue.largeValue;
    }

    return result;
}
