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
#include "util/VAD.h"

// ==================== PCM格式验证工具 ====================

// 验证PCM数据格式
bool validate_pcm_format(const std::vector<spx_int16_t>& audio_data, int sample_rate) {
    // 检查数据类型大小
    if (sizeof(spx_int16_t) != 2) {
        std::cerr << "错误：spx_int16_t不是16位" << std::endl;
        return false;
    }
    
    // 检查采样率
    if (sample_rate != 16000 && sample_rate != 8000) {
        std::cout << "警告：非推荐采样率 " << sample_rate << " Hz" << std::endl;
        std::cout << "推荐使用16kHz或8kHz以获得最佳效果" << std::endl;
    }
    
    // 检查数据范围
    for (spx_int16_t sample : audio_data) {
        if (sample < -32768 || sample > 32767) {
            std::cerr << "错误：样本值超出16位范围: " << sample << std::endl;
            return false;
        }
    }
    
    std::cout << "✅ PCM格式验证通过" << std::endl;
    std::cout << "   - 数据类型: 16位有符号整数" << std::endl;
    std::cout << "   - 采样率: " << sample_rate << " Hz" << std::endl;
    std::cout << "   - 样本数: " << audio_data.size() << std::endl;
    std::cout << "   - 时长: " << (audio_data.size() * 1000.0 / sample_rate) << " ms" << std::endl;
    
    return true;
}

// ==================== 音频文件读取工具 ====================

// 读取PCM音频文件
bool read_pcm_file(const std::string& filename, std::vector<spx_int16_t>& audio_data, int& sample_rate) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 检查文件大小是否为偶数（16位 = 2字节）
    if (file_size % 2 != 0) {
        std::cerr << "错误：文件大小不是偶数，可能不是16位PCM文件" << std::endl;
        return false;
    }
    
    // 计算样本数量
    size_t num_samples = file_size / sizeof(spx_int16_t);
    
    // 读取音频数据
    audio_data.resize(num_samples);
    file.read(reinterpret_cast<char*>(audio_data.data()), file_size);
    
    if (file.fail()) {
        std::cerr << "读取文件失败: " << filename << std::endl;
        return false;
    }
    
    file.close();
    
    std::cout << "📁 成功读取PCM文件: " << filename << std::endl;
    std::cout << "   - 文件大小: " << file_size << " 字节" << std::endl;
    std::cout << "   - 样本数量: " << num_samples << std::endl;
    
    return true;
}

// 保存PCM音频文件
bool save_pcm_file(const std::string& filename, const std::vector<spx_int16_t>& audio_data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法创建文件: " << filename << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(audio_data.data()), 
               audio_data.size() * sizeof(spx_int16_t));
    
    if (file.fail()) {
        std::cerr << "写入文件失败: " << filename << std::endl;
        return false;
    }
    
    file.close();
    std::cout << "💾 成功保存PCM文件: " << filename << std::endl;
    return true;
}

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
void process_audio_frames(const std::vector<spx_int16_t>& audio_data, srv::VAD& vad, int frame_size, 
                         bool show_details = true, const std::string& audio_name = "音频") {
    if (show_details) {
        std::cout << "\n=== 开始VAD检测: " << audio_name << " ===" << std::endl;
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
    
    std::cout << "\n=== VAD检测结果: " << audio_name << " ===" << std::endl;
    std::cout << "总帧数: " << frame_count << std::endl;
    std::cout << "语音帧: " << speech_frames << std::endl;
    std::cout << "静音帧: " << silence_frames << std::endl;
    std::cout << "语音比例: " << (speech_frames * 100.0 / frame_count) << "%" << std::endl;
    std::cout << "总时长: " << (frame_count * frame_size * 1000.0 / vad.get_sample_rate()) << " ms" << std::endl;
}

// ==================== 测试用例函数 ====================

// 测试1: PCM格式验证
void test_pcm_format_validation() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试1: PCM格式验证" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    int sample_rate = 16000;
    
    // 生成测试数据
    auto test_audio = generate_sine_wave(sample_rate, 1000, 440);
    
    // 验证格式
    if (validate_pcm_format(test_audio, sample_rate)) {
        std::cout << "✅ PCM格式验证测试通过" << std::endl;
    } else {
        std::cout << "❌ PCM格式验证测试失败" << std::endl;
    }
}

// 测试2: 真实PCM文件检测
void test_real_pcm_detection() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试2: 真实PCM文件检测" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // 检查是否有PCM文件 ffplay -f s16le -ar 16000 -ac 1 16000_1_16_speech.pcm
    std::vector<std::string> pcm_files = {
        "16000_1_16_speech.pcm",
        "16000_1_16_silence.pcm",
        "16000_1_16_mixed.pcm",
        "audio_sample.pcm"
    };
    
    bool found_pcm_file = false;
    std::string selected_file;
    
    for (const auto& file : pcm_files) {
        std::ifstream test_file(file);
        if (test_file.good()) {
            selected_file = file;
            found_pcm_file = true;
            break;
        }
    }
    
    if (!found_pcm_file) {
        std::cout << "⚠️  未找到PCM文件，将生成测试文件..." << std::endl;
        
        // 生成一些测试PCM文件
        int sample_rate = 16000;
        
        // 生成语音文件
        auto speech_data = generate_sine_wave(sample_rate, 3000, 440);  // 3秒语音
        save_pcm_file("16000_1_16_speech.pcm", speech_data);
        
        // 生成静音文件
        auto silence_data = generate_silence(sample_rate, 2000);  // 2秒静音
        save_pcm_file("16000_1_16_silence.pcm", silence_data);
        
        // 生成混合文件
        std::vector<spx_int16_t> mixed_data;
        mixed_data.insert(mixed_data.end(), silence_data.begin(), silence_data.end());
        mixed_data.insert(mixed_data.end(), speech_data.begin(), speech_data.end());
        mixed_data.insert(mixed_data.end(), silence_data.begin(), silence_data.end());
        save_pcm_file("16000_1_16_mixed.pcm", mixed_data);
        
        selected_file = "16000_1_16_speech.pcm";
        found_pcm_file = true;
        
        std::cout << "✅ 已生成测试PCM文件" << std::endl;
    }
    
    if (found_pcm_file) {
        std::cout << "使用PCM文件: " << selected_file << std::endl;
        
        srv::VAD vad;
        if (!vad.init(16000, 160)) {
            std::cerr << "VAD初始化失败!" << std::endl;
            return;
        }
        
        vad.set_vad_params(80, 80, -15);
        
        // 读取PCM文件
        std::vector<spx_int16_t> audio_data;
        int sample_rate = 16000;  // 假设16kHz
        if (read_pcm_file(selected_file, audio_data, sample_rate)) {
            // 验证PCM格式
            if (validate_pcm_format(audio_data, sample_rate)) {
                process_audio_frames(audio_data, vad, 160, false, selected_file);
            }
        }
    }
    
    std::cout << "\n✅ 真实PCM文件检测完成！" << std::endl;
}

// 测试3: 不同采样率测试
void test_different_sample_rates() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试3: 不同采样率测试" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::vector<int> sample_rates = {8000, 16000, 22050, 44100};
    
    for (int sample_rate : sample_rates) {
        std::cout << "\n--- 测试采样率: " << sample_rate << " Hz ---" << std::endl;
        
        // 计算合适的帧大小
        int frame_size = (sample_rate * 10) / 1000;  // 10ms帧
        
        srv::VAD vad;
        if (!vad.init(sample_rate, frame_size)) {
            std::cerr << "VAD初始化失败 (采样率: " << sample_rate << ")" << std::endl;
            continue;
        }
        
        vad.set_vad_params(80, 80, -15);
        
        // 生成测试音频
        auto test_audio = generate_sine_wave(sample_rate, 1000, 440);
        
        // 验证格式
        if (validate_pcm_format(test_audio, sample_rate)) {
            process_audio_frames(test_audio, vad, frame_size, false, 
                               "采样率" + std::to_string(sample_rate) + "Hz");
        }
    }
    
    std::cout << "\n✅ 不同采样率测试完成！" << std::endl;
}

// 测试4: 批量PCM文件处理
void test_batch_pcm_processing() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试4: 批量PCM文件处理" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::vector<std::string> pcm_files = {
        "16000_1_16_speech.pcm",
        "16000_1_16_silence.pcm",
        "16000_1_16_mixed.pcm"
    };
    
    srv::VAD vad;
    if (!vad.init(16000, 160)) {
        std::cerr << "VAD初始化失败!" << std::endl;
        return;
    }
    
    vad.set_vad_params(80, 80, -15);
    
    for (const auto& filename : pcm_files) {
        std::ifstream test_file(filename);
        if (test_file.good()) {
            test_file.close();
            
            std::vector<spx_int16_t> audio_data;
            int sample_rate = 16000;
            
            if (read_pcm_file(filename, audio_data, sample_rate)) {
                if (validate_pcm_format(audio_data, sample_rate)) {
                    process_audio_frames(audio_data, vad, 160, false, filename);
                }
            }
        }
    }
    
    std::cout << "\n✅ 批量PCM文件处理完成！" << std::endl;
}

// 测试5: 实时PCM流处理模拟
void test_realtime_pcm_stream() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试5: 实时PCM流处理模拟" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "模拟实时音频流处理..." << std::endl;
    std::cout << "按回车键开始实时演示..." << std::endl;
    std::cin.get();
    
    srv::VAD vad;
    if (!vad.init(16000, 160)) {
        std::cerr << "VAD初始化失败!" << std::endl;
        return;
    }
    
    vad.set_vad_params(80, 80, -15);
    
    // 生成短音频进行实时演示
    auto demo_audio = generate_sine_wave(16000, 1000, 440);  // 1秒
    
    std::cout << "开始实时VAD检测 (1秒音频，逐帧显示)..." << std::endl;
    process_audio_frames(demo_audio, vad, 160, true, "实时流");
    
    std::cout << "\n✅ 实时PCM流处理模拟完成！" << std::endl;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "🎤 SpeexDSP VAD 静音检测 - PCM格式处理" << std::endl;
    std::cout << "=" << std::string(58, '=') << std::endl;
    
    std::cout << "\n📚 学习路径：" << std::endl;
    std::cout << "1. PCM格式验证 - 验证音频数据格式" << std::endl;
    std::cout << "2. 真实PCM文件检测 - 处理真实音频文件" << std::endl;
    std::cout << "3. 不同采样率测试 - 测试各种采样率" << std::endl;
    std::cout << "4. 批量PCM文件处理 - 批量处理多个文件" << std::endl;
    std::cout << "5. 实时PCM流处理模拟 - 模拟实时应用" << std::endl;
    
    std::cout << "\n💡 PCM格式要求：" << std::endl;
    std::cout << "- 数据类型: 16位有符号整数 (spx_int16_t)" << std::endl;
    std::cout << "- 采样率: 推荐16kHz，支持8kHz" << std::endl;
    std::cout << "- 声道: 单声道" << std::endl;
    std::cout << "- 帧大小: 推荐160样本 (10ms@16kHz)" << std::endl;
    
    std::cout << "\n🔧 文件转换命令：" << std::endl;
    std::cout << "ffmpeg -i input.wav -f s16le -acodec pcm_s16le -ar 16000 -ac 1 output.pcm" << std::endl;
    
    std::cout << "\n🚀 开始PCM格式处理..." << std::endl;
    
    // 执行所有测试用例
    test_pcm_format_validation();
    test_real_pcm_detection();
    test_different_sample_rates();
    test_batch_pcm_processing();
    test_realtime_pcm_stream();
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🎉 恭喜！PCM格式处理完成！" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "\n📖 学习总结：" << std::endl;
    std::cout << "✅ 掌握了PCM格式要求和验证方法" << std::endl;
    std::cout << "✅ 学会了处理真实PCM音频文件" << std::endl;
    std::cout << "✅ 了解了不同采样率的处理方法" << std::endl;
    std::cout << "✅ 体验了批量文件处理和实时流处理" << std::endl;
    
    std::cout << "\n💡 下一步建议：" << std::endl;
    std::cout << "1. 使用真实的录音文件进行测试" << std::endl;
    std::cout << "2. 集成到音频录制应用中" << std::endl;
    std::cout << "3. 根据实际环境调整VAD参数" << std::endl;
    std::cout << "4. 结合其他音频处理功能（降噪、回声消除等）" << std::endl;
    
    return 0;
} 