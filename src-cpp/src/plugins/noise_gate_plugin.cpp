#include "noise_gate_plugin.h"
#include "../common.h"

NoiseGatePlugin::NoiseGatePlugin() : AudioEffectPlugin("Noise Gate") {
}

void NoiseGatePlugin::process(float* buffer, int buffer_size) {

    std::vector<float> input_data(buffer, buffer + buffer_size);

    // Calculate RMS
    float sum_squared = 0.0f;
    for (int i = 0; i < buffer_size; ++i) {
        sum_squared += buffer[i] * buffer[i];
    }
    float rms = sqrt(sum_squared / buffer_size);

    float db = 20.0f * log10(rms) + 30;

    int time_now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (db >= threshold_db) {
        if (!active) {
            active = true;
            initial_activation_time = time_now;
        }
        last_activation_time = time_now;
    }
    else if (active && db >= (threshold_db + mumble_threshold)) {
        last_activation_time = time_now;
    }

    if (active) {
        int time_since_start = time_now - initial_activation_time;
        int time_since_last_activation = time_now - last_activation_time;

        if (time_since_last_activation >= release_time) {
            // Deactivate the noise gate and mute the buffer
            active = false;
            initial_activation_time = 0;
            memset(buffer, 0, buffer_size * sizeof(float));
        }
        else if (time_since_start <= attack_time) {
            // Apply fade-in (attack) effect
            float exponent = 2.0f; // Adjust the exponent for the fade-in curve
            float fade = pow(static_cast<float>(time_since_start) / attack_time, exponent);

            for (int i = 0; i < buffer_size; ++i) {
                buffer[i] *= fade;
            }
        }
    }
    else {
        memset(buffer, 0, buffer_size * sizeof(float));
    }
}