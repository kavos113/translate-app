#ifndef ENGINE_TRANSLATE_ENGINE_H
#define ENGINE_TRANSLATE_ENGINE_H

#include "llama.h"

#include <string>
#include <functional>

class translate_engine
{
public:
    translate_engine() = default;
    ~translate_engine();

    bool load_model();
    void free_model();

    void translate_en_to_jp(const std::string& en, const std::function<void(const std::string&)>& token_output_callback);
    void translate_jp_to_en(const std::string& jp, const std::function<void(const std::string&)>& token_output_callback);

private:
    void translate(const std::string& prompt, const std::function<void(const std::string&)>& token_output_callback);

    llama_model *m_model = nullptr;

    static constexpr int N_PREDICT = 100000;
    static constexpr int N_GPU_LAYERS = 99;

    const std::string MODEL_PATH = "./plamo-2-translate.gguf";
};


#endif //ENGINE_TRANSLATE_ENGINE_H