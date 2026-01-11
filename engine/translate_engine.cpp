#include "translate_engine.h"

#include <iostream>
#include <vector>
#include <sstream>

translate_engine::~translate_engine()
{
    if (m_model != nullptr)
    {
        free_model();
    }
}

bool translate_engine::load_model()
{
    ggml_backend_load_all();

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = N_GPU_LAYERS;

    m_model = llama_model_load_from_file(MODEL_PATH.c_str(), model_params);
    if (m_model == nullptr)
    {
        std::cerr << "failed to load model" << std::endl;
        return false;
    }
    
    return true;
}

void translate_engine::free_model()
{
    llama_model_free(m_model);
}

void translate_engine::translate_en_to_jp(const std::string& en, const std::function<void(const std::string&)>& token_output_callback)
{
    std::stringstream ss;
    ss << "<|plamo:op|>dataset\ntranslation\n<|plamo:op|>input lang=English\n" << en <<"\n<|plamo:op|>output lang=Japanese";
    std::string prompt = ss.str();
    
    return translate(prompt, token_output_callback);
}

void translate_engine::translate_jp_to_en(const std::string& jp, const std::function<void(const std::string&)>& token_output_callback)
{
    std::stringstream ss;
    ss << "<|plamo:op|>dataset\ntranslation\n<|plamo:op|>input lang=Japanese\n" << jp <<"\n<|plamo:op|>output lang=English";
    std::string prompt = ss.str();
    
    return translate(prompt, token_output_callback);
}

void translate_engine::translate(const std::string& prompt, const std::function<void(const std::string&)>& token_output_callback)
{
    if (m_model == nullptr)
    {
        load_model();
    }

    const llama_vocab *vocab = llama_model_get_vocab(m_model);
    const int n_prompt = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), nullptr, 0, true, true);

    std::vector<llama_token> prompt_tokens(n_prompt);
    if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true) < 0)
    {
        std::cerr << "failed to tokenize prompt" << std::endl;
        return;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = n_prompt + N_PREDICT - 1;
    ctx_params.n_batch = n_prompt;
    ctx_params.no_perf = false;

    llama_context *ctx = llama_init_from_model(m_model, ctx_params);
    if (ctx == nullptr)
    {
        std::cerr << "failed to init model" << std::endl;
        return;
    }

    llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
    sampler_params.no_perf = false;
    llama_sampler *sampler = llama_sampler_chain_init(sampler_params);

    llama_sampler_chain_add(sampler, llama_sampler_init_greedy());

    for (llama_token token : prompt_tokens)
    {
        char buf[128];
        int n = llama_token_to_piece(vocab, token, buf, sizeof(buf), 0, true);
        if (n < 0)
        {
            std::cerr << "failed to convert token to piece" << std::endl;
            return;
        }
        std::string s(buf, n);
    }

    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

    if (llama_model_has_encoder(m_model))
    {
        if (llama_encode(ctx, batch))
        {
            std::cerr << "failed to eval" << std::endl;
            return;
        }

        llama_token decoder_start_token_id = llama_model_decoder_start_token(m_model);
        if (decoder_start_token_id == LLAMA_TOKEN_NULL)
        {
            decoder_start_token_id = llama_vocab_bos(vocab);
        }

        batch = llama_batch_get_one(&decoder_start_token_id, 1);
    }

    const int64_t main_start = ggml_time_us();
    int n_decode = 0;
    llama_token new_token_id;

    for (int n_pos = 0; n_pos + batch.n_tokens < n_prompt + N_PREDICT; )
    {
        if (llama_decode(ctx, batch))
        {
            std::cerr << "failed to decode" << std::endl;
            return;
        }

        n_pos += batch.n_tokens;

        new_token_id = llama_sampler_sample(sampler, ctx, -1);

        if (llama_vocab_is_eog(vocab, new_token_id))
        {
            break;
        }

        char buf[128];
        int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
        if (n < 0)
        {
            std::cerr << "failed to convert token to piece" << std::endl;
            return;
        }

        std::string s(buf, n);
        token_output_callback(s);

        batch = llama_batch_get_one(&new_token_id, 1);

        n_decode += 1;
    }

    std::cerr << std::endl;

    const int64_t main_end = ggml_time_us();

    std::cerr << "decoded " << n_decode << " tokens, " << static_cast<double>(main_end - main_start) / 1000000.0 << "s, " << static_cast<double>(n_decode) / (static_cast<double>(main_end - main_start) / 1000000) << " t/s" << std::endl;

    std::cerr << std::endl;
    llama_perf_sampler_print(sampler);
    llama_perf_context_print(ctx);
    std::cerr << std::endl;

    llama_free(ctx);
}
