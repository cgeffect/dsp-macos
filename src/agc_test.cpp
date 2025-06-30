#include "util/ANS.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>

// 生成不同强度的正弦波
std::vector<spx_int16_t> generate_variable_amplitude_sine(int sample_rate, int duration_ms, 
                                                         int frequency, const std::vector<int>& amplitudes) {
    int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<spx_int16_t> audio_data(num_samples);
    
    int segment_samples = num_samples / amplitudes.size();
    
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        int segment = i / segment_samples;
        if (segment >= static_cast<int>(amplitudes.size())) {
            segment = amplitudes.size() - 1;
        }
        
        int amplitude = amplitudes[segment];
        double sine_value = amplitude * std::sin(2.0 * M_PI * frequency * t);
        audio_data[i] = static_cast<spx_int16_t>(std::round(sine_value));
    }
    
    return audio_data;
}

// 生成渐变的音量变化
std::vector<spx_int16_t> generate_fade_sine(int sample_rate, int duration_ms, 
                                           int frequency, int start_amplitude, int end_amplitude) {
    int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<spx_int16_t> audio_data(num_samples);
    
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        double progress = static_cast<double>(i) / num_samples;
        
        // 线性插值计算当前振幅
        int current_amplitude = static_cast<int>(start_amplitude + progress * (end_amplitude - start_amplitude));
        
        double sine_value = current_amplitude * std::sin(2.0 * M_PI * frequency * t);
        audio_data[i] = static_cast<spx_int16_t>(std::round(sine_value));
    }
    
    return audio_data;
}

// 生成语音模拟信号（不同音量）
std::vector<spx_int16_t> generate_voice_with_variable_volume(int sample_rate, int duration_ms, 
                                                            const std::vector<int>& volumes) {
    int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<spx_int16_t> audio_data(num_samples);
    
    // 语音的基频和谐波
    std::vector<int> frequencies = {150, 300, 450, 600, 750, 900, 1050, 1200};
    std::vector<double> amplitudes = {1.0, 0.8, 0.6, 0.4, 0.3, 0.2, 0.15, 0.1};
    
    int segment_samples = num_samples / volumes.size();
    
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        int segment = i / segment_samples;
        if (segment >= static_cast<int>(volumes.size())) {
            segment = volumes.size() - 1;
        }
        
        int volume = volumes[segment];
        double signal = 0.0;
        
        // 添加基频和谐波
        for (size_t j = 0; j < frequencies.size(); ++j) {
            signal += amplitudes[j] * std::sin(2.0 * M_PI * frequencies[j] * t);
        }
        
        signal *= volume;
        
        // 限制范围
        signal = std::max(-32768.0, std::min(32767.0, signal));
        audio_data[i] = static_cast<spx_int16_t>(std::round(signal));
    }
    
    return audio_data;
}

// 计算音频的RMS值
double calculate_rms(const std::vector<spx_int16_t>& audio_data) {
    if (audio_data.empty()) return 0.0;
    
    double sum_squares = 0.0;
    for (const auto& sample : audio_data) {
        sum_squares += static_cast<double>(sample) * sample;
    }
    
    return std::sqrt(sum_squares / audio_data.size());
}

// 计算音频的峰值
int calculate_peak(const std::vector<spx_int16_t>& audio_data) {
    if (audio_data.empty()) return 0;
    
    int max_peak = 0;
    for (const auto& sample : audio_data) {
        max_peak = std::max(max_peak, std::abs(sample));
    }
    
    return max_peak;
}

// 保存PCM文件
bool save_pcm_file(const std::vector<spx_int16_t>& audio_data, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "错误：无法创建文件 " << filename << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(audio_data.data()), 
               audio_data.size() * sizeof(spx_int16_t));
    file.close();
    
    std::cout << "✅ PCM文件已保存: " << filename << std::endl;
    std::cout << "   文件大小: " << (audio_data.size() * sizeof(spx_int16_t)) << " 字节" << std::endl;
    
    return true;
}

// 使用ANS处理音频（只启用AGC）
std::vector<spx_int16_t> process_audio_with_agc(const std::vector<spx_int16_t>& input_audio, 
                                                srv::ANS& ans, int frame_size) {
    std::vector<spx_int16_t> output_audio;
    output_audio.reserve(input_audio.size());
    
    // 前几帧用于学习，不输出
    int learning_frames = 3;
    int frame_count = 0;
    
    for (size_t i = 0; i < input_audio.size(); i += frame_size) {
        frame_count++;
        
        // 创建当前帧
        std::vector<spx_int16_t> frame;
        for (int j = 0; j < frame_size && (i + j) < input_audio.size(); ++j) {
            frame.push_back(input_audio[i + j]);
        }
        
        // 如果帧不完整，用0填充
        while (frame.size() < static_cast<size_t>(frame_size)) {
            frame.push_back(0);
        }
        
        // 使用ANS处理帧
        auto processed_frame = ans.process_frame(frame);
        
        if (processed_frame.empty()) {
            std::cerr << "警告：帧处理失败，使用原始帧" << std::endl;
            if (frame_count > learning_frames) {
                output_audio.insert(output_audio.end(), frame.begin(), frame.end());
            }
        } else {
            // 前几帧用于学习，不输出
            if (frame_count > learning_frames) {
                output_audio.insert(output_audio.end(), processed_frame.begin(), processed_frame.end());
            }
        }
    }
    
    return output_audio;
}

int main() {
    std::cout << "=== AGC自动增益控制测试 ===" << std::endl;
    
    // 配置参数
    int sample_rate = 16000;
    int frame_size = 160; // 10ms @ 16kHz
    int duration_ms = 6000; // 6秒
    
    std::cout << "\n测试配置:" << std::endl;
    std::cout << "采样率: " << sample_rate << " Hz" << std::endl;
    std::cout << "帧大小: " << frame_size << " 样本" << std::endl;
    std::cout << "持续时间: " << duration_ms << "ms" << std::endl;
    
    // 测试1: 不同振幅的正弦波
    std::cout << "\n=== 测试1: 不同振幅的正弦波 ===" << std::endl;
    std::vector<int> amplitudes = {1000, 3000, 8000, 15000, 500, 12000, 2000, 10000};
    auto variable_sine = generate_variable_amplitude_sine(sample_rate, duration_ms, 440, amplitudes);
    
    std::cout << "生成不同振幅正弦波，振幅序列: ";
    for (size_t i = 0; i < amplitudes.size(); ++i) {
        std::cout << amplitudes[i];
        if (i < amplitudes.size() - 1) std::cout << " → ";
    }
    std::cout << std::endl;
    
    save_pcm_file(variable_sine, "agc_variable_sine_input.pcm");
    
    // 测试2: 渐变音量
    std::cout << "\n=== 测试2: 渐变音量 ===" << std::endl;
    auto fade_sine = generate_fade_sine(sample_rate, duration_ms, 440, 500, 15000);
    std::cout << "生成渐变音量正弦波: 500 → 15000" << std::endl;
    save_pcm_file(fade_sine, "agc_fade_sine_input.pcm");
    
    // 测试3: 语音信号（不同音量）
    std::cout << "\n=== 测试3: 语音信号（不同音量） ===" << std::endl;
    std::vector<int> voice_volumes = {2000, 8000, 1500, 12000, 3000, 10000, 1000, 9000};
    auto variable_voice = generate_voice_with_variable_volume(sample_rate, duration_ms, voice_volumes);
    
    std::cout << "生成不同音量语音信号，音量序列: ";
    for (size_t i = 0; i < voice_volumes.size(); ++i) {
        std::cout << voice_volumes[i];
        if (i < voice_volumes.size() - 1) std::cout << " → ";
    }
    std::cout << std::endl;
    
    save_pcm_file(variable_voice, "agc_variable_voice_input.pcm");
    
    // 测试不同的AGC参数
    std::vector<std::pair<std::string, std::vector<int>>> agc_configs = {
        {"标准AGC", {8000, 32768, 32768, 32768}},
        {"强AGC", {4000, 16384, 16384, 16384}},
        {"弱AGC", {12000, 49152, 49152, 49152}},
        {"快速AGC", {8000, 16384, 16384, 32768}},
        {"慢速AGC", {8000, 49152, 49152, 32768}}
    };
    
    std::vector<std::vector<spx_int16_t>> test_inputs = {
        variable_sine,
        fade_sine,
        variable_voice
    };
    
    std::vector<std::string> test_names = {
        "variable_sine",
        "fade_sine", 
        "variable_voice"
    };
    
    for (size_t config_idx = 0; config_idx < agc_configs.size(); ++config_idx) {
        const auto& config = agc_configs[config_idx];
        std::cout << "\n=== 测试AGC配置: " << config.first << " ===" << std::endl;
        
        for (size_t test_idx = 0; test_idx < test_inputs.size(); ++test_idx) {
            std::cout << "\n处理 " << test_names[test_idx] << "..." << std::endl;
            
            // 初始化ANS（只启用AGC）
            srv::ANS ans;
            if (!ans.init(sample_rate, frame_size)) {
                std::cerr << "❌ ANS初始化失败" << std::endl;
                continue;
            }
            
            // 设置AGC参数，禁用噪声抑制
            ans.set_noise_suppress_params(0, 0, 0);  // 禁用噪声抑制
            ans.set_agc_params(config.second[0], config.second[1], config.second[2], config.second[3]);
            std::cout << "AGC参数: 目标电平=" << config.second[0] 
                      << ", 增量=" << config.second[1] 
                      << ", 减量=" << config.second[2] 
                      << ", 最大增益=" << config.second[3] << std::endl;
            
            // 处理音频
            auto processed_audio = process_audio_with_agc(test_inputs[test_idx], ans, frame_size);
            
            // 分析结果
            double input_rms = calculate_rms(test_inputs[test_idx]);
            double output_rms = calculate_rms(processed_audio);
            int input_peak = calculate_peak(test_inputs[test_idx]);
            int output_peak = calculate_peak(processed_audio);
            
            std::cout << "分析结果:" << std::endl;
            std::cout << "  输入RMS: " << std::fixed << std::setprecision(2) << input_rms << std::endl;
            std::cout << "  输出RMS: " << std::fixed << std::setprecision(2) << output_rms << std::endl;
            std::cout << "  输入峰值: " << input_peak << std::endl;
            std::cout << "  输出峰值: " << output_peak << std::endl;
            std::cout << "  增益变化: " << std::fixed << std::setprecision(2) 
                      << (20.0 * std::log10(output_rms / (input_rms + 1e-10))) << " dB" << std::endl;
            
            // 保存结果
            std::string filename = "agc_" + config.first + "_" + test_names[test_idx] + "_output.pcm";
            save_pcm_file(processed_audio, filename);
        }
    }
    
    // 总结
    std::cout << "\n=== 测试总结 ===" << std::endl;
    std::cout << "生成的文件:" << std::endl;
    std::cout << "输入文件:" << std::endl;
    std::cout << "  agc_variable_sine_input.pcm - 不同振幅正弦波" << std::endl;
    std::cout << "  agc_fade_sine_input.pcm - 渐变音量正弦波" << std::endl;
    std::cout << "  agc_variable_voice_input.pcm - 不同音量语音" << std::endl;
    
    std::cout << "\n输出文件:" << std::endl;
    for (const auto& config : agc_configs) {
        for (const auto& test_name : test_names) {
            std::cout << "  agc_" << config.first << "_" << test_name << "_output.pcm" << std::endl;
        }
    }
    
    std::cout << "\n测试说明:" << std::endl;
    std::cout << "  1. 播放 *_input.pcm 听原始音频（音量变化很大）" << std::endl;
    std::cout << "  2. 播放 *_output.pcm 听AGC处理后音频（音量应该更稳定）" << std::endl;
    std::cout << "  3. 对比不同AGC配置的效果" << std::endl;
    
    std::cout << "\nAGC参数说明:" << std::endl;
    std::cout << "  目标电平: AGC试图达到的输出电平" << std::endl;
    std::cout << "  增量: 音量增加时的增益调整速度" << std::endl;
    std::cout << "  减量: 音量减少时的增益调整速度" << std::endl;
    std::cout << "  最大增益: AGC允许的最大增益倍数" << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
} 