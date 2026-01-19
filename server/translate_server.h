#ifndef SERVER_TRANSLATE_SERVER_H
#define SERVER_TRANSLATE_SERVER_H

#include <grpcpp/grpcpp.h>

#include <translate.grpc.pb.h>
#include <translate.pb.h>

#include "translate_engine.h"
#include "stderr_capturer.h"
#include "log_queue.h"

#include <functional>
#include <filesystem>

using translate::TranslateService;
using translate::TranslateRequest;
using translate::TranslateResponse;
using translate::LogResponse;
using google::protobuf::Empty;

class translate_server final : public TranslateService::Service
{
public:
    grpc::Status LoadModel(
        grpc::ServerContext* context,
        const translate::LoadModelRequest* request,
        Empty* response
    ) override
    {
        bool res = m_engine.load_model(request->model_name());
        if (res)
        {
            return grpc::Status::OK;
        }
        else
        {
            return grpc::Status::CANCELLED;
        }
    }
    
    grpc::Status FreeModel(
        grpc::ServerContext* context,
        const Empty* request,
        Empty* response
    ) override
    {
        m_engine.free_model();
        return grpc::Status::OK;
    }
    
    grpc::Status Translate(
        grpc::ServerContext* context,
        const TranslateRequest* request,
        grpc::ServerWriter<TranslateResponse> *writer
    ) override
    {
        std::string content = request->content();

        if (context->IsCancelled())
        {
            return grpc::Status::CANCELLED;
        }

        std::function callback = [writer](const std::string& token)
        {
            TranslateResponse response;
            response.set_content(token);

            writer->Write(response);
        };

        if (request->is_jp_to_en())
        {
            m_engine.translate_jp_to_en(content, callback);
        }
        else
        {
            m_engine.translate_en_to_jp(content, callback);
        }

        return grpc::Status::OK;
    }

    grpc::Status WatchLog(
        grpc::ServerContext* context,
        const Empty* request,
        grpc::ServerWriter<LogResponse>* writer
    ) override
    {
        if (context->IsCancelled())
        {
            m_capturer.stop();

            return grpc::Status::CANCELLED;
        }

        std::function callback = [this](const std::string& content)
        {
            m_logQueue.push(content);
        };

        m_capturer.set_callback(callback);
        m_capturer.start();

        while (true)
        {
            if (context->IsCancelled())
            {
                m_capturer.stop();

                break;
            }

            std::string logContent;
            if (m_logQueue.pop_with_timeout(logContent, 1000))
            {
                LogResponse response;
                response.set_log_content(logContent);

                writer->Write(response);
            }
        }

        return grpc::Status::OK;
    }

    grpc::Status ListModel(
        grpc::ServerContext* context,
        const Empty* request,
        translate::ListModelResponse* response
    ) override
    {
        std::filesystem::directory_iterator it{"."};
        for (const std::filesystem::directory_entry& entry : it)
        {
            if (entry.is_regular_file() && entry.path().string().ends_with(".gguf"))
            {
                std::string *target = response->add_model_name();
                *target = entry.path().string();
            }
        }

        return grpc::Status::OK;
    }

private:
    translate_engine m_engine;
    stderr_capturer m_capturer;
    log_queue m_logQueue;
};

#endif //SERVER_TRANSLATE_SERVER_H