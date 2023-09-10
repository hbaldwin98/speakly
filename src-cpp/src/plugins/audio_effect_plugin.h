#ifndef SPEAKLY_AUDIO_EFFECT_PLUGIN_H
#define SPEAKLY_AUDIO_EFFECT_PLUGIN_H
#pragma once

#include <opus/opus_types.h>

class AudioEffectPlugin {
protected:
    const char* name;
public:
    AudioEffectPlugin() : name("Audio Effect Plugin") {}
    AudioEffectPlugin(const char* name) : name(name) {}
    virtual void process(float* buffer, int buffer_size) = 0;
};

#endif //SPEAKLY_AUDIO_EFFECT_PLUGIN_H
