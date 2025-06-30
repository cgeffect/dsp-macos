#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <fftw3.h>

// 读取PCM文件
std::vector<short> read_pcm_file_int16(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "❌ 无法打开文件: " << filename << std::endl;
        return {};
    }
    
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    size_t num_samples = file_size / sizeof(short);
    std::vector<short> audio_data(num_samples);
    file.read(reinterpret_cast<char*>(audio_data.data()), file_size);
    
    std::cout << "✅ 成功读取PCM文件: " << filename << std::endl;
    std::cout << "   文件大小: " << file_size << " 字节" << std::endl;
    std::cout << "   样本数量: " << num_samples << std::endl;
    std::cout << "   时长: " << std::fixed << std::setprecision(2) << (num_samples / 48000.0) << " 秒" << std::endl;
    
    return audio_data;
}

// 保存PCM文件
bool save_pcm_file_int16(const std::vector<short>& audio_data, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "❌ 无法创建文件: " << filename << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(audio_data.data()), audio_data.size() * sizeof(short));
    std::cout << "✅ PCM文件已保存: " << filename << std::endl;
    std::cout << "   文件大小: " << (audio_data.size() * sizeof(short)) << " 字节" << std::endl;
    return true;
}

// 计算RMS
double calculate_rms(const std::vector<short>& audio_data) {
    double sum = 0.0;
    for (short sample : audio_data) {
        sum += static_cast<double>(sample) * sample;
    }
    return std::sqrt(sum / audio_data.size());
}

// 计算峰值
short calculate_peak(const std::vector<short>& audio_data) {
    short peak = 0;
    for (short sample : audio_data) {
        peak = std::max(peak, static_cast<short>(std::abs(sample)));
    }
    return peak;
}

class QMPlay2StyleEqualizer {
private:
    int fft_size_;
    int fft_bits_;
    double sample_rate_;
    float preamp_;
    
    fftwf_plan fft_plan_forward_;
    fftwf_plan fft_plan_backward_;
    fftwf_complex* fft_buffer_;
    
    std::vector<float> window_;
    std::vector<float> eq_response_;
    std::vector<float> overlap_buffer_;
    std::vector<float> input_buffer_;
    
public:
    QMPlay2StyleEqualizer() : fft_size_(4096), sample_rate_(48000), preamp_(1.0f) {
        init();
    }
    
    ~QMPlay2StyleEqualizer() {
        cleanup();
    }
    
    void init() {
        // 分配FFT缓冲区
        fft_buffer_ = fftwf_alloc_complex(fft_size_);
        
        // 创建FFT计划
        fft_plan_forward_ = fftwf_plan_dft_1d(fft_size_, fft_buffer_, fft_buffer_, FFTW_FORWARD, FFTW_ESTIMATE);
        fft_plan_backward_ = fftwf_plan_dft_1d(fft_size_, fft_buffer_, fft_buffer_, FFTW_BACKWARD, FFTW_ESTIMATE);
        
        // 初始化窗口函数（Hann窗口）
        window_.resize(fft_size_);
        for (int i = 0; i < fft_size_; ++i) {
            window_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fft_size_ - 1)));
        }
        
        // 初始化EQ响应
        eq_response_.resize(fft_size_ / 2, 1.0f);
        
        // 初始化重叠缓冲区
        overlap_buffer_.resize(fft_size_ / 2, 0.0f);
        
        // 初始化输入缓冲区
        input_buffer_.resize(fft_size_, 0.0f);
    }
    
    void cleanup() {
        if (fft_plan_forward_) fftwf_destroy_plan(fft_plan_forward_);
        if (fft_plan_backward_) fftwf_destroy_plan(fft_plan_backward_);
        if (fft_buffer_) fftwf_free(fft_buffer_);
    }
    
    // QMPlay2风格的余弦插值函数
    static float cosI(float y1, float y2, float p) {
        p = (1.0f - cos(p * M_PI)) / 2.0f;
        return y1 * (1.0f - p) + y2 * p;
    }
    
    // QMPlay2风格的增益计算函数
    static float getAmpl(int val) {
        if (val < 0)
            return 0.0f; // -inf
        if (val == 50)
            return 1.0f;
        if (val > 50)
            return powf(val / 50.0f, 3.33f);  // 指数增长
        return powf(50.0f / (100 - val), 3.33f);  // 指数衰减
    }
    
    // 计算8个频段的频率（QMPlay2风格）
    static std::vector<float> calculateFreqs(int count = 8, int minFreq = 200, int maxFreq = 18000) {
        std::vector<float> freqs(count);
        for (int i = 0; i < count; ++i) {
            freqs[i] = minFreq * std::pow(static_cast<float>(maxFreq) / minFreq, static_cast<float>(i) / (count - 1));
        }
        return freqs;
    }
    
    // 设置EQ滑块值（QMPlay2风格，0-100，50为中性）
    void setEQSliders(const std::vector<int>& slider_values) {
        // 计算频率
        auto freqs = calculateFreqs(slider_values.size());
        
        // 清空响应
        std::fill(eq_response_.begin(), eq_response_.end(), 1.0f);
        
        // 计算每个FFT bin的增益（使用QMPlay2的余弦插值）
        std::vector<float> gains(fft_size_ / 2);
        const int maxHz = sample_rate_ / 2;
        
        for (int i = 0; i < fft_size_ / 2; ++i) {
            const float freq = (i + 1) * maxHz / (fft_size_ / 2);
            
            // 找到对应的EQ频段
            int band = 0;
            for (int j = 0; j < freqs.size(); ++j) {
                if (freq >= freqs[j]) {
                    band = j;
                }
            }
            
            // 使用QMPlay2的余弦插值
            if (band + 1 < freqs.size()) {
                float p = (freq - freqs[band]) / (freqs[band + 1] - freqs[band]);
                gains[i] = cosI(getAmpl(slider_values[band]), getAmpl(slider_values[band + 1]), p);
            } else {
                gains[i] = getAmpl(slider_values.back());
            }
        }
        
        // 应用增益
        for (int i = 0; i < fft_size_ / 2; ++i) {
            eq_response_[i] = gains[i] * preamp_;
        }
    }
    
    // 设置预放大
    void setPreamp(float preamp) {
        preamp_ = preamp;
    }
    
    // 处理音频
    std::vector<short> processAudio(const std::vector<short>& input) {
        std::vector<short> output;
        output.reserve(input.size());
        
        int hop_size = fft_size_ / 2;
        
        for (size_t i = 0; i < input.size(); i += hop_size) {
            // 填充输入缓冲区
            for (int j = 0; j < fft_size_; ++j) {
                if (i + j < input.size()) {
                    input_buffer_[j] = static_cast<float>(input[i + j]) / 32767.0f;
                } else {
                    input_buffer_[j] = 0.0f;
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
    
    int getFFTSize() const {
        return fft_size_;
    }
};

// 创建QMPlay2风格的EQ预设
std::vector<int> createQMPlay2Preset(const std::string& preset_name) {
    std::vector<int> sliders(8, 50); // 默认所有滑块为50（中性）
    
    if (preset_name == "flat") {
        // 平坦响应 - 所有滑块为50
        sliders = {50, 50, 50, 50, 50, 50, 50, 50};
    } else if (preset_name == "bass_boost") {
        // 低频增强 - 类似QMPlay2的低音增强
        sliders = {65, 60, 50, 50, 50, 50, 50, 50};
    } else if (preset_name == "treble_boost") {
        // 高频增强 - 类似QMPlay2的高音增强
        sliders = {50, 50, 50, 50, 50, 50, 60, 65};
    } else if (preset_name == "vocal_boost") {
        // 人声增强 - 类似QMPlay2的人声增强
        sliders = {45, 50, 60, 65, 60, 50, 50, 50};
    } else if (preset_name == "rock") {
        // 摇滚音色 - 类似QMPlay2的摇滚预设
        sliders = {60, 50, 40, 50, 60, 70, 60, 50};
    } else if (preset_name == "jazz") {
        // 爵士音色 - 类似QMPlay2的爵士预设
        sliders = {55, 60, 65, 60, 55, 50, 45, 40};
    } else if (preset_name == "classical") {
        // 古典音色 - 类似QMPlay2的古典预设
        sliders = {45, 50, 55, 60, 65, 60, 55, 50};
    } else if (preset_name == "pop") {
        // 流行音色 - 类似QMPlay2的流行预设
        sliders = {55, 60, 65, 70, 65, 60, 55, 50};
    } else if (preset_name == "max_bass") {
        // 最大低音增强 - 200Hz设为100，其他为50
        sliders = {100, 50, 50, 50, 50, 50, 50, 50};
    } else if (preset_name == "min_bass") {
        // 最小低音 - 200Hz设为0，其他为50
        sliders = {0, 50, 50, 50, 50, 50, 50, 50};
    } else {
        // 默认平坦
        sliders = {100, 0, 0, 0, 0, 0, 0, 0};
    }
    
    return sliders;
}

int main() {
    std::cout << "=== QMPlay2风格EQ对比测试 ===" << std::endl;
    std::cout << "测试与QMPlay2相似的效果" << std::endl;
    
    // 配置参数（与QMPlay2一致）
    double sample_rate = 48000.0;
    int fft_bits = 10;  // 1024点FFT
    
    std::cout << "配置参数（QMPlay2风格）:" << std::endl;
    std::cout << "采样率: " << sample_rate << " Hz" << std::endl;
    std::cout << "FFT大小: " << (1 << fft_bits) << " (4096 for better low frequency resolution)" << std::endl;
    std::cout << "滑块范围: 0-100，50为中性值" << std::endl;
    std::cout << std::endl;
    
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
    QMPlay2StyleEqualizer eq;
    
    // 定义QMPlay2风格的测试预设
    std::vector<std::string> presets = {"flat", "bass_boost", "treble_boost", "vocal_boost", "rock", "jazz", "classical", "pop", "max_bass", "min_bass"};
    std::vector<std::string> preset_names = {"平坦", "低频增强", "高频增强", "人声增强", "摇滚", "爵士", "古典", "流行", "最大低音", "最小低音"};
    
    // 执行测试
    for (size_t i = 0; i < presets.size(); ++i) {
        std::cout << "\n步骤3." << (i+1) << ": " << preset_names[i] << "测试..." << std::endl;
        
        // 设置EQ配置（QMPlay2风格）
        auto sliders = createQMPlay2Preset(presets[i]);
        eq.setEQSliders(sliders);
        
        // 计算QMPlay2风格的自动预放大
        int maxSliderValue = 0;
        for (int val : sliders) {
            maxSliderValue = std::max(val, maxSliderValue);
        }
        
        // QMPlay2的自动预放大逻辑：如果最大滑块值>50，则预放大为100-maxSliderValue
        float preamp = 1.0f;
        if (maxSliderValue > 50) {
            int preampSlider = 100 - maxSliderValue;
            preamp = QMPlay2StyleEqualizer::getAmpl(preampSlider);
            printf("自动预放大: 滑块值=%d, 增益=%.2fdB\n", preampSlider, 20 * log10(preamp));
        }
        
        eq.setPreamp(preamp);
        
        std::cout << "EQ配置（QMPlay2滑块值）:" << std::endl;
        auto freqs = QMPlay2StyleEqualizer::calculateFreqs(sliders.size());
        for (size_t j = 0; j < sliders.size(); ++j) {
            float gain_db = 20.0 * std::log10(QMPlay2StyleEqualizer::getAmpl(sliders[j]));
            std::cout << "  频率: " << std::setw(5) << freqs[j] << "Hz, "
                      << "滑块: " << std::setw(2) << sliders[j] << "/100, "
                      << "增益: " << std::setw(6) << std::fixed << std::setprecision(2) << gain_db << "dB" << std::endl;
        }
        
        // 应用EQ处理
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
        
        // 保存处理后的文件
        std::string output_filename = "qmplay2_style_" + presets[i] + ".pcm";
        save_pcm_file_int16(processed_audio, output_filename);
    }
    
    // 总结
    std::cout << "\n=== 测试完成 ===" << std::endl;
    std::cout << "生成的文件（QMPlay2风格）:" << std::endl;
    for (size_t i = 0; i < presets.size(); ++i) {
        std::cout << "  qmplay2_style_" << presets[i] << ".pcm - " << preset_names[i] << std::endl;
    }
    
    std::cout << "\n播放命令（对比QMPlay2效果）:" << std::endl;
    std::cout << "  原始音频: ffplay -f s16le -ar 48000 -nodisp -autoexit res/48000_1_s16le.pcm" << std::endl;
    for (size_t i = 0; i < presets.size(); ++i) {
        std::cout << "  " << preset_names[i] << ": ffplay -f s16le -ar 48000 -nodisp -autoexit qmplay2_style_" << presets[i] << ".pcm" << std::endl;
    }
    
    std::cout << "\nQMPlay2风格说明:" << std::endl;
    std::cout << "  - 使用QMPlay2的滑块值范围（0-100，50为中性）" << std::endl;
    std::cout << "  - 使用QMPlay2的getAmpl函数计算增益" << std::endl;
    std::cout << "  - 预设值参考QMPlay2的实际设置" << std::endl;
    std::cout << "  - 技术参数与QMPlay2完全一致" << std::endl;
    
    return 0;
} 