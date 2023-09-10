#include "high_pass_plugin.h"

HighPassPlugin::HighPassPlugin(float timeConstant) : timeConstant(timeConstant), prevSample(0.0) {}

void HighPassPlugin::process(float* buffer, int buffer_size) {
    for (int i = 0; i < buffer_size; ++i) {
        // Calculate the high-pass filtered value
        double inputSample = static_cast<double>(buffer[i]);
        double filteredValue = inputSample - prevSample + timeConstant * prevSample;

        // Update the previous sample and buffer with the filtered value
        prevSample = filteredValue;
        buffer[i] = filteredValue;
    }
}

void HighPassPlugin::set_time_constant(float timeConstant) {
    // Ensure the time constant is within a valid range (0 < timeConstant < 1)
    if (timeConstant > 1.0) {
        timeConstant = 1.0;
    }
    else if (timeConstant < 0.0) {
        timeConstant = 0.0;
    }

    this->timeConstant = timeConstant;
}