#include "audio_processor.h"

void AudioProcessor::add_plugin(std::shared_ptr<AudioEffectPlugin> plugin) {
    plugins.push_back(plugin);
}

void AudioProcessor::process_audio(float* buffer, int buffer_size) {
    for (auto plugin : plugins) {
        plugin->process(buffer, buffer_size);
    }

}