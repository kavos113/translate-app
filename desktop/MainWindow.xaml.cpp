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
        winrt::hstring htext;
        inputbox().Document().GetText(TextGetOptions::None, htext);

        std::string result = m_client->Translate(winrt::to_string(htext), false);
        UpdateTextBlock(result);
    }

    void MainWindow::TranslateToENButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        winrt::hstring htext;
        inputbox().Document().GetText(TextGetOptions::None, htext);

        std::string result = m_client->Translate(winrt::to_string(htext), true);
        UpdateTextBlock(result);
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

    void MainWindow::UpdateTextBlock(const std::string& text)
    {
        ResultBlock().Text(winrt::to_hstring(text));
    }


}
