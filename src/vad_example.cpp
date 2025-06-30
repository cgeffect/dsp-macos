#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <fstream>
#include <string>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include "util/VAD.h"

// ==================== PCM数据生成工具 ====================

// PCM文件头信息结构
struct PCMFileInfo {
    int sample_rate;
    int channels;
    int bit_depth;
    std::string filename;
    
    PCMFileInfo(int sr, int ch, int bd) : sample_rate(sr), channels(ch), bit_depth(bd) {
        filename = std::to_string(sample_rate) + "Hz_" + 
                   std::to_string(channels) + "ch_" + 
                   std::to_string(bit_depth) + "bit.pcm";
    }
    
    // 计算每个样本的字节数
    int bytes_per_sample() const {
        return bit_depth / 8;
    }
    
    // 计算每个帧的字节数
    int bytes_per_frame() const {
        return channels * bytes_per_sample();
    }
    
    // 计算指定字节数对应的时间(毫秒)
    double bytes_to_ms(size_t bytes) const {
        return (bytes * 1000.0) / (sample_rate * bytes_per_frame());
    }
    
    // 计算指定时间(毫秒)对应的字节数
    size_t ms_to_bytes(double ms) const {
        return static_cast<size_t>((ms * sample_rate * bytes_per_frame()) / 1000.0);
    }
    
    // 计算指定样本数对应的时间(毫秒)
    double samples_to_ms(size_t samples) const {
        return (samples * 1000.0) / sample_rate;
    }
};

// 生成正弦波音频数据
std::vector<spx_int16_t> generate_sine_wave(int sample_rate, int duration_ms, 
                                           int frequency, int amplitude = 8000) {
    int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<spx_int16_t> audio_data(num_samples);
    
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        double sine_value = std::sin(2.0 * M_PI * frequency * t);
        audio_data[i] = static_cast<spx_int16_t>(amplitude * sine_value);
    }
    
    return audio_data;
}

// 生成静音数据
std::vector<spx_int16_t> generate_silence(int sample_rate, int duration_ms) {
    int num_samples = (sample_rate * duration_ms) / 1000;
    return std::vector<spx_int16_t>(num_samples, 0);
}

// 生成带噪声的音频数据
std::vector<spx_int16_t> generate_noisy_audio(int sample_rate, int duration_ms, 
                                             int frequency, int noise_level = 1000) {
    int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<spx_int16_t> audio_data(num_samples);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> noise_dist(0.0, noise_level);
    
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        double sine_value = std::sin(2.0 * M_PI * frequency * t);
        double noise = noise_dist(gen);
        double combined = 8000 * sine_value + noise;
        
        // 限制在16位范围内
        combined = std::max(-32768.0, std::min(32767.0, combined));
        audio_data[i] = static_cast<spx_int16_t>(combined);
    }
    
    return audio_data;
}

// 生成低音量音频数据（模拟接近静音的声音）
std::vector<spx_int16_t> generate_low_volume_audio(int sample_rate, int duration_ms, 
                                                   int frequency, int amplitude = 100) {
    int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<spx_int16_t> audio_data(num_samples);
    
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        double sine_value = std::sin(2.0 * M_PI * frequency * t);
        audio_data[i] = static_cast<spx_int16_t>(amplitude * sine_value);
    }
    
    return audio_data;
}

// ==================== PCM文件操作工具 ====================

// 保存PCM数据到文件
bool save_pcm_file(const std::vector<spx_int16_t>& audio_data, const PCMFileInfo& info) {
    std::ofstream file(info.filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "错误：无法创建文件 " << info.filename << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(audio_data.data()), 
               audio_data.size() * sizeof(spx_int16_t));
    file.close();
    
    std::cout << "✅ PCM文件已保存: " << info.filename << std::endl;
    std::cout << "   文件大小: " << (audio_data.size() * sizeof(spx_int16_t)) << " 字节" << std::endl;
    std::cout << "   音频时长: " << info.samples_to_ms(audio_data.size()) << " 毫秒" << std::endl;
    
    return true;
}

// 从文件读取PCM数据
std::vector<spx_int16_t> load_pcm_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "错误：无法打开文件 " << filename << std::endl;
        return std::vector<spx_int16_t>();
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<spx_int16_t> audio_data(size / sizeof(spx_int16_t));
    file.read(reinterpret_cast<char*>(audio_data.data()), size);
    file.close();
    
    std::cout << "✅ PCM文件已加载: " << filename << std::endl;
    std::cout << "   文件大小: " << size << " 字节" << std::endl;
    std::cout << "   样本数量: " << audio_data.size() << std::endl;
    
    return audio_data;
}

// ==================== 静音检测工具 ====================

// 简单的静音检测（基于阈值）
struct SilenceSegment {
    size_t start_byte;
    size_t end_byte;
    double start_ms;
    double end_ms;
    double duration_ms;
    
    SilenceSegment(size_t start_b, size_t end_b, double start_m, double end_m)
        : start_byte(start_b), end_byte(end_b), start_ms(start_m), end_ms(end_m) {
        duration_ms = end_ms - start_ms;
    }
};

// 传统阈值检测方法（非Speex）- 改进版
std::vector<SilenceSegment> detect_silence_threshold(const std::vector<spx_int16_t>& audio_data, 
                                                     const PCMFileInfo& info, 
                                                     int threshold = 100) {
    std::vector<SilenceSegment> silence_segments;
    bool in_silence = false;
    size_t silence_start_byte = 0;
    size_t silence_start_sample = 0;
    
    // 调试统计
    int total_samples = 0;
    int silent_samples = 0;
    int voice_samples = 0;
    
    // 平滑处理参数
    const int window_size = 160; // 10ms窗口 @ 16kHz
    const int min_silence_duration = 160; // 最小静音持续时间（样本数）
    
    std::cout << "阈值检测调试信息:" << std::endl;
    std::cout << "  阈值: " << threshold << std::endl;
    std::cout << "  总样本数: " << audio_data.size() << std::endl;
    std::cout << "  平滑窗口: " << window_size << " 样本 (" << (window_size * 1000.0 / info.sample_rate) << "ms)" << std::endl;
    std::cout << "  最小静音时长: " << min_silence_duration << " 样本 (" << (min_silence_duration * 1000.0 / info.sample_rate) << "ms)" << std::endl;
    
    for (size_t i = 0; i < audio_data.size(); ++i) {
        // 计算滑动窗口内的平均能量
        double window_energy = 0.0;
        int window_count = 0;
        
        for (int j = 0; j < window_size && (i + j) < audio_data.size(); ++j) {
            window_energy += std::abs(audio_data[i + j]);
            window_count++;
        }
        
        // 计算平均幅度
        double avg_amplitude = window_energy / window_count;
        
        // 基于平均幅度的阈值检测
        bool is_silent = avg_amplitude <= threshold;
        size_t current_byte = i * sizeof(spx_int16_t);
        double current_ms = info.samples_to_ms(i);
        
        // 统计
        total_samples++;
        if (is_silent) {
            silent_samples++;
        } else {
            voice_samples++;
        }
        
        if (is_silent && !in_silence) {
            // 开始静音
            in_silence = true;
            silence_start_byte = current_byte;
            silence_start_sample = i;
        } else if (!is_silent && in_silence) {
            // 结束静音，检查持续时间
            size_t silence_duration = i - silence_start_sample;
            if (silence_duration >= min_silence_duration) {
                double silence_start_ms = info.samples_to_ms(silence_start_sample);
                silence_segments.emplace_back(silence_start_byte, current_byte, 
                                            silence_start_ms, current_ms);
            }
            in_silence = false;
        }
    }
    
    // 处理文件末尾的静音
    if (in_silence) {
        size_t silence_duration = audio_data.size() - silence_start_sample;
        if (silence_duration >= min_silence_duration) {
            size_t end_byte = audio_data.size() * sizeof(spx_int16_t);
            double end_ms = info.samples_to_ms(audio_data.size());
            double silence_start_ms = info.samples_to_ms(silence_start_sample);
            silence_segments.emplace_back(silence_start_byte, end_byte, 
                                        silence_start_ms, end_ms);
        }
    }
    
    // 输出统计信息
    std::cout << "阈值检测统计结果:" << std::endl;
    std::cout << "  总样本数: " << total_samples << std::endl;
    std::cout << "  静音样本: " << silent_samples << " (" << (silent_samples * 100.0 / total_samples) << "%)" << std::endl;
    std::cout << "  语音样本: " << voice_samples << " (" << (voice_samples * 100.0 / total_samples) << "%)" << std::endl;
    std::cout << "  检测到的静音段数: " << silence_segments.size() << std::endl;
    
    return silence_segments;
}

// Speex VAD智能静音检测
std::vector<SilenceSegment> detect_silence_speex(const std::vector<spx_int16_t>& audio_data, 
                                                 const PCMFileInfo& info, 
                                                 srv::VAD& vad) {
    std::vector<SilenceSegment> silence_segments;
    bool in_silence = false;
    size_t silence_start_byte = 0;
    size_t silence_start_frame = 0;
    int frame_size = vad.get_frame_size();
    
    // 调试统计
    int total_frames = 0;
    int silent_frames = 0;
    int voice_frames = 0;
    
    std::cout << "VAD调试信息:" << std::endl;
    std::cout << "  帧大小: " << frame_size << " 样本 (" << (frame_size * 1000.0 / info.sample_rate) << "ms)" << std::endl;
    std::cout << "  总样本数: " << audio_data.size() << std::endl;
    std::cout << "  预计帧数: " << (audio_data.size() + frame_size - 1) / frame_size << std::endl;
    
    for (size_t frame_start = 0; frame_start < audio_data.size(); frame_start += frame_size) {
        // 创建当前帧
        std::vector<spx_int16_t> frame;
        for (int j = 0; j < frame_size && (frame_start + j) < audio_data.size(); ++j) {
            frame.push_back(audio_data[frame_start + j]);
        }
        
        // 如果帧不完整，用0填充
        while (frame.size() < static_cast<size_t>(frame_size)) {
            frame.push_back(0);
        }
        
        // 使用Speex VAD进行智能语音活动检测
        int vad_result = vad.detect_voice_activity(frame);
        bool is_silent = (vad_result == 0);
        
        // 统计
        total_frames++;
        if (is_silent) {
            silent_frames++;
        } else {
            voice_frames++;
        }
        
        // 调试输出（前10帧和每100帧输出一次）
        if (total_frames <= 10 || total_frames % 100 == 0) {
            double frame_start_ms = info.samples_to_ms(frame_start);
            std::cout << "  帧 " << total_frames << " (@" << std::fixed << std::setprecision(1) 
                      << frame_start_ms << "ms): " << (is_silent ? "静音" : "语音") 
                      << " (VAD结果=" << vad_result << ")" << std::endl;
        }
        
        size_t current_byte = frame_start * sizeof(spx_int16_t);
        double current_ms = info.samples_to_ms(frame_start);
        
        if (is_silent && !in_silence) {
            // 开始静音
            in_silence = true;
            silence_start_byte = current_byte;
            silence_start_frame = frame_start;
        } else if (!is_silent && in_silence) {
            // 结束静音
            in_silence = false;
            double silence_start_ms = info.samples_to_ms(silence_start_frame);
            silence_segments.emplace_back(silence_start_byte, current_byte, 
                                        silence_start_ms, current_ms);
        }
    }
    
    // 处理文件末尾的静音
    if (in_silence) {
        size_t end_byte = audio_data.size() * sizeof(spx_int16_t);
        double end_ms = info.samples_to_ms(audio_data.size());
        double silence_start_ms = info.samples_to_ms(silence_start_frame);
        silence_segments.emplace_back(silence_start_byte, end_byte, 
                                    silence_start_ms, end_ms);
    }
    
    // 输出统计信息
    std::cout << "VAD统计结果:" << std::endl;
    std::cout << "  总帧数: " << total_frames << std::endl;
    std::cout << "  静音帧: " << silent_frames << " (" << (silent_frames * 100.0 / total_frames) << "%)" << std::endl;
    std::cout << "  语音帧: " << voice_frames << " (" << (voice_frames * 100.0 / total_frames) << "%)" << std::endl;
    std::cout << "  检测到的静音段数: " << silence_segments.size() << std::endl;
    
    return silence_segments;
}

// 打印静音段信息
void print_silence_segments(const std::vector<SilenceSegment>& segments, 
                           const std::string& method_name) {
    std::cout << "\n=== " << method_name << " 静音检测结果 ===" << std::endl;
    std::cout << "检测到 " << segments.size() << " 个静音段:" << std::endl;
    
    for (size_t i = 0; i < segments.size(); ++i) {
        const auto& seg = segments[i];
        std::cout << "静音段 " << (i + 1) << ":" << std::endl;
        std::cout << "  时间范围: " << std::fixed << std::setprecision(2) 
                  << seg.start_ms << "ms - " << seg.end_ms << "ms" << std::endl;
        std::cout << "  持续时间: " << std::fixed << std::setprecision(2) 
                  << seg.duration_ms << "ms" << std::endl;
        std::cout << "  字节范围: " << seg.start_byte << " - " << seg.end_byte << std::endl;
        std::cout << "  字节数量: " << (seg.end_byte - seg.start_byte) << " 字节" << std::endl;
    }
}

// ==================== 测试音频生成 ====================

// 生成测试音频序列
std::vector<spx_int16_t> generate_test_audio_sequence(int sample_rate, int channels, int bit_depth) {
    std::vector<spx_int16_t> audio_sequence;
    
    // 1. 500ms 静音
    auto silence1 = generate_silence(sample_rate, 500);
    audio_sequence.insert(audio_sequence.end(), silence1.begin(), silence1.end());
    
    // 2. 1000ms 440Hz 正弦波
    auto sine1 = generate_sine_wave(sample_rate, 1000, 440, 8000);
    audio_sequence.insert(audio_sequence.end(), sine1.begin(), sine1.end());
    
    // 3. 300ms 静音
    auto silence2 = generate_silence(sample_rate, 300);
    audio_sequence.insert(audio_sequence.end(), silence2.begin(), silence2.end());
    
    // 4. 800ms 880Hz 正弦波
    auto sine2 = generate_sine_wave(sample_rate, 800, 880, 6000);
    audio_sequence.insert(audio_sequence.end(), sine2.begin(), sine2.end());
    
    // 5. 200ms 低音量音频（接近静音）
    auto low_volume = generate_low_volume_audio(sample_rate, 200, 220, 50);
    audio_sequence.insert(audio_sequence.end(), low_volume.begin(), low_volume.end());
    
    // 6. 400ms 静音
    auto silence3 = generate_silence(sample_rate, 400);
    audio_sequence.insert(audio_sequence.end(), silence3.begin(), silence3.end());
    
    // 7. 600ms 带噪声的音频
    auto noisy = generate_noisy_audio(sample_rate, 600, 660, 2000);
    audio_sequence.insert(audio_sequence.end(), noisy.begin(), noisy.end());
    
    // 8. 500ms 静音
    auto silence4 = generate_silence(sample_rate, 500);
    audio_sequence.insert(audio_sequence.end(), silence4.begin(), silence4.end());
    
    return audio_sequence;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "=== SpeexDSP 静音检测示例程序 ===" << std::endl;
    
    // 配置音频参数
    int sample_rate = 16000;
    int channels = 1;
    int bit_depth = 16;
    
    PCMFileInfo pcm_info(sample_rate, channels, bit_depth);
    
    std::cout << "\n音频配置:" << std::endl;
    std::cout << "采样率: " << sample_rate << " Hz" << std::endl;
    std::cout << "声道数: " << channels << std::endl;
    std::cout << "位深度: " << bit_depth << " bit" << std::endl;
    std::cout << "文件名: " << pcm_info.filename << std::endl;
    
    // 步骤1: 生成测试音频数据
    std::cout << "\n步骤1: 生成测试音频数据..." << std::endl;
    auto test_audio = generate_test_audio_sequence(sample_rate, channels, bit_depth);
    std::cout << "✅ 测试音频生成完成，总时长: " << pcm_info.samples_to_ms(test_audio.size()) << "ms" << std::endl;
    
    // 步骤2: 保存PCM文件
    std::cout << "\n步骤2: 保存PCM文件..." << std::endl;
    if (!save_pcm_file(test_audio, pcm_info)) {
        std::cerr << "❌ PCM文件保存失败" << std::endl;
        return -1;
    }
    
    // 步骤3: 从文件读取PCM数据
    std::cout << "\n步骤3: 从文件读取PCM数据..." << std::endl;
    auto loaded_audio = load_pcm_file(pcm_info.filename);
    if (loaded_audio.empty()) {
        std::cerr << "❌ PCM文件读取失败" << std::endl;
        return -1;
    }
    
    // 步骤4: 初始化VAD
    std::cout << "\n步骤4: 初始化VAD..." << std::endl;
    srv::VAD vad;
    if (!vad.init(sample_rate, 160)) {  // 160样本 = 10ms @ 16kHz
        std::cerr << "❌ VAD初始化失败" << std::endl;
        return -1;
    }
    
    // 设置VAD参数
    vad.set_vad_params(80, 80, -15);  // 中等敏感度
    std::cout << "✅ VAD初始化完成" << std::endl;
    
    // 步骤5: 基于阈值的静音检测
    std::cout << "\n步骤5: 基于阈值的静音检测..." << std::endl;
    auto threshold_segments = detect_silence_threshold(loaded_audio, pcm_info, 100);
    print_silence_segments(threshold_segments, "阈值检测");
    
    // 步骤6: Speex VAD静音检测
    std::cout << "\n步骤6: Speex VAD静音检测..." << std::endl;
    auto speex_segments = detect_silence_speex(loaded_audio, pcm_info, vad);
    print_silence_segments(speex_segments, "Speex VAD检测");
    
    // 步骤7: 对比分析
    std::cout << "\n步骤7: 检测方法对比分析..." << std::endl;
    std::cout << "传统阈值检测方法:" << std::endl;
    std::cout << "  - 方法: 基于滑动窗口的平均能量阈值检测" << std::endl;
    std::cout << "  - 优点: 平滑处理，更接近实际应用" << std::endl;
    std::cout << "  - 缺点: 无法区分低音量语音和噪声，容易误判" << std::endl;
    std::cout << "  - 适用: 对纯静音检测要求较高的场景，或资源受限环境" << std::endl;
    
    std::cout << "\nSpeex VAD智能检测方法:" << std::endl;
    std::cout << "  - 方法: 基于频谱分析的智能语音活动检测" << std::endl;
    std::cout << "  - 优点: 智能识别语音特征，抗噪声能力强，准确性高" << std::endl;
    std::cout << "  - 缺点: 计算复杂度较高，需要更多内存和CPU资源" << std::endl;
    std::cout << "  - 适用: 实际语音通信场景，需要高精度检测的应用" << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    std::cout << "生成的PCM文件: " << pcm_info.filename << std::endl;
    std::cout << "可以使用音频播放器播放此文件进行验证" << std::endl;
    
    return 0;
}
