#ifndef SPEAKLY_HIGH_PASS_PLUGIN_H
#define SPEAKLY_HIGH_PASS_PLUGIN_H
#pragma once

#include "audio_effect_plugin.h"

class HighPassPlugin : public AudioEffectPlugin {
private:
    double timeConstant;
    double prevSample;

public:
    HighPassPlugin(float timeConstant);

    void process(float* buffer, int buffer_size) override;

    void set_time_constant(float timeConstant);
};

#endif //SPEAKLY_HIGH_PASS_PLUGIN_H
