#include "util/ANS.h"
#include <iostream>
#include <vector>
#include <random>
#include <fstream>
#include <iomanip>
#include <cmath>

// 生成语音模拟信号（使用多个正弦波模拟语音的谐波结构）
std::vector<spx_int16_t> generate_voice_like_signal(int sample_rate, int duration_ms, int amplitude = 6000) {
    int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<spx_int16_t> audio_data(num_samples);
    
    // 模拟语音的基频和谐波
    std::vector<int> frequencies = {150, 300, 450, 600, 750, 900, 1050, 1200}; // 基频150Hz的谐波
    std::vector<double> amplitudes = {1.0, 0.8, 0.6, 0.4, 0.3, 0.2, 0.15, 0.1}; // 谐波衰减
    
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        double signal = 0.0;
        
        // 添加基频和谐波
        for (size_t j = 0; j < frequencies.size(); ++j) {
            signal += amplitudes[j] * std::sin(2.0 * M_PI * frequencies[j] * t);
        }
        
        // 添加一些随机变化模拟真实语音
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> noise_dist(-0.1, 0.1);
        signal += noise_dist(gen);
        
        // 应用包络变化（模拟语音的起音和衰减）
        double envelope = 1.0;
        if (i < num_samples * 0.1) {
            // 起音阶段
            envelope = static_cast<double>(i) / (num_samples * 0.1);
        } else if (i > num_samples * 0.8) {
            // 衰减阶段
            envelope = static_cast<double>(num_samples - i) / (num_samples * 0.2);
        }
        
        signal *= envelope * amplitude;
        
        // 限制范围
        signal = std::max(-32768.0, std::min(32767.0, signal));
        audio_data[i] = static_cast<spx_int16_t>(std::round(signal));
    }
    
    return audio_data;
}

// 生成不同类型的噪声
std::vector<spx_int16_t> generate_noise(int sample_rate, int duration_ms, const std::string& noise_type, int amplitude = 2000) {
    int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<spx_int16_t> audio_data(num_samples);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    if (noise_type == "white") {
        // 白噪声
        std::uniform_int_distribution<int> dist(-amplitude, amplitude);
        for (int i = 0; i < num_samples; ++i) {
            audio_data[i] = static_cast<spx_int16_t>(dist(gen));
        }
    } else if (noise_type == "pink") {
        // 粉红噪声（低频能量更多）
        std::uniform_int_distribution<int> dist(-amplitude, amplitude);
        for (int i = 0; i < num_samples; ++i) {
            double t = static_cast<double>(i) / sample_rate;
            double pink_factor = 1.0 / std::sqrt(1.0 + 100.0 * t); // 低频增强
            int sample = static_cast<int>(dist(gen) * pink_factor);
            audio_data[i] = static_cast<spx_int16_t>(std::max(-32768, std::min(32767, sample)));
        }
    } else if (noise_type == "babble") {
        // 模拟多人说话声（多个不同频率的正弦波）
        std::uniform_int_distribution<int> freq_dist(200, 800);
        std::uniform_int_distribution<int> amp_dist(-amplitude/2, amplitude/2);
        
        for (int i = 0; i < num_samples; ++i) {
            double t = static_cast<double>(i) / sample_rate;
            double signal = 0.0;
            
            // 添加多个不同频率的干扰信号
            for (int j = 0; j < 5; ++j) {
                int freq = freq_dist(gen);
                int amp = amp_dist(gen);
                signal += amp * std::sin(2.0 * M_PI * freq * t + j * 0.5);
            }
            
            audio_data[i] = static_cast<spx_int16_t>(std::max(-32768, std::min(32767, static_cast<int>(signal))));
        }
    } else if (noise_type == "machine") {
        // 机器噪声（低频周期性噪声）
        for (int i = 0; i < num_samples; ++i) {
            double t = static_cast<double>(i) / sample_rate;
            double signal = amplitude * 0.5 * std::sin(2.0 * M_PI * 60 * t) +  // 60Hz基频
                           amplitude * 0.3 * std::sin(2.0 * M_PI * 120 * t) +  // 120Hz谐波
                           amplitude * 0.2 * std::sin(2.0 * M_PI * 180 * t);   // 180Hz谐波
            
            std::uniform_int_distribution<int> dist(-amplitude/4, amplitude/4);
            signal += dist(gen);
            
            audio_data[i] = static_cast<spx_int16_t>(std::max(-32768, std::min(32767, static_cast<int>(signal))));
        }
    }
    
    return audio_data;
}

// 混合音频
std::vector<spx_int16_t> mix_audio(const std::vector<spx_int16_t>& audio1, 
                                  const std::vector<spx_int16_t>& audio2) {
    size_t max_size = std::max(audio1.size(), audio2.size());
    std::vector<spx_int16_t> mixed_audio(max_size, 0);
    
    for (size_t i = 0; i < max_size; ++i) {
        int sample1 = (i < audio1.size()) ? audio1[i] : 0;
        int sample2 = (i < audio2.size()) ? audio2[i] : 0;
        int mixed = sample1 + sample2;
        
        // 防止溢出
        mixed = std::max(-32768, std::min(32767, mixed));
        mixed_audio[i] = static_cast<spx_int16_t>(mixed);
    }
    
    return mixed_audio;
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

// 计算信噪比
double calculate_snr(const std::vector<spx_int16_t>& signal, const std::vector<spx_int16_t>& noise) {
    double signal_power = calculate_rms(signal);
    double noise_power = calculate_rms(noise);
    
    if (noise_power == 0.0) return 100.0;
    
    return 20.0 * std::log10(signal_power / noise_power);
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

// 使用ANS处理音频
std::vector<spx_int16_t> process_audio_with_ans(const std::vector<spx_int16_t>& input_audio, 
                                                srv::ANS& ans, int frame_size) {
    std::vector<spx_int16_t> output_audio;
    output_audio.reserve(input_audio.size());
    
    // 前几帧用于噪声学习，不输出
    int learning_frames = 5;
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
    std::cout << "=== 真实语音ANS降噪测试 ===" << std::endl;
    
    // 配置参数
    int sample_rate = 16000;
    int frame_size = 160; // 10ms @ 16kHz
    int duration_ms = 8000; // 8秒
    int voice_amplitude = 6000; // 语音振幅
    int noise_amplitude = 2000; // 噪声振幅
    
    std::cout << "\n测试配置:" << std::endl;
    std::cout << "采样率: " << sample_rate << " Hz" << std::endl;
    std::cout << "帧大小: " << frame_size << " 样本" << std::endl;
    std::cout << "持续时间: " << duration_ms << "ms" << std::endl;
    std::cout << "语音振幅: " << voice_amplitude << std::endl;
    std::cout << "噪声振幅: " << noise_amplitude << std::endl;
    
    // 步骤1: 生成语音模拟信号
    std::cout << "\n步骤1: 生成语音模拟信号..." << std::endl;
    auto clean_voice = generate_voice_like_signal(sample_rate, duration_ms, voice_amplitude);
    std::cout << "✅ 语音信号生成完成，样本数: " << clean_voice.size() << std::endl;
    
    // 步骤2: 生成不同类型的噪声
    std::vector<std::string> noise_types = {"white", "pink", "babble", "machine"};
    std::vector<std::string> noise_names = {"白噪声", "粉红噪声", "多人说话声", "机器噪声"};
    
    for (size_t i = 0; i < noise_types.size(); ++i) {
        std::cout << "\n步骤2." << (i+1) << ": 生成" << noise_names[i] << "..." << std::endl;
        auto noise = generate_noise(sample_rate, duration_ms, noise_types[i], noise_amplitude);
        
        // 步骤3: 混合音频
        std::cout << "步骤3." << (i+1) << ": 混合语音和" << noise_names[i] << "..." << std::endl;
        auto noisy_audio = mix_audio(clean_voice, noise);
        
        // 步骤4: 计算原始信噪比
        double original_snr = calculate_snr(clean_voice, noise);
        std::cout << "原始信噪比: " << std::fixed << std::setprecision(2) << original_snr << " dB" << std::endl;
        
        // 步骤5: 保存原始文件
        std::string prefix = "voice_" + noise_types[i];
        save_pcm_file(clean_voice, prefix + "_clean_voice.pcm");
        save_pcm_file(noise, prefix + "_noise.pcm");
        save_pcm_file(noisy_audio, prefix + "_noisy_audio.pcm");
        
        // 步骤6: 初始化ANS
        std::cout << "步骤6." << (i+1) << ": 初始化ANS..." << std::endl;
        srv::ANS ans;
        if (!ans.init(sample_rate, frame_size)) {
            std::cerr << "❌ ANS初始化失败" << std::endl;
            continue;
        }
        
        // 设置ANS参数
        ans.set_noise_suppress_params(-35, -40, -15);  // 中等噪声抑制
        ans.set_agc_params(8000, 32768, 32768, 32768); // 标准AGC
        std::cout << "✅ ANS初始化完成" << std::endl;
        
        // 步骤7: 使用ANS处理音频
        std::cout << "步骤7." << (i+1) << ": 使用ANS处理音频..." << std::endl;
        auto processed_audio = process_audio_with_ans(noisy_audio, ans, frame_size);
        std::cout << "✅ ANS处理完成，输出样本数: " << processed_audio.size() << std::endl;
        
        // 步骤8: 保存处理后的文件
        save_pcm_file(processed_audio, prefix + "_processed_audio.pcm");
        
        // 步骤9: 分析结果
        std::cout << "步骤9." << (i+1) << ": 分析结果..." << std::endl;
        
        double clean_rms = calculate_rms(clean_voice);
        double noisy_rms = calculate_rms(noisy_audio);
        double processed_rms = calculate_rms(processed_audio);
        
        std::cout << "音频分析结果 (" << noise_names[i] << "):" << std::endl;
        std::cout << "  纯净语音RMS: " << std::fixed << std::setprecision(2) << clean_rms << std::endl;
        std::cout << "  带噪声音频RMS: " << std::fixed << std::setprecision(2) << noisy_rms << std::endl;
        std::cout << "  处理后音频RMS: " << std::fixed << std::setprecision(2) << processed_rms << std::endl;
        
        // 计算改善程度
        double improvement = 20.0 * std::log10(noisy_rms / (processed_rms + 1e-10));
        std::cout << "  噪声抑制效果: " << std::fixed << std::setprecision(2) << improvement << " dB" << std::endl;
        
        // 计算与纯净语音的相似度
        double similarity = 20.0 * std::log10(clean_rms / (std::abs(clean_rms - processed_rms) + 1e-10));
        std::cout << "  与纯净语音相似度: " << std::fixed << std::setprecision(2) << similarity << " dB" << std::endl;
        
        std::cout << "---" << std::endl;
    }
    
    // 步骤10: 总结
    std::cout << "\n步骤10: 总结..." << std::endl;
    std::cout << "生成的文件:" << std::endl;
    for (size_t i = 0; i < noise_types.size(); ++i) {
        std::string prefix = "voice_" + noise_types[i];
        std::cout << "  " << prefix << "_clean_voice.pcm - 纯净语音信号" << std::endl;
        std::cout << "  " << prefix << "_noise.pcm - " << noise_names[i] << std::endl;
        std::cout << "  " << prefix << "_noisy_audio.pcm - 语音+" << noise_names[i] << "（处理前）" << std::endl;
        std::cout << "  " << prefix << "_processed_audio.pcm - ANS处理后音频" << std::endl;
    }
    
    std::cout << "\n测试说明:" << std::endl;
    std::cout << "  1. 播放 *_clean_voice.pcm 听纯净语音（应该清晰）" << std::endl;
    std::cout << "  2. 播放 *_noisy_audio.pcm 听带噪声的语音（应该嘈杂）" << std::endl;
    std::cout << "  3. 播放 *_processed_audio.pcm 听处理后的语音（应该更清晰）" << std::endl;
    std::cout << "  4. 对比不同噪声类型的处理效果" << std::endl;
    
    std::cout << "\n噪声类型说明:" << std::endl;
    std::cout << "  白噪声: 在所有频率上能量均匀分布" << std::endl;
    std::cout << "  粉红噪声: 低频能量更多，更接近真实环境噪声" << std::endl;
    std::cout << "  多人说话声: 模拟会议室等场景的背景噪声" << std::endl;
    std::cout << "  机器噪声: 模拟空调、风扇等设备的低频噪声" << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
} 