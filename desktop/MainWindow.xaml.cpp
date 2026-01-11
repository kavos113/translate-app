#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <grpcpp/grpcpp.h>
#include <thread>

#include "client.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Text;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

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

    void MainWindow::OnGPUTimerTick(IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e)
    {
        auto infos = m_monitor.QueryMemoryInfo();
        
        gpuInfo().Text(L"");
        for (const auto& info : infos)
        {
            Run run;
            run.Text(info.deviceName + L": " + winrt::to_hstring(info.value) + L"\n");
            gpuInfo().Inlines().Append(run);
        }
    }

    void MainWindow::UpdateTextBlock(const std::string& text)
    {
        ResultBlock().Text(winrt::to_hstring(text));
    }

    void MainWindow::AppendTextBlock(const std::string& text)
    {
        Run run;
        run.Text(winrt::to_hstring(text));

        ResultBlock().Inlines().Append(run);
    }
}
