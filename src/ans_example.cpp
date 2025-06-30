#include "util/ANS.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <cmath>

// 读取PCM文件（int16格式）
std::vector<spx_int16_t> read_pcm_file_int16(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "❌ 错误：无法打开文件 " << filename << std::endl;
        return {};
    }
    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    size_t num_samples = file_size / sizeof(spx_int16_t);
    std::vector<spx_int16_t> audio_data(num_samples);
    file.read(reinterpret_cast<char*>(audio_data.data()), file_size);
    file.close();
    std::cout << "✅ 成功读取PCM文件: " << filename << std::endl;
    std::cout << "   文件大小: " << file_size << " 字节" << std::endl;
    std::cout << "   样本数量: " << num_samples << std::endl;
    std::cout << "   时长: " << std::fixed << std::setprecision(2)
              << (static_cast<double>(num_samples) / 48000.0) << " 秒" << std::endl;
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
spx_int16_t calculate_peak(const std::vector<spx_int16_t>& audio_data) {
    if (audio_data.empty()) return 0;
    spx_int16_t max_peak = 0;
    for (const auto& sample : audio_data) {
        max_peak = std::max(max_peak, static_cast<spx_int16_t>(std::abs(sample)));
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

// 使用ANS处理音频
std::vector<spx_int16_t> process_audio_with_ans(const std::vector<spx_int16_t>& input_audio, 
                                                srv::ANS& ans, int frame_size) {
    std::vector<spx_int16_t> output_audio;
    output_audio.reserve(input_audio.size());
    
    // 前几帧用于噪声学习，不输出
    int learning_frames = 10;
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
    std::cout << "=== ANS降噪测试（读取本地PCM文件）===" << std::endl;
    
    // 配置参数
    int sample_rate = 16000; // 匹配输入文件的采样率
    int frame_size = 160; // 10ms @ 16kHz
    
    std::cout << "\n测试配置:" << std::endl;
    std::cout << "采样率: " << sample_rate << " Hz" << std::endl;
    std::cout << "帧大小: " << frame_size << " 样本" << std::endl;
    
    // 步骤1: 读取PCM文件
    std::cout << "\n步骤1: 读取PCM文件..." << std::endl;
    auto input_audio = read_pcm_file_int16("res/noise_16k_mono_s16le.pcm");
    
    if (input_audio.empty()) {
        std::cerr << "❌ 无法读取PCM文件，请确保 res/noise_16k_mono_s16le.pcm 存在" << std::endl;
        std::cout << "请使用以下命令转换你的WAV文件：" << std::endl;
        std::cout << "ffmpeg -i res/your_noise.wav -f s16le -ar 16000 -ac 1 res/noise_16k_mono_s16le.pcm" << std::endl;
        return 1;
    }
    
    // 步骤2: 分析输入音频
    std::cout << "\n步骤2: 分析输入音频..." << std::endl;
    double input_rms = calculate_rms(input_audio);
    spx_int16_t input_peak = calculate_peak(input_audio);
    std::cout << "输入音频分析:" << std::endl;
    std::cout << "  RMS: " << std::fixed << std::setprecision(4) << input_rms << std::endl;
    std::cout << "  峰值: " << input_peak << std::endl;
    
    // 步骤3: 初始化ANS
    std::cout << "\n步骤3: 初始化ANS..." << std::endl;
    srv::ANS ans;
    if (!ans.init(sample_rate, frame_size)) {
        std::cerr << "❌ ANS初始化失败" << std::endl;
        return 1;
    }
    
    // 设置ANS参数
    ans.set_noise_suppress_params(-45, -50, -30);  // 更强的噪声抑制
    ans.set_agc_enabled(false); // 关闭AGC
    std::cout << "✅ ANS初始化完成" << std::endl;
    
    // 步骤4: 使用ANS处理音频
    std::cout << "\n步骤4: 使用ANS处理音频..." << std::endl;
    auto processed_audio = process_audio_with_ans(input_audio, ans, frame_size);
    std::cout << "✅ ANS处理完成，输出样本数: " << processed_audio.size() << std::endl;
    
    // 步骤5: 分析输出音频
    std::cout << "\n步骤5: 分析输出音频..." << std::endl;
    double output_rms = calculate_rms(processed_audio);
    spx_int16_t output_peak = calculate_peak(processed_audio);
    std::cout << "输出音频分析:" << std::endl;
    std::cout << "  RMS: " << std::fixed << std::setprecision(4) << output_rms << std::endl;
    std::cout << "  峰值: " << output_peak << std::endl;
    
    // 计算降噪效果
    double noise_reduction = 20.0 * std::log10(input_rms / (output_rms + 1e-10));
    std::cout << "  降噪效果: " << std::fixed << std::setprecision(2) << noise_reduction << " dB" << std::endl;
    
    // 步骤6: 保存处理后的文件
    std::cout << "\n步骤6: 保存处理后的文件..." << std::endl;
    save_pcm_file(processed_audio, "ans_processed_audio_16k.pcm");
    
    // 步骤7: 总结
    std::cout << "\n步骤7: 总结..." << std::endl;
    std::cout << "生成的文件:" << std::endl;
    std::cout << "  res/noise_16k_mono_s16le.pcm - 原始音频（处理前）" << std::endl;
    std::cout << "  ans_processed_audio_16k.pcm - ANS处理后音频" << std::endl;
    
    std::cout << "\n播放命令:" << std::endl;
    std::cout << "  ffplay -f s16le -ar 16000 -nodisp -autoexit res/noise_16k_mono_s16le.pcm" << std::endl;
    std::cout << "  ffplay -f s16le -ar 16000 -nodisp -autoexit ans_processed_audio_16k.pcm" << std::endl;
    
    std::cout << "\n测试说明:" << std::endl;
    std::cout << "  1. 播放 res/noise_16k_mono_s16le.pcm 听原始音频" << std::endl;
    std::cout << "  2. 播放 ans_processed_audio_16k.pcm 听处理后的音频" << std::endl;
    std::cout << "  3. 对比两个文件的降噪效果" << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
} 