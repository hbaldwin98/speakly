#ifndef SPEAKLY_NOISE_GATE_PLUGIN_H
#define SPEAKLY_NOISE_GATE_PLUGIN_H
#pragma once

#include "audio_effect_plugin.h"

class NoiseGatePlugin : public AudioEffectPlugin {
private:
    bool active = false;

    double threshold_db = -8.0;
    double mumble_threshold = -3.0;

    int attack_time = 10;
    int release_time = 800;
    int last_activation_time = 0;
    int initial_activation_time = 0;
public:
    NoiseGatePlugin();

    void process(float* buffer, int buffer_size) override;
};
#endif //SPEAKLY_NOISE_GATE_PLUGIN_H
