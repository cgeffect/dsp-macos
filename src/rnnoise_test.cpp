#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <iomanip>

// RNNoise头文件
extern "C" {
    #include "rnnoise.h"
}

// 读取PCM文件（int16格式）
std::vector<short> read_pcm_file_int16(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "❌ 错误：无法打开文件 " << filename << std::endl;
        return {};
    }
    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    size_t num_samples = file_size / sizeof(short);
    std::vector<short> audio_data(num_samples);
    file.read(reinterpret_cast<char*>(audio_data.data()), file_size);
    file.close();
    std::cout << "✅ 成功读取PCM文件: " << filename << std::endl;
    std::cout << "   文件大小: " << file_size << " 字节" << std::endl;
    std::cout << "   样本数量: " << num_samples << std::endl;
    std::cout << "   时长: " << std::fixed << std::setprecision(2)
              << (static_cast<double>(num_samples) / 48000.0) << " 秒" << std::endl;
    return audio_data;
}

// 保存PCM文件（int16格式）
bool save_pcm_file_int16(const std::vector<short>& audio_data, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "错误：无法创建文件 " << filename << std::endl;
        return false;
    }
    file.write(reinterpret_cast<const char*>(audio_data.data()), audio_data.size() * sizeof(short));
    file.close();
    std::cout << "✅ PCM文件已保存: " << filename << std::endl;
    std::cout << "   文件大小: " << (audio_data.size() * sizeof(short)) << " 字节" << std::endl;
    return true;
}

// 计算音频的RMS值
double calculate_rms_int16(const std::vector<short>& audio_data) {
    if (audio_data.empty()) return 0.0;
    double sum_squares = 0.0;
    for (const auto& sample : audio_data) {
        sum_squares += static_cast<double>(sample) * sample;
    }
    return std::sqrt(sum_squares / audio_data.size());
}

// 计算音频的峰值
short calculate_peak_int16(const std::vector<short>& audio_data) {
    if (audio_data.empty()) return 0;
    short max_peak = 0;
    for (const auto& sample : audio_data) {
        max_peak = std::max(max_peak, static_cast<short>(std::abs(sample)));
    }
    return max_peak;
}

// 使用RNNoise处理音频（int16格式）
std::vector<short> process_audio_with_rnnoise_int16(const std::vector<short>& input_audio, int sample_rate) {
    std::vector<short> output_audio;
    output_audio.reserve(input_audio.size());
    DenoiseState *st = rnnoise_create(NULL);
    if (!st) {
        std::cerr << "❌ RNNoise初始化失败" << std::endl;
        return input_audio;
    }
    const int frame_size = 480;
    if (sample_rate != 48000) {
        std::cout << "⚠️  警告：当前采样率为" << sample_rate << "Hz，RNNoise推荐48kHz" << std::endl;
    }
    
    // 跳过第一帧（RNNoise的惯例）
    bool first_frame = true;
    
    for (size_t i = 0; i < input_audio.size(); i += frame_size) {
        std::vector<short> input_frame(frame_size, 0);
        for (int j = 0; j < frame_size && (i + j) < input_audio.size(); ++j) {
            input_frame[j] = input_audio[i + j];
        }
        
        // 转换为float进行处理
        std::vector<float> float_frame(frame_size);
        for (int j = 0; j < frame_size; ++j) {
            float_frame[j] = static_cast<float>(input_frame[j]);
        }
        
        // RNNoise处理
        float vad_prob = rnnoise_process_frame(st, float_frame.data(), float_frame.data());
        
        // 转换回short
        std::vector<short> output_frame(frame_size);
        for (int j = 0; j < frame_size; ++j) {
            output_frame[j] = static_cast<short>(float_frame[j]);
        }
        
        // 跳过第一帧的输出
        if (!first_frame) {
            output_audio.insert(output_audio.end(), output_frame.begin(), output_frame.end());
        }
        first_frame = false;
        
        if ((i / frame_size) % 100 == 0) {
            std::cout << "帧 " << (i / frame_size) << " VAD概率: " << std::fixed
                      << std::setprecision(3) << vad_prob << std::endl;
        }
    }
    rnnoise_destroy(st);
    return output_audio;
}

int main() {
    std::cout << "=== RNNoise降噪测试（int16格式）===" << std::endl;
    int sample_rate = 48000; // RNNoise推荐48kHz
    
    // 尝试多个可能的文件路径
    std::vector<std::string> possible_files = {
        "noise_48k_mono_int16.pcm",
        "res/noise_48k_mono_int16.pcm",
        "../res/noise_48k_mono_int16.pcm",
        "res/noise_48k_mono_s16le.pcm",
        "voice_noise_48k.pcm"
    };
    
    std::vector<short> noise_data;
    std::string found_file;
    
    for (const auto& file : possible_files) {
        noise_data = read_pcm_file_int16(file);
        if (!noise_data.empty()) {
            found_file = file;
            break;
        }
    }
    
    if (noise_data.empty()) {
        std::cerr << "❌ 无法读取噪声文件，请确保文件存在且格式正确" << std::endl;
        std::cout << "请使用以下命令转换你的WAV文件：" << std::endl;
        std::cout << "ffmpeg -i res/your_noise.wav -f s16le -ar 48000 -ac 1 noise_48k_mono_int16.pcm" << std::endl;
        std::cout << "或者确保以下文件之一存在：" << std::endl;
        for (const auto& file : possible_files) {
            std::cout << "  " << file << std::endl;
        }
        return 1;
    }
    
    std::cout << "使用文件: " << found_file << std::endl;
    
    std::cout << "\n=== RNNoise降噪处理 ===" << std::endl;
    double input_rms = calculate_rms_int16(noise_data);
    short input_peak = calculate_peak_int16(noise_data);
    std::cout << "输入音频分析:" << std::endl;
    std::cout << "  RMS: " << std::fixed << std::setprecision(4) << input_rms << std::endl;
    std::cout << "  峰值: " << input_peak << std::endl;
    
    auto processed_audio = process_audio_with_rnnoise_int16(noise_data, sample_rate);
    
    double output_rms = calculate_rms_int16(processed_audio);
    short output_peak = calculate_peak_int16(processed_audio);
    std::cout << "输出音频分析:" << std::endl;
    std::cout << "  RMS: " << std::fixed << std::setprecision(4) << output_rms << std::endl;
    std::cout << "  峰值: " << output_peak << std::endl;
    std::cout << "  降噪效果: " << std::fixed << std::setprecision(2)
              << (20.0 * std::log10(output_rms / (input_rms + 1e-10))) << " dB" << std::endl;
    
    save_pcm_file_int16(processed_audio, "rnnoise_test_processed.pcm");
    
    std::cout << "\n播放命令:" << std::endl;
    std::cout << "  ffplay -f s16le -ar 48000 -nodisp -autoexit " << found_file << std::endl;
    std::cout << "  ffplay -f s16le -ar 48000 -nodisp -autoexit rnnoise_test_processed.pcm" << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
} 