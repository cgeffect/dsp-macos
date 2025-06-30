#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <string>

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

// 计算音频帧的RMS值
double calculate_frame_rms(const std::vector<float>& frame) {
    if (frame.empty()) return 0.0;
    double sum_squares = 0.0;
    for (const auto& sample : frame) {
        sum_squares += static_cast<double>(sample) * sample;
    }
    return std::sqrt(sum_squares / frame.size());
}

// 计算音频帧的峰值
float calculate_frame_peak(const std::vector<float>& frame) {
    if (frame.empty()) return 0.0f;
    float max_peak = 0.0f;
    for (const auto& sample : frame) {
        max_peak = std::max(max_peak, std::abs(sample));
    }
    return max_peak;
}

// 使用RNNoise进行VAD检测
std::vector<std::pair<float, double>> process_vad_with_rnnoise(const std::vector<short>& input_audio) {
    std::vector<std::pair<float, double>> vad_results; // <VAD概率, RMS值>
    
    DenoiseState *st = rnnoise_create(NULL);
    if (!st) {
        std::cerr << "❌ RNNoise初始化失败" << std::endl;
        return vad_results;
    }
    
    const int frame_size = 480; // RNNoise固定帧长
    const int sample_rate = 48000; // RNNoise要求48kHz
    
    std::cout << "开始VAD分析，帧大小: " << frame_size << " 样本" << std::endl;
    std::cout << "总帧数: " << (input_audio.size() / frame_size) << std::endl;
    
    for (size_t i = 0; i < input_audio.size(); i += frame_size) {
        // 创建当前帧
        std::vector<float> float_frame(frame_size, 0.0f);
        for (int j = 0; j < frame_size && (i + j) < input_audio.size(); ++j) {
            float_frame[j] = static_cast<float>(input_audio[i + j]);
        }
        
        // 计算当前帧的RMS值
        double frame_rms = calculate_frame_rms(float_frame);
        
        // RNNoise处理（获取VAD概率）
        float vad_prob = rnnoise_process_frame(st, float_frame.data(), float_frame.data());
        
        // 保存结果
        vad_results.push_back({vad_prob, frame_rms});
        
        // 每10帧输出一次详细信息
        if ((i / frame_size) % 10 == 0) {
            float frame_peak = calculate_frame_peak(float_frame);
            std::cout << "帧 " << std::setw(4) << (i / frame_size) 
                      << " | VAD概率: " << std::fixed << std::setprecision(3) << vad_prob
                      << " | RMS: " << std::fixed << std::setprecision(1) << frame_rms
                      << " | 峰值: " << std::fixed << std::setprecision(1) << frame_peak
                      << " | 时间: " << std::fixed << std::setprecision(2) 
                      << (static_cast<double>(i) / sample_rate) << "s" << std::endl;
        }
    }
    
    rnnoise_destroy(st);
    return vad_results;
}

// 分析VAD结果
void analyze_vad_results(const std::vector<std::pair<float, double>>& vad_results) {
    if (vad_results.empty()) {
        std::cerr << "❌ 没有VAD结果可分析" << std::endl;
        return;
    }
    
    std::cout << "\n=== VAD分析结果 ===" << std::endl;
    
    // 统计信息
    int total_frames = vad_results.size();
    int voice_frames = 0;
    int silence_frames = 0;
    float max_vad_prob = 0.0f;
    float min_vad_prob = 1.0f;
    double avg_vad_prob = 0.0;
    double avg_rms = 0.0;
    
    // 不同阈值下的统计
    std::vector<float> thresholds = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f};
    std::vector<int> threshold_counts(thresholds.size(), 0);
    
    for (const auto& result : vad_results) {
        float vad_prob = result.first;
        double rms = result.second;
        
        // 基本统计
        avg_vad_prob += vad_prob;
        avg_rms += rms;
        max_vad_prob = std::max(max_vad_prob, vad_prob);
        min_vad_prob = std::min(min_vad_prob, vad_prob);
        
        // 阈值统计（使用0.5作为默认阈值）
        if (vad_prob > 0.5f) {
            voice_frames++;
        } else {
            silence_frames++;
        }
        
        // 不同阈值统计
        for (size_t i = 0; i < thresholds.size(); ++i) {
            if (vad_prob > thresholds[i]) {
                threshold_counts[i]++;
            }
        }
    }
    
    avg_vad_prob /= total_frames;
    avg_rms /= total_frames;
    
    // 输出统计结果
    std::cout << "总帧数: " << total_frames << std::endl;
    std::cout << "总时长: " << std::fixed << std::setprecision(2) 
              << (static_cast<double>(total_frames) * 480.0 / 48000.0) << " 秒" << std::endl;
    std::cout << "\nVAD概率统计:" << std::endl;
    std::cout << "  最大值: " << std::fixed << std::setprecision(3) << max_vad_prob << std::endl;
    std::cout << "  最小值: " << std::fixed << std::setprecision(3) << min_vad_prob << std::endl;
    std::cout << "  平均值: " << std::fixed << std::setprecision(3) << avg_vad_prob << std::endl;
    std::cout << "\nRMS统计:" << std::endl;
    std::cout << "  平均值: " << std::fixed << std::setprecision(1) << avg_rms << std::endl;
    
    std::cout << "\n不同阈值下的语音帧比例:" << std::endl;
    for (size_t i = 0; i < thresholds.size(); ++i) {
        double percentage = (static_cast<double>(threshold_counts[i]) / total_frames) * 100.0;
        std::cout << "  VAD > " << std::fixed << std::setprecision(1) << thresholds[i] 
                  << ": " << std::setw(4) << threshold_counts[i] << " 帧 (" 
                  << std::fixed << std::setprecision(1) << percentage << "%)" << std::endl;
    }
    
    std::cout << "\n默认阈值(0.5)分析:" << std::endl;
    double voice_percentage = (static_cast<double>(voice_frames) / total_frames) * 100.0;
    double silence_percentage = (static_cast<double>(silence_frames) / total_frames) * 100.0;
    std::cout << "  语音帧: " << voice_frames << " (" << std::fixed << std::setprecision(1) 
              << voice_percentage << "%)" << std::endl;
    std::cout << "  静音帧: " << silence_frames << " (" << std::fixed << std::setprecision(1) 
              << silence_percentage << "%)" << std::endl;
}

// 保存VAD结果到文件
bool save_vad_results(const std::vector<std::pair<float, double>>& vad_results, 
                     const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "❌ 无法创建文件 " << filename << std::endl;
        return false;
    }
    
    file << "Frame,VAD_Probability,RMS,Time(s)" << std::endl;
    
    for (size_t i = 0; i < vad_results.size(); ++i) {
        float vad_prob = vad_results[i].first;
        double rms = vad_results[i].second;
        double time = static_cast<double>(i) * 480.0 / 48000.0; // 每帧10ms
        
        file << i << "," << std::fixed << std::setprecision(6) << vad_prob 
             << "," << std::fixed << std::setprecision(2) << rms 
             << "," << std::fixed << std::setprecision(3) << time << std::endl;
    }
    
    file.close();
    std::cout << "✅ VAD结果已保存到: " << filename << std::endl;
    return true;
}

int main() {
    std::cout << "=== RNNoise VAD测试 ===" << std::endl;
    
    // 尝试多个可能的文件路径
    std::vector<std::string> possible_files = {
        "res/noise_48k_mono_s16le.pcm",
        "res/noise_48k_mono_int16.pcm",
        "noise_48k_mono_int16.pcm",
        "res/noise_48k.pcm"
    };
    
    std::vector<short> audio_data;
    std::string found_file;
    
    for (const auto& file : possible_files) {
        audio_data = read_pcm_file_int16(file);
        if (!audio_data.empty()) {
            found_file = file;
            break;
        }
    }
    
    if (audio_data.empty()) {
        std::cerr << "❌ 无法读取PCM文件" << std::endl;
        std::cout << "请确保以下文件之一存在：" << std::endl;
        for (const auto& file : possible_files) {
            std::cout << "  " << file << std::endl;
        }
        std::cout << "\n或者使用以下命令转换你的WAV文件：" << std::endl;
        std::cout << "ffmpeg -i res/your_audio.wav -f s16le -ar 48000 -ac 1 res/noise_48k_mono_s16le.pcm" << std::endl;
        return 1;
    }
    
    std::cout << "使用文件: " << found_file << std::endl;
    
    // 检查采样率
    if (audio_data.size() > 0) {
        double duration = static_cast<double>(audio_data.size()) / 48000.0;
        std::cout << "音频时长: " << std::fixed << std::setprecision(2) << duration << " 秒" << std::endl;
    }
    
    // 进行VAD分析
    std::cout << "\n=== 开始VAD分析 ===" << std::endl;
    auto vad_results = process_vad_with_rnnoise(audio_data);
    
    if (vad_results.empty()) {
        std::cerr << "❌ VAD分析失败" << std::endl;
        return 1;
    }
    
    // 分析结果
    analyze_vad_results(vad_results);
    
    // 保存结果
    save_vad_results(vad_results, "rnnoise_vad_results.csv");
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    std::cout << "VAD结果已保存到 rnnoise_vad_results.csv" << std::endl;
    std::cout << "你可以用Excel或其他工具查看详细的VAD概率变化" << std::endl;
    
    return 0;
}