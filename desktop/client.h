#pragma once

#include <grpcpp/grpcpp.h>

#include <translate.pb.h>
#include <translate.grpc.pb.h>

#include <memory>

using translate::TranslateService;
using translate::TranslateRequest;
using translate::TranslateResponse;
using google::protobuf::Empty;

class client
{
public:
	client(const std::shared_ptr<grpc::Channel>& channel)
		: m_stub(TranslateService::NewStub(channel))
	{

	}

	void LoadModel()
	{
		Empty request, response;
		grpc::ClientContext ctx;

		auto status = m_stub->LoadModel(&ctx, request, &response);
	}

	std::string Translate(const std::string& content, bool is_jp_to_en)
	{
		TranslateRequest request;
		request.set_content(content);
		request.set_is_jp_to_en(is_jp_to_en);

		TranslateResponse response;
		grpc::ClientContext ctx;

		auto status = m_stub->Translate(&ctx, request, &response);

		if (status.ok())
		{
			return response.content();
		}
		else
		{
			std::stringstream ss;
			ss << "翻訳中にエラーが発生しました: " << status.error_code() << ", " << status.error_message();
			return ss.str();
		}
	}

private:
	std::unique_ptr<TranslateService::Stub> m_stub;
};

