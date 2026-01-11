#pragma once

#include <grpcpp/grpcpp.h>

#include <translate.pb.h>
#include <translate.grpc.pb.h>

#include <memory>
#include <functional>

using translate::TranslateService;
using translate::TranslateRequest;
using translate::TranslateResponse;
using translate::LogResponse;
using google::protobuf::Empty;

class client
{
public:
	client(const std::shared_ptr<grpc::Channel>& channel)
		: m_stub(TranslateService::NewStub(channel))
	{

	}

	bool LoadModel()
	{
		Empty request, response;
		grpc::ClientContext ctx;

		auto status = m_stub->LoadModel(&ctx, request, &response);

		return status.ok();
	}

	bool FreeModel()
	{
		Empty request, response;
		grpc::ClientContext ctx;

		auto status = m_stub->FreeModel(&ctx, request, &response);

		return status.ok();
	}

	void Translate(const std::string& content, bool is_jp_to_en, std::function<void(const std::string&, bool)> callback)
	{
		TranslateRequest request;
		request.set_content(content);
		request.set_is_jp_to_en(is_jp_to_en);

		TranslateResponse response;
		grpc::ClientContext ctx;

		std::unique_ptr reader(m_stub->Translate(&ctx, request));
		while (reader->Read(&response))
		{
			callback(response.content(), false);
		}

		auto status = reader->Finish();

		if (!status.ok())
		{
			std::stringstream ss;
			ss << "翻訳中にエラーが発生しました: " << status.error_code() << ", " << status.error_message();
			callback(ss.str(), true);
		}
	}

	void WatchLog(std::function<void(const std::string&)> callback)
	{
		Empty req;
		LogResponse response;
		grpc::ClientContext ctx;

		std::unique_ptr reader(m_stub->WatchLog(&ctx, req));
		while (reader->Read(&response))
		{
			callback(response.log_content());
		}

		auto status = reader->Finish();

		if (!status.ok())
		{
			std::stringstream ss;
			ss << "GPUのログ送信中にエラーが発生しました: " << status.error_code() << ", " << status.error_message();
			callback(ss.str());
		}
	}

private:
	std::unique_ptr<TranslateService::Stub> m_stub;
};

