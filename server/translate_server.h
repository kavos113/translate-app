#ifndef SERVER_TRANSLATE_SERVER_H
#define SERVER_TRANSLATE_SERVER_H

#include <grpcpp/grpcpp.h>

#include "translate.grpc.pb.h"
#include "translate.pb.h"

#include "translate_engine.h"

using translate::TranslateService;
using translate::TranslateRequest;
using translate::TranslateResponse;
using google::protobuf::Empty;

class translate_server final : public TranslateService::Service
{
public:
    grpc::Status LoadModel(
        grpc::ServerContext* context,
        const Empty* request,
        Empty* response
    ) override
    {
        bool res = m_engine.load_model();
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
        TranslateResponse* response
    ) override
    {
        std::string content = request->content();
        if (request->is_jp_to_en())
        {
            std::string result = m_engine.translate_jp_to_en(content);
            response->set_content(result);
            return grpc::Status::OK;
        }
        else
        {
            std::string result = m_engine.translate_en_to_jp(content);
            response->set_content(result);
            return grpc::Status::OK;
        }
    }

private:
    translate_engine m_engine;
};


#endif //SERVER_TRANSLATE_SERVER_H