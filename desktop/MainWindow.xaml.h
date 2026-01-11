#pragma once

#include "MainWindow.g.h"
#include "client.h"
#include "gpu_monitor.h"

#include <memory>
#include <vector>

namespace winrt::desktop::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
    public:
        MainWindow();

        int32_t MyProperty();
        void MyProperty(int32_t value);
        void LoadModelButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void TranslateToJPButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void TranslateToENButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void DestroyModelButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void Window_Closed(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowEventArgs const& args);

    private:
        void UpdateTextBlock(const std::string& text);
        void AppendTextBlock(const std::string& text);
        void OnGPUTimerTick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e);

        void InitGPUInfoTable();

        std::unique_ptr<client> m_client;
        gpu_monitor m_monitor;
        std::vector<gpu_monitor::gpu_adapter_info> m_adapters;
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_gpuInfoTimer{ nullptr };

        std::vector<winrt::Microsoft::UI::Xaml::Controls::TextBlock> m_memoryTexts;
    };
}

namespace winrt::desktop::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
