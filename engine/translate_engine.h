#ifndef ENGINE_TRANSLATE_ENGINE_H
#define ENGINE_TRANSLATE_ENGINE_H

#include "llama.h"

#include <string>

class translate_engine
{
public:
    translate_engine() = default;
    ~translate_engine();

    void load_model();
    void free_model();

    std::string translate_en_to_jp(std::string jp);
    std::string translate_jp_to_en(std::string en);

private:
    llama_model *m_model = nullptr;
    llama_sampler *m_sampler = nullptr;
};


#endif //ENGINE_TRANSLATE_ENGINE_H