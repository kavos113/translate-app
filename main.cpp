#include <iostream>
#include <string>
#include <vector>

#include "llama.h"

constexpr int n_predict = 32;
constexpr int n_gpu_layers = 99;

const std::string model_path = "embeddinggemma-300M-Q8_0.gguf";
const std::string prompt = "What is your name?";

int main()
{
    ggml_backend_load_all();

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = n_gpu_layers;

    llama_model *model = llama_model_load_from_file(model_path.c_str(), model_params);
    if (model == nullptr)
    {
        std::cerr << "failed to load model" << std::endl;
        return 1;
    }

    const llama_vocab *vocab = llama_model_get_vocab(model);
    const int n_prompt = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), nullptr, 0, true, true);

    std::vector<llama_token> prompt_tokens(n_prompt);
    if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true) < 0)
    {
        std::cerr << "failed to tokenize prompt" << std::endl;
        return 1;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = n_prompt + n_predict - 1;
    ctx_params.n_batch = n_prompt;
    ctx_params.no_perf = false;

    llama_context *ctx = llama_init_from_model(model, ctx_params);
    if (ctx == nullptr)
    {
        std::cerr << "failed to init model" << std::endl;
        return 1;
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
            return 1;
        }
        std::string s(buf, n);
        std::cout << s;
    }

    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

    if (llama_model_has_encoder(model))
    {
        if (llama_encode(ctx, batch))
        {
            std::cerr << "failed to eval" << std::endl;
            return 1;
        }

        llama_token decoder_start_token_id = llama_model_decoder_start_token(model);
        if (decoder_start_token_id == LLAMA_TOKEN_NULL)
        {
            decoder_start_token_id = llama_vocab_bos(vocab);
        }

        batch = llama_batch_get_one(&decoder_start_token_id, 1);
    }

    const int64_t main_start = ggml_time_us();
    int n_decode = 0;
    llama_token new_token_id;

    for (int n_pos = 0; n_pos + batch.n_tokens < n_prompt + n_predict; )
    {
        if (llama_decode(ctx, batch))
        {
            std::cerr << "failed to decode" << std::endl;
            return 1;
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
            return 1;
        }

        std::string s(buf, n);
        std::cout << s;
        fflush(stdout);

        batch = llama_batch_get_one(&new_token_id, 1);

        n_decode += 1;
    }

    std::cout << std::endl;

    const int64_t main_end = ggml_time_us();

    std::cout << "decoded " << n_decode << " tokens, " << static_cast<float>(main_start - main_end) / 1000000.0f << "s, " << static_cast<float>(n_decode) / (static_cast<float>(main_start - main_end)) << " t/s" << std::endl;

    std::cout << std::endl;
    llama_perf_sampler_print(sampler);
    llama_perf_context_print(ctx);
    std::cout << std::endl;

    llama_sampler_free(sampler);
    llama_free(ctx);
    llama_model_free(model);

    return 0;
}