#ifndef ENGINE_TRANSLATE_ENGINE_H
#define ENGINE_TRANSLATE_ENGINE_H

#include "llama.h"

#include <string>

class translate_engine
{
public:
    translate_engine() = default;
    ~translate_engine();

    bool load_model();
    void free_model();

    std::string translate_en_to_jp(const std::string& jp);
    std::string translate_jp_to_en(const std::string& en);

private:
    std::string tranlate(const std::string& prompt);

    llama_model *m_model = nullptr;
    llama_sampler *m_sampler = nullptr;

    static constexpr int N_PREDICT = 100;
    static constexpr int N_GPU_LAYERS = 99;

    const std::string MODEL_PATH = "./plamo-2-translate.gguf";
};


#endif //ENGINE_TRANSLATE_ENGINE_H