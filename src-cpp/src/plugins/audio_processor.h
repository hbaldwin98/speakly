#ifndef SPEAKLY_AUDIO_PROCESSOR_H
#define SPEAKLY_AUDIO_PROCESSOR_H
#pragma once

#include <vector>
#include <memory>
#include "audio_effect_plugin.h"

class AudioProcessor {
private:
    std::vector<std::shared_ptr<AudioEffectPlugin>> plugins;

public:
    void add_plugin(std::shared_ptr<AudioEffectPlugin> plugin);
    void process_audio(float* buffer, int buffer_size);
};

#endif //SPEAKLY_AUDIO_PROCESSOR_H
