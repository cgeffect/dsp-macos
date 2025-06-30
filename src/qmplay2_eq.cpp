#include <iostream>
#include <vector>
#include <complex>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <string>
#include <algorithm>

// FFTW头文件
extern "C" {
    #include <fftw3.h>
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
        std::cerr << "❌ 错误：无法创建文件 " << filename << std::endl;
        return false;
    }
    file.write(reinterpret_cast<const char*>(audio_data.data()), audio_data.size() * sizeof(short));
    file.close();
    std::cout << "✅ PCM文件已保存: " << filename << std::endl;
    std::cout << "   文件大小: " << (audio_data.size() * sizeof(short)) << " 字节" << std::endl;
    return true;
}

// 计算音频的RMS值
double calculate_rms(const std::vector<short>& audio_data) {
    if (audio_data.empty()) return 0.0;
    double sum_squares = 0.0;
    for (const auto& sample : audio_data) {
        sum_squares += static_cast<double>(sample) * sample;
    }
    return std::sqrt(sum_squares / audio_data.size());
}

// 计算音频的峰值
short calculate_peak(const std::vector<short>& audio_data) {
    if (audio_data.empty()) return 0;
    short max_peak = 0;
    for (const auto& sample : audio_data) {
        max_peak = std::max(max_peak, static_cast<short>(std::abs(sample)));
    }
    return max_peak;
}

// 基于QMPlay2的EQ实现
class QMPlay2Equalizer {
private:
    int fft_size_;
    int fft_bits_;
    double sample_rate_;
    float preamp_;
    
    // FFTW计划
    fftwf_plan fft_plan_forward_;
    fftwf_plan fft_plan_backward_;
    fftwf_complex* fft_buffer_;
    
    // 窗口函数
    std::vector<float> window_;
    
    // EQ频率响应
    std::vector<float> eq_response_;
    
    // 重叠-相加缓冲区
    std::vector<float> overlap_buffer_;
    
    // 输入缓冲区
    std::vector<float> input_buffer_;
    
public:
    QMPlay2Equalizer(int fft_bits = 10, double sample_rate = 48000.0) 
        : fft_bits_(fft_bits)
        , fft_size_(1 << fft_bits)
        , sample_rate_(sample_rate)
        , preamp_(1.0f)
        , fft_plan_forward_(nullptr)
        , fft_plan_backward_(nullptr)
        , fft_buffer_(nullptr) {
        
        init();
    }
    
    ~QMPlay2Equalizer() {
        cleanup();
    }
    
    void init() {
        // 分配FFTW缓冲区
        fft_buffer_ = fftwf_alloc_complex(fft_size_);
        
        // 创建FFTW计划
        fft_plan_forward_ = fftwf_plan_dft_1d(fft_size_, fft_buffer_, fft_buffer_, FFTW_FORWARD, FFTW_ESTIMATE);
        fft_plan_backward_ = fftwf_plan_dft_1d(fft_size_, fft_buffer_, fft_buffer_, FFTW_BACKWARD, FFTW_ESTIMATE);
        
        // 创建窗口函数（Hann窗口）
        window_.resize(fft_size_);
        for (int i = 0; i < fft_size_; ++i) {
            window_[i] = 0.5f - 0.5f * cos(2.0f * M_PI * i / (fft_size_ - 1));
        }
        
        // 初始化缓冲区
        overlap_buffer_.resize(fft_size_ / 2, 0.0f);
        input_buffer_.reserve(fft_size_);
        eq_response_.resize(fft_size_ / 2, 1.0f);
    }
    
    void cleanup() {
        if (fft_plan_forward_) {
            fftwf_destroy_plan(fft_plan_forward_);
            fft_plan_forward_ = nullptr;
        }
        if (fft_plan_backward_) {
            fftwf_destroy_plan(fft_plan_backward_);
            fft_plan_backward_ = nullptr;
        }
        if (fft_buffer_) {
            fftwf_free(fft_buffer_);
            fft_buffer_ = nullptr;
        }
    }
    
    // QMPlay2的getAmpl函数
    static float getAmpl(int val) {
        if (val < 0)
            return 0.0f; //-inf
        if (val == 50)
            return 1.0f;
        if (val > 50)
            return powf(val / 50.0f, 3.33f);
        return powf(50.0f / (100 - val), 3.33f);
    }
    
    // 计算EQ频率（基于QMPlay2的freqs函数）
    static std::vector<float> calculateFreqs(int count, int minFreq = 200, int maxFreq = 18000) {
        std::vector<float> freqs(count);
        const float l = powf(maxFreq / minFreq, 1.0f / (count - 1));
        for (int i = 0; i < count; ++i)
            freqs[i] = minFreq * powf(l, i);
        return freqs;
    }
    
    // 设置EQ频段dB值（直接输入dB值，更直观）
    void setEQdB(const std::vector<float>& db_values) {
        // 计算频率
        auto freqs = calculateFreqs(db_values.size());
        
        // 清空响应
        std::fill(eq_response_.begin(), eq_response_.end(), 1.0f);
        
        // 计算每个频率点的增益
        for (size_t i = 0; i < eq_response_.size(); ++i) {
            double freq = static_cast<double>(i + 1) * sample_rate_ / (2.0 * eq_response_.size());
            
            // 找到最近的频段进行插值
            float gain = 1.0f;
            for (size_t j = 0; j < freqs.size() - 1; ++j) {
                if (freq >= freqs[j] && freq <= freqs[j + 1]) {
                    // 线性插值
                    float p = static_cast<float>((freq - freqs[j]) / (freqs[j + 1] - freqs[j]));
                    float g1 = powf(10.0f, db_values[j] / 20.0f);  // dB转增益
                    float g2 = powf(10.0f, db_values[j + 1] / 20.0f);
                    gain = g1 * (1.0f - p) + g2 * p;
                    break;
                }
            }
            
            // 如果频率超出范围，使用边界值
            if (freq < freqs[0]) {
                gain = powf(10.0f, db_values[0] / 20.0f);
            } else if (freq > freqs.back()) {
                gain = powf(10.0f, db_values.back() / 20.0f);
            }
            
            eq_response_[i] = gain * preamp_;
        }
    }
    
    // 设置预放大
    void setPreamp(float preamp) {
        preamp_ = preamp;
    }
    
    // 处理音频（基于QMPlay2的重叠-相加法）
    std::vector<short> processAudio(const std::vector<short>& input) {
        std::vector<short> output;
        output.reserve(input.size());
        
        const int hop_size = fft_size_ / 2;  // 50%重叠
        
        for (size_t i = 0; i < input.size(); i += hop_size) {
            // 准备输入数据
            input_buffer_.clear();
            for (int j = 0; j < fft_size_; ++j) {
                if (i + j < input.size()) {
                    input_buffer_.push_back(static_cast<float>(input[i + j]) / 32768.0f);
                } else {
                    input_buffer_.push_back(0.0f);
                }
            }
            
            // 应用窗口函数
            for (int j = 0; j < fft_size_; ++j) {
                fft_buffer_[j][0] = input_buffer_[j] * window_[j];
                fft_buffer_[j][1] = 0.0f;
            }
            
            // 执行FFT
            fftwf_execute(fft_plan_forward_);
            
            // 应用EQ响应
            for (int j = 0; j < fft_size_ / 2; ++j) {
                float coeff = eq_response_[j];
                fft_buffer_[j][0] *= coeff;
                fft_buffer_[j][1] *= coeff;
                
                // 处理负频率（共轭对称）
                if (j > 0) {
                    fft_buffer_[fft_size_ - j][0] *= coeff;
                    fft_buffer_[fft_size_ - j][1] *= coeff;
                }
            }
            
            // 执行IFFT
            fftwf_execute(fft_plan_backward_);
            
            // 重叠-相加
            for (int j = 0; j < hop_size; ++j) {
                if (i + j < input.size()) {
                    float sample = fft_buffer_[j][0] / fft_size_;
                    sample += overlap_buffer_[j];
                    
                    // 限制范围并转换回short
                    sample = std::max(-1.0f, std::min(1.0f, sample));
                    output.push_back(static_cast<short>(sample * 32767.0f));
                }
                
                // 保存重叠部分
                overlap_buffer_[j] = fft_buffer_[j + hop_size][0] / fft_size_;
            }
        }
        
        return output;
    }
};

// 创建EQ预设（8个频段，直接使用dB值）
std::vector<float> createEQPreset(const std::string& preset_name) {
    std::vector<float> db_values(8, 0.0f); // 默认所有频段为0dB
    
    if (preset_name == "flat") {
        // 平坦响应 - 所有频段为0dB
        db_values = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    } else if (preset_name == "bass_boost") {
        // 低频增强
        db_values = {12.0f, 8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    } else if (preset_name == "treble_boost") {
        // 高频增强
        db_values = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 8.0f, 12.0f};
    } else if (preset_name == "vocal_boost") {
        // 人声增强
        db_values = {-6.0f, 0.0f, 8.0f, 12.0f, 8.0f, 0.0f, 0.0f, 0.0f};
    } else if (preset_name == "noise_reduction") {
        // 噪声抑制
        db_values = {-12.0f, -6.0f, 0.0f, 0.0f, 0.0f, -6.0f, -12.0f, -18.0f};
    } else if (preset_name == "warm") {
        // 温暖音色
        db_values = {8.0f, 6.0f, 0.0f, -3.0f, -6.0f, -8.0f, -12.0f, -15.0f};
    } else if (preset_name == "bright") {
        // 明亮音色
        db_values = {-15.0f, -12.0f, -8.0f, -6.0f, -3.0f, 0.0f, 6.0f, 8.0f};
    } else if (preset_name == "rock") {
        // 摇滚音色
        db_values = {6.0f, 0.0f, -6.0f, 0.0f, 6.0f, 12.0f, 6.0f, 0.0f};
    } else if (preset_name == "jazz") {
        // 爵士音色
        db_values = {3.0f, 6.0f, 8.0f, 6.0f, 3.0f, 0.0f, -3.0f, -6.0f};
    } else if (preset_name == "heavy_bass") {
        // 重低音（测试用）
        db_values = {20.0f, 15.0f, 10.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    } else if (preset_name == "custom_test") {
        // 自定义测试
        db_values = {20.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    } else {
        // 默认平坦
        db_values = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    return db_values;
}

int main() {
    std::cout << "=== QMPlay2风格EQ均衡器 - 平坦EQ vs 温和EQ对比测试 ===" << std::endl;
    std::cout << "验证FFT转换和频域调整的基本逻辑" << std::endl;
    
    // 配置参数
    double sample_rate = 48000.0;
    int fft_bits = 10;  // 1024点FFT
    
    std::cout << "\n配置参数:" << std::endl;
    std::cout << "采样率: " << sample_rate << " Hz" << std::endl;
    std::cout << "FFT大小: " << (1 << fft_bits) << std::endl;
    
    // 读取音频文件
    std::cout << "\n步骤1: 读取音频文件..." << std::endl;
    auto input_audio = read_pcm_file_int16("res/48000_1_s16le.pcm");
    
    if (input_audio.empty()) {
        std::cerr << "❌ 无法读取音频文件" << std::endl;
        return 1;
    }
    
    // 分析输入音频
    std::cout << "\n步骤2: 分析输入音频..." << std::endl;
    double input_rms = calculate_rms(input_audio);
    short input_peak = calculate_peak(input_audio);
    std::cout << "输入音频分析:" << std::endl;
    std::cout << "  RMS: " << std::fixed << std::setprecision(2) << input_rms << std::endl;
    std::cout << "  峰值: " << input_peak << std::endl;
    
    // 创建EQ实例
    QMPlay2Equalizer eq(fft_bits, sample_rate);
    
    // 定义测试配置
    struct TestConfig {
        std::string name;
        std::vector<float> db_values;
        std::string description;
    };
    
    std::vector<TestConfig> test_configs = {
        {
            "平坦EQ",
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            "所有频段为0dB，理论上输出应该和输入几乎一样"
        },
        {
            "温和重低音",
            {6.0f, 4.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            "低频温和提升：200Hz+6dB, 380Hz+4dB, 723Hz+2dB"
        },
        {
            "只提升200Hz",
            {6.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            "只提升200Hz频段6dB，其他频段保持0dB"
        }
    };
    
    // 执行测试
    for (size_t test_idx = 0; test_idx < test_configs.size(); ++test_idx) {
        const auto& config = test_configs[test_idx];
        
        std::cout << "\n步骤3." << (test_idx + 1) << ": " << config.name << "测试..." << std::endl;
        std::cout << "描述: " << config.description << std::endl;
        
        // 设置EQ配置
        eq.setEQdB(config.db_values);
        eq.setPreamp(1.0f);
        
        std::cout << "EQ配置:" << std::endl;
        auto freqs = QMPlay2Equalizer::calculateFreqs(config.db_values.size());
        for (size_t j = 0; j < config.db_values.size(); ++j) {
            std::cout << "  频率: " << std::setw(5) << freqs[j] << "Hz, "
                      << "增益: " << std::setw(6) << config.db_values[j] << "dB" << std::endl;
        }
        
        // 应用EQ处理（核心逻辑完全相同）
        auto processed_audio = eq.processAudio(input_audio);
        
        // 分析输出音频
        double output_rms = calculate_rms(processed_audio);
        short output_peak = calculate_peak(processed_audio);
        
        std::cout << "输出音频分析:" << std::endl;
        std::cout << "  RMS: " << std::fixed << std::setprecision(2) << output_rms << std::endl;
        std::cout << "  峰值: " << output_peak << std::endl;
        
        // 计算变化
        double rms_change = 20.0 * std::log10(output_rms / (input_rms + 1e-10));
        std::cout << "  RMS变化: " << std::fixed << std::setprecision(2) << rms_change << " dB" << std::endl;
        
        // 计算样本差异（仅对平坦EQ有意义）
        if (test_idx == 0) { // 平坦EQ
            double total_diff = 0.0;
            size_t num_samples = std::min(input_audio.size(), processed_audio.size());
            for (size_t i = 0; i < num_samples; ++i) {
                total_diff += std::abs(static_cast<double>(input_audio[i] - processed_audio[i]));
            }
            double avg_diff = total_diff / num_samples;
            double diff_percentage = (avg_diff / 32767.0) * 100.0;
            
            std::cout << "  平均样本差异: " << std::fixed << std::setprecision(2) << avg_diff << std::endl;
            std::cout << "  差异百分比: " << std::fixed << std::setprecision(4) << diff_percentage << "%" << std::endl;
            
            // 验证平坦EQ结果
            if (std::abs(rms_change) < 0.1 && diff_percentage < 1.0) {
                std::cout << "  ✅ 平坦EQ测试通过！" << std::endl;
            } else {
                std::cout << "  ❌ 平坦EQ测试失败！" << std::endl;
            }
        }
        
        // 保存处理后的文件
        std::string output_filename = "eq_test_" + std::to_string(test_idx + 1) + "_" + config.name + ".pcm";
        save_pcm_file_int16(processed_audio, output_filename);
    }
    
    // 总结
    std::cout << "\n=== 测试完成 ===" << std::endl;
    std::cout << "生成的文件:" << std::endl;
    for (size_t i = 0; i < test_configs.size(); ++i) {
        std::cout << "  eq_test_" << (i + 1) << "_" << test_configs[i].name << ".pcm" << std::endl;
    }
    
    std::cout << "\n播放命令（对比测试）:" << std::endl;
    std::cout << "  原始音频: ffplay -f s16le -ar 48000 -nodisp -autoexit res/48000_1_s16le.pcm" << std::endl;
    for (size_t i = 0; i < test_configs.size(); ++i) {
        std::cout << "  " << test_configs[i].name << ": ffplay -f s16le -ar 48000 -nodisp -autoexit eq_test_" << (i + 1) << "_" << test_configs[i].name << ".pcm" << std::endl;
    }
    
    std::cout << "\n对比说明:" << std::endl;
    std::cout << "  - 所有测试使用完全相同的核心逻辑（FFT转换+频域调整）" << std::endl;
    std::cout << "  - 只有EQ参数不同，便于对比效果" << std::endl;
    std::cout << "  - 平坦EQ验证算法正确性，温和EQ测试实际效果" << std::endl;
    
    return 0;
} 