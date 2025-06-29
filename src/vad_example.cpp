#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include "util/VAD.h"

// ==================== 音频生成工具函数 ====================

// 生成测试音频数据
std::vector<spx_int16_t> generate_sine_wave(int sample_rate, int duration_ms, int frequency, int amplitude = 8000) {
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
std::vector<spx_int16_t> generate_noisy_audio(int sample_rate, int duration_ms, int frequency, int noise_level = 1000) {
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

// ==================== VAD处理工具函数 ====================

// 按帧处理音频数据
void process_audio_frames(const std::vector<spx_int16_t>& audio_data, srv::VAD& vad, int frame_size, bool show_details = true) {
    if (show_details) {
        std::cout << "\n=== 开始VAD检测 ===" << std::endl;
    }
    
    int frame_count = 0;
    int speech_frames = 0;
    int silence_frames = 0;
    
    for (size_t i = 0; i < audio_data.size(); i += frame_size) {
        // 创建当前帧
        std::vector<spx_int16_t> frame;
        for (int j = 0; j < frame_size && (i + j) < audio_data.size(); ++j) {
            frame.push_back(audio_data[i + j]);
        }
        
        // 如果帧不完整，用0填充
        while (frame.size() < static_cast<size_t>(frame_size)) {
            frame.push_back(0);
        }
        
        // 进行VAD检测
        int vad_result = vad.detect_voice_activity(frame);
        int speech_prob = vad.get_speech_probability();
        
        frame_count++;
        
        if (vad_result == 1) {
            speech_frames++;
            if (show_details) {
                std::cout << "帧 " << frame_count << ": 检测到语音 (概率: " << speech_prob << "%)" << std::endl;
            }
        } else {
            silence_frames++;
            if (show_details) {
                std::cout << "帧 " << frame_count << ": 检测到静音 (概率: " << speech_prob << "%)" << std::endl;
            }
        }
        
        // 添加小延迟以便观察输出
        if (show_details) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    std::cout << "\n=== VAD检测结果 ===" << std::endl;
    std::cout << "总帧数: " << frame_count << std::endl;
    std::cout << "语音帧: " << speech_frames << std::endl;
    std::cout << "静音帧: " << silence_frames << std::endl;
    std::cout << "语音比例: " << (speech_frames * 100.0 / frame_count) << "%" << std::endl;
}

// ==================== 测试用例函数 ====================

// 测试1: 基础用法演示 (来自VAD_example.cpp)
void test_basic_usage() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试1: 基础用法演示 (VAD_example功能)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // 1. 创建VAD实例
    std::cout << "步骤1: 创建VAD实例" << std::endl;
    srv::VAD vad;
    
    // 2. 初始化VAD
    std::cout << "步骤2: 初始化VAD (16kHz采样率，160样本/帧 = 10ms)" << std::endl;
    if (!vad.init(16000, 160)) {
        std::cerr << "VAD初始化失败!" << std::endl;
        return;
    }
    
    // 3. 配置参数
    std::cout << "步骤3: 配置VAD参数" << std::endl;
    vad.set_vad_params(80, 80, -15);
    
    // 4. 准备测试音频数据 (简单的正弦波模拟语音)
    std::cout << "步骤4: 生成测试音频 (440Hz正弦波)" << std::endl;
    std::vector<spx_int16_t> audio_frame(160);
    for (int i = 0; i < 160; ++i) {
        double t = static_cast<double>(i) / 16000.0;
        audio_frame[i] = static_cast<spx_int16_t>(8000 * std::sin(2.0 * M_PI * 440.0 * t));
    }
    
    // 5. 进行VAD检测
    std::cout << "步骤5: 进行VAD检测" << std::endl;
    int vad_result = vad.detect_voice_activity(audio_frame);
    int speech_prob = vad.get_speech_probability();
    
    // 6. 输出结果
    std::cout << "步骤6: 输出检测结果" << std::endl;
    std::cout << "VAD检测结果: " << (vad_result == 1 ? "检测到语音" : "检测到静音") << std::endl;
    std::cout << "语音概率: " << speech_prob << "%" << std::endl;
    
    // 7. 测试静音检测
    std::cout << "步骤7: 测试静音检测" << std::endl;
    std::vector<spx_int16_t> silence_frame(160, 0);  // 全零数据
    int silence_result = vad.detect_voice_activity(silence_frame);
    int silence_prob = vad.get_speech_probability();
    
    std::cout << "静音检测结果: " << (silence_result == 1 ? "检测到语音" : "检测到静音") << std::endl;
    std::cout << "静音概率: " << silence_prob << "%" << std::endl;
    
    std::cout << "\n✅ 基础用法演示完成！" << std::endl;
}

// 测试2: 纯静音检测
void test_silence_detection() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试2: 纯静音检测 (验证VAD不会误报)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    srv::VAD vad;
    if (!vad.init(16000, 160)) {
        std::cerr << "VAD初始化失败!" << std::endl;
        return;
    }
    
    vad.set_vad_params(80, 80, -15);
    
    // 生成2秒静音
    auto silence_data = generate_silence(16000, 2000);
    process_audio_frames(silence_data, vad, 160, false);  // 不显示详细过程
    
    std::cout << "✅ 纯静音检测完成！" << std::endl;
}

// 测试3: 纯语音检测
void test_speech_detection() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试3: 纯语音检测 (440Hz正弦波)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    srv::VAD vad;
    if (!vad.init(16000, 160)) {
        std::cerr << "VAD初始化失败!" << std::endl;
        return;
    }
    
    vad.set_vad_params(80, 80, -15);
    
    // 生成2秒语音
    auto speech_data = generate_sine_wave(16000, 2000, 440);
    process_audio_frames(speech_data, vad, 160, false);
    
    std::cout << "✅ 纯语音检测完成！" << std::endl;
}

// 测试4: 噪声环境下的语音检测
void test_noisy_speech_detection() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试4: 噪声环境下的语音检测" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    srv::VAD vad;
    if (!vad.init(16000, 160)) {
        std::cerr << "VAD初始化失败!" << std::endl;
        return;
    }
    
    vad.set_vad_params(80, 80, -15);
    
    // 生成带噪声的语音
    auto noisy_speech_data = generate_noisy_audio(16000, 2000, 440, 2000);
    process_audio_frames(noisy_speech_data, vad, 160, false);
    
    std::cout << "✅ 噪声环境语音检测完成！" << std::endl;
}

// 测试5: 混合音频检测
void test_mixed_audio_detection() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试5: 混合音频检测 (静音-语音-静音)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    srv::VAD vad;
    if (!vad.init(16000, 160)) {
        std::cerr << "VAD初始化失败!" << std::endl;
        return;
    }
    
    vad.set_vad_params(80, 80, -15);
    
    // 构建混合音频：静音-语音-静音
    std::vector<spx_int16_t> mixed_data;
    
    // 添加1秒静音
    auto silence_part = generate_silence(16000, 1000);
    mixed_data.insert(mixed_data.end(), silence_part.begin(), silence_part.end());
    
    // 添加1秒语音
    auto speech_part = generate_sine_wave(16000, 1000, 440);
    mixed_data.insert(mixed_data.end(), speech_part.begin(), speech_part.end());
    
    // 添加1秒静音
    mixed_data.insert(mixed_data.end(), silence_part.begin(), silence_part.end());
    
    process_audio_frames(mixed_data, vad, 160, false);
    
    std::cout << "✅ 混合音频检测完成！" << std::endl;
}

// 测试6: 参数调整对比
void test_parameter_comparison() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试6: 参数调整对比 (学习如何调优)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // 生成测试音频
    auto test_audio = generate_sine_wave(16000, 1000, 440);
    
    // 测试不同参数设置
    std::vector<std::tuple<std::string, int, int, int>> test_configs = {
        {"宽松设置", 70, 70, -10},
        {"默认设置", 80, 80, -15},
        {"严格设置", 90, 90, -20}
    };
    
    for (const auto& config : test_configs) {
        std::string name = std::get<0>(config);
        int prob_start = std::get<1>(config);
        int prob_continue = std::get<2>(config);
        int noise_suppress = std::get<3>(config);
        
        std::cout << "\n--- " << name << " (prob_start=" << prob_start 
                  << ", prob_continue=" << prob_continue 
                  << ", noise_suppress=" << noise_suppress << ") ---" << std::endl;
        
        srv::VAD vad;
        if (!vad.init(16000, 160)) {
            std::cerr << "VAD初始化失败!" << std::endl;
            continue;
        }
        
        vad.set_vad_params(prob_start, prob_continue, noise_suppress);
        process_audio_frames(test_audio, vad, 160, false);
    }
    
    std::cout << "\n✅ 参数调整对比完成！" << std::endl;
}

// 测试7: 实时处理演示
void test_realtime_processing() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试7: 实时处理演示 (逐帧显示)" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    srv::VAD vad;
    if (!vad.init(16000, 160)) {
        std::cerr << "VAD初始化失败!" << std::endl;
        return;
    }
    
    vad.set_vad_params(80, 80, -15);
    
    // 生成短音频进行实时演示
    auto demo_audio = generate_sine_wave(16000, 500, 440);  // 0.5秒
    
    std::cout << "开始实时VAD检测演示 (0.5秒音频，逐帧显示)..." << std::endl;
    std::cout << "按回车键开始..." << std::endl;
    std::cin.get();
    
    process_audio_frames(demo_audio, vad, 160, true);  // 显示详细过程
    
    std::cout << "✅ 实时处理演示完成！" << std::endl;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "🎤 SpeexDSP VAD 静音检测 - 由浅入深学习教程" << std::endl;
    std::cout << "=" << std::string(58, '=') << std::endl;
    
    std::cout << "\n📚 学习路径：" << std::endl;
    std::cout << "1. 基础用法演示 - 学习基本API调用" << std::endl;
    std::cout << "2. 纯静音检测 - 验证不会误报" << std::endl;
    std::cout << "3. 纯语音检测 - 验证检测准确性" << std::endl;
    std::cout << "4. 噪声环境检测 - 测试抗干扰能力" << std::endl;
    std::cout << "5. 混合音频检测 - 模拟真实场景" << std::endl;
    std::cout << "6. 参数调整对比 - 学习如何调优" << std::endl;
    std::cout << "7. 实时处理演示 - 体验实时效果" << std::endl;
    
    std::cout << "\n🚀 开始学习之旅..." << std::endl;
    
    // 执行所有测试用例
    test_basic_usage();
    test_silence_detection();
    test_speech_detection();
    test_noisy_speech_detection();
    test_mixed_audio_detection();
    test_parameter_comparison();
    test_realtime_processing();
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🎉 恭喜！VAD学习教程完成！" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "\n📖 学习总结：" << std::endl;
    std::cout << "✅ 掌握了VAD的基本使用方法" << std::endl;
    std::cout << "✅ 了解了VAD在不同场景下的表现" << std::endl;
    std::cout << "✅ 学会了如何调整VAD参数" << std::endl;
    std::cout << "✅ 体验了实时VAD处理效果" << std::endl;
    
    std::cout << "\n💡 下一步建议：" << std::endl;
    std::cout << "1. 尝试处理真实的音频文件" << std::endl;
    std::cout << "2. 集成到你的音频处理项目中" << std::endl;
    std::cout << "3. 根据实际需求调整VAD参数" << std::endl;
    std::cout << "4. 探索其他SpeexDSP功能（如回声消除、噪声抑制等）" << std::endl;
    
    return 0;
}