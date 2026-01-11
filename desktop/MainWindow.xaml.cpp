#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <grpcpp/grpcpp.h>
#include <thread>
#include <format>

#include "client.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Text;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace
{
    winrt::hstring FormatMemorySize(uint64_t value)
    {
        return winrt::to_hstring(std::format("{:.3f} GB", static_cast<double>(value) / (1024.0 * 1024.0 * 1024.0)));
    }
}

namespace winrt::desktop::implementation
{
    MainWindow::MainWindow()
    {
        const std::string server_address = "localhost:50551";

        m_client = std::make_unique<client>(
            grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials())
        );

        m_gpuInfoTimer = DispatcherTimer();
        m_gpuInfoTimer.Interval(std::chrono::milliseconds(1000));
        m_gpuInfoTimer.Tick({ this, &MainWindow::OnGPUTimerTick });
        m_gpuInfoTimer.Start();

        InitializeComponent();

        InitGPUInfoTable();
        InitGPULogs();

        Microsoft::UI::Windowing::AppWindow appWindow = AppWindow();
        appWindow.Resize(Windows::Graphics::SizeInt32{1200, 1000});
    }

    void MainWindow::InitGPUInfoTable()
    {
        m_adapters = m_monitor.GetAdapterInfo();

        gpuInfoTable().Children().Clear();
        gpuInfoTable().RowDefinitions().Clear();
        gpuInfoTable().ColumnDefinitions().Clear();

        ColumnDefinition col1;
        col1.Width(GridLengthHelper::Auto());
        ColumnDefinition col2;
        col2.Width(GridLengthHelper::FromPixels(150));
        ColumnDefinition col3;
        col3.Width(GridLengthHelper::FromPixels(150));

        gpuInfoTable().ColumnDefinitions().Append(col1);
        gpuInfoTable().ColumnDefinitions().Append(col2);
        gpuInfoTable().ColumnDefinitions().Append(col3);

        for (int i = 0; i < m_adapters.size(); i++)
        {
            RowDefinition row;
            row.Height(GridLengthHelper::Auto());
            gpuInfoTable().RowDefinitions().Append(row);

            TextBlock gpuName;
            gpuName.Text(m_adapters[i].name);
            gpuName.Margin({ 5, 5 });
            Grid::SetRow(gpuName, i);
            Grid::SetColumn(gpuName, 0);
            gpuInfoTable().Children().Append(gpuName);

            TextBlock memory;
            memory.Text(L"0.000 GB");
            memory.Margin({ 5, 5 });
            memory.TextAlignment(TextAlignment::Right);
            m_memoryTexts.push_back(memory);
            Grid::SetRow(memory, i);
            Grid::SetColumn(memory, 1);
            gpuInfoTable().Children().Append(memory);

            TextBlock totalMemory;
            totalMemory.Text(L"/ " + FormatMemorySize(m_adapters[i].totalMemory));
            totalMemory.Margin({ 5, 5 });
            totalMemory.TextAlignment(TextAlignment::Left);
            Grid::SetRow(totalMemory, i);
            Grid::SetColumn(totalMemory, 2);
            gpuInfoTable().Children().Append(totalMemory);
        }
    }

    void MainWindow::InitGPULogs()
    {
        m_logTimer = DispatcherTimer();
        m_logTimer.Interval(std::chrono::milliseconds(100));
        m_logTimer.Tick({ this, &MainWindow::OnLogTimerTick });
        m_logTimer.Start();

        std::thread([this]()
            {
                m_client->WatchLog([this](const std::string& log)
                    {
                        m_logBuffer.push_back(log);
                    });
            }).detach();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainWindow::LoadModelButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        apiProgress().IsActive(true);

        std::thread([this]()
            {
                bool status = m_client->LoadModel();

                this->DispatcherQueue().TryEnqueue([this, status]()
                    {
                        if (!status)
                        {
                            UpdateTextBlock("モデルのロード中にエラーが発生しました");
                        }

                        apiProgress().IsActive(false);
                        translateToENButton().IsEnabled(true);
                        translateToJPButton().IsEnabled(true);
                        destroyButton().IsEnabled(true);
                        loadButton().IsEnabled(false);
                    }
                );
            }).detach();
    }

    void MainWindow::TranslateToJPButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        UpdateTextBlock("");

        winrt::hstring htext;
        inputbox().Document().GetText(TextGetOptions::None, htext);

        apiProgress().IsActive(true);

        std::thread([this, htext]()
            {
                m_client->Translate(winrt::to_string(htext), false, [this](const std::string& text, bool isOverwrite)
                    {
                        this->DispatcherQueue().TryEnqueue([this, text, isOverwrite]()
                            {
                                if (isOverwrite)
                                {
                                    UpdateTextBlock(text);
                                }
                                else
                                {
                                    AppendTextBlock(text);
                                }
                                apiProgress().IsActive(false);
                            });
                    });
            }).detach();
    }

    void MainWindow::TranslateToENButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        UpdateTextBlock("");

        winrt::hstring htext;
        inputbox().Document().GetText(TextGetOptions::None, htext);

        apiProgress().IsActive(true);

        std::thread([this, htext]()
            {
                m_client->Translate(winrt::to_string(htext), true, [this](const std::string& text, bool isOverwrite)
                    {
                        this->DispatcherQueue().TryEnqueue([this, text, isOverwrite]()
                            {
                                if (isOverwrite)
                                {
                                    UpdateTextBlock(text);
                                }
                                else
                                {
                                    AppendTextBlock(text);
                                }
                                apiProgress().IsActive(false);
                            });
                    });
            }).detach();
    }

    void MainWindow::DestroyModelButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        apiProgress().IsActive(true);

        std::thread([this]()
            {
                bool status = m_client->FreeModel();

                this->DispatcherQueue().TryEnqueue([this, status]()
                    {
                        if (!status)
                        {
                            UpdateTextBlock("モデルの破棄中にエラーが発生しました");
                        }

                        apiProgress().IsActive(false);
                        translateToENButton().IsEnabled(false);
                        translateToJPButton().IsEnabled(false);
                        destroyButton().IsEnabled(false);
                        loadButton().IsEnabled(true);
                    }
                );
            }).detach();
    }

    void MainWindow::Window_Closed(IInspectable const& sender, WindowEventArgs const& args)
    {
        m_client->FreeModel();
    }

    void MainWindow::OnGPUTimerTick(IInspectable const& sender, IInspectable const& e)
    {
        auto infos = m_monitor.QueryMemoryInfo();
        
        for (const auto& info : infos)
        {
            m_memoryTexts[info.adapterIndex].Text(FormatMemorySize(info.value));
        }
    }

    void MainWindow::OnLogTimerTick(IInspectable const& sender, IInspectable const& e)
    {
        std::vector<std::string> logs;
        
        {
            std::lock_guard<std::mutex> lock(m_logMutex);
            if (m_logBuffer.empty())
            {
                return;
            }

            logs = std::move(m_logBuffer);
        }

        std::stringstream ss;
        for (const auto& log : logs)
        {
            ss << log;
        }

        if (!logs.empty())
        {
            Run run;
            run.Text(winrt::to_hstring(ss.str()));

            gpuLogBlock().Inlines().Append(run);
            gpuLogScroll().UpdateLayout();

            gpuLogScroll().ChangeView(nullptr, gpuLogScroll().ScrollableHeight(), nullptr);
        }
    }

    void MainWindow::UpdateTextBlock(const std::string& text)
    {
        ResultBlock().Text(winrt::to_hstring(text));
        ResultScroll().UpdateLayout();
        ResultScroll().ChangeView(nullptr, ResultScroll().ScrollableHeight(), nullptr);
    }

    void MainWindow::AppendTextBlock(const std::string& text)
    {
        Run run;
        run.Text(winrt::to_hstring(text));

        ResultBlock().Inlines().Append(run);
        ResultScroll().UpdateLayout();
        ResultScroll().ChangeView(nullptr, ResultScroll().ScrollableHeight(), nullptr);
    }
}
