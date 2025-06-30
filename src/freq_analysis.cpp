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

class FrequencyAnalyzer {
private:
    int fft_size_;
    double sample_rate_;
    fftwf_plan fft_plan_;
    fftwf_complex* fft_buffer_;
    std::vector<float> window_;
    
public:
    FrequencyAnalyzer(int fft_size = 1024, double sample_rate = 48000.0) 
        : fft_size_(fft_size)
        , sample_rate_(sample_rate)
        , fft_plan_(nullptr)
        , fft_buffer_(nullptr) {
        init();
    }
    
    ~FrequencyAnalyzer() {
        cleanup();
    }
    
    void init() {
        // 分配FFT缓冲区
        fft_buffer_ = fftwf_alloc_complex(fft_size_);
        
        // 创建FFT计划
        fft_plan_ = fftwf_plan_dft_1d(fft_size_, fft_buffer_, fft_buffer_, FFTW_FORWARD, FFTW_ESTIMATE);
        
        // 初始化窗口函数
        window_.resize(fft_size_);
        for (int i = 0; i < fft_size_; ++i) {
            window_[i] = 0.54f - 0.46f * std::cos(2.0f * M_PI * i / (fft_size_ - 1));
        }
    }
    
    void cleanup() {
        if (fft_plan_) fftwf_destroy_plan(fft_plan_);
        if (fft_buffer_) fftwf_free(fft_buffer_);
    }
    
    // 计算频率分辨率
    double getFrequencyResolution() const {
        return sample_rate_ / fft_size_;
    }
    
    // 计算FFT bin对应的频率
    double getBinFrequency(int bin) const {
        return bin * sample_rate_ / fft_size_;
    }
    
    // 计算频率对应的FFT bin
    int getFrequencyBin(double frequency) const {
        return static_cast<int>(frequency * fft_size_ / sample_rate_ + 0.5);
    }
    
    // 分析音频片段的频谱
    std::vector<float> analyzeSpectrum(const std::vector<short>& audio_data, size_t start_sample) {
        std::vector<float> spectrum(fft_size_ / 2);
        
        // 填充输入缓冲区
        for (int i = 0; i < fft_size_; ++i) {
            if (start_sample + i < audio_data.size()) {
                fft_buffer_[i][0] = static_cast<float>(audio_data[start_sample + i]) / 32767.0f * window_[i];
            } else {
                fft_buffer_[i][0] = 0.0f;
            }
            fft_buffer_[i][1] = 0.0f;
        }
        
        // 执行FFT
        fftwf_execute(fft_plan_);
        
        // 计算功率谱
        for (int i = 0; i < fft_size_ / 2; ++i) {
            float real = fft_buffer_[i][0];
            float imag = fft_buffer_[i][1];
            spectrum[i] = std::sqrt(real * real + imag * imag);
        }
        
        return spectrum;
    }
    
    // 计算8个EQ频段的频率
    static std::vector<float> calculateEQFreqs(int count = 8, int minFreq = 200, int maxFreq = 18000) {
        std::vector<float> freqs(count);
        for (int i = 0; i < count; ++i) {
            freqs[i] = minFreq * std::pow(static_cast<float>(maxFreq) / minFreq, static_cast<float>(i) / (count - 1));
        }
        return freqs;
    }
    
    // 显示频段映射信息
    void showBandMapping() {
        auto eq_freqs = calculateEQFreqs(8);
        double freq_res = getFrequencyResolution();
        
        std::cout << "\n=== 频段映射分析 ===" << std::endl;
        std::cout << "FFT大小: " << fft_size_ << std::endl;
        std::cout << "采样率: " << sample_rate_ << " Hz" << std::endl;
        std::cout << "频率分辨率: " << std::fixed << std::setprecision(2) << freq_res << " Hz/bin" << std::endl;
        
        std::cout << "\n8个EQ频段映射:" << std::endl;
        std::cout << std::setw(8) << "频段" << std::setw(10) << "中心频率" << std::setw(8) << "FFT bin" << std::setw(15) << "频率范围" << std::endl;
        std::cout << std::string(45, '-') << std::endl;
        
        for (size_t i = 0; i < eq_freqs.size(); ++i) {
            int bin = getFrequencyBin(eq_freqs[i]);
            double bin_freq = getBinFrequency(bin);
            double lower_freq = getBinFrequency(bin - 1);
            double upper_freq = getBinFrequency(bin + 1);
            
            std::cout << std::setw(8) << (i + 1) 
                      << std::setw(10) << std::fixed << std::setprecision(1) << eq_freqs[i] << "Hz"
                      << std::setw(8) << bin
                      << std::setw(7) << std::fixed << std::setprecision(0) << lower_freq << "-"
                      << std::setw(7) << std::fixed << std::setprecision(0) << upper_freq << "Hz" << std::endl;
        }
    }
    
    // 分析音频中的主要频率成分
    void analyzeAudioFrequencies(const std::vector<short>& audio_data) {
        std::cout << "\n=== 音频频率成分分析 ===" << std::endl;
        
        // 分析多个时间片段
        size_t num_segments = 5;
        size_t segment_size = fft_size_;
        size_t step = (audio_data.size() - segment_size) / num_segments;
        
        std::vector<float> avg_spectrum(fft_size_ / 2, 0.0f);
        
        for (size_t seg = 0; seg < num_segments; ++seg) {
            size_t start_sample = seg * step;
            auto spectrum = analyzeSpectrum(audio_data, start_sample);
            
            for (size_t i = 0; i < spectrum.size(); ++i) {
                avg_spectrum[i] += spectrum[i];
            }
        }
        
        // 计算平均频谱
        for (size_t i = 0; i < avg_spectrum.size(); ++i) {
            avg_spectrum[i] /= num_segments;
        }
        
        // 找到主要频率成分
        std::vector<std::pair<float, int>> peaks;
        for (int i = 1; i < static_cast<int>(avg_spectrum.size()) - 1; ++i) {
            if (avg_spectrum[i] > avg_spectrum[i-1] && avg_spectrum[i] > avg_spectrum[i+1]) {
                float freq = getBinFrequency(i);
                if (freq >= 50 && freq <= 20000) { // 只关注可听频率范围
                    peaks.push_back({avg_spectrum[i], i});
                }
            }
        }
        
        // 按强度排序
        std::sort(peaks.begin(), peaks.end(), std::greater<std::pair<float, int>>());
        
        std::cout << "\n主要频率成分 (前10个):" << std::endl;
        std::cout << std::setw(8) << "排名" << std::setw(10) << "频率" << std::setw(8) << "FFT bin" << std::setw(12) << "强度" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        
        for (size_t i = 0; i < std::min(size_t(10), peaks.size()); ++i) {
            float freq = getBinFrequency(peaks[i].second);
            std::cout << std::setw(8) << (i + 1)
                      << std::setw(10) << std::fixed << std::setprecision(1) << freq << "Hz"
                      << std::setw(8) << peaks[i].second
                      << std::setw(12) << std::fixed << std::setprecision(4) << peaks[i].first << std::endl;
        }
        
        // 分析每个EQ频段的能量
        auto eq_freqs = calculateEQFreqs(8);
        std::cout << "\n各EQ频段能量分布:" << std::endl;
        std::cout << std::setw(8) << "频段" << std::setw(10) << "中心频率" << std::setw(12) << "能量" << std::setw(15) << "占总能量%" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        
        float total_energy = 0.0f;
        for (size_t i = 0; i < avg_spectrum.size(); ++i) {
            total_energy += avg_spectrum[i];
        }
        
        for (size_t i = 0; i < eq_freqs.size(); ++i) {
            int bin = getFrequencyBin(eq_freqs[i]);
            float energy = 0.0f;
            
            // 计算频段附近的能量（±2个bin）
            for (int j = std::max(0, bin - 2); j <= std::min(static_cast<int>(avg_spectrum.size()) - 1, bin + 2); ++j) {
                energy += avg_spectrum[j];
            }
            
            float percentage = (energy / total_energy) * 100.0f;
            std::cout << std::setw(8) << (i + 1)
                      << std::setw(10) << std::fixed << std::setprecision(1) << eq_freqs[i] << "Hz"
                      << std::setw(12) << std::fixed << std::setprecision(4) << energy
                      << std::setw(15) << std::fixed << std::setprecision(2) << percentage << "%" << std::endl;
        }
    }
};

int main() {
    std::cout << "=== PCM音频频率分析 ===" << std::endl;
    std::cout << "分析PCM数据如何映射到频段" << std::endl;
    
    // 读取音频文件
    std::cout << "\n步骤1: 读取音频文件..." << std::endl;
    auto input_audio = read_pcm_file_int16("res/48000_1_s16le.pcm");
    
    if (input_audio.empty()) {
        std::cerr << "❌ 无法读取音频文件" << std::endl;
        return 1;
    }
    
    // 创建频率分析器
    FrequencyAnalyzer analyzer(1024, 48000.0);
    
    // 显示频段映射信息
    analyzer.showBandMapping();
    
    // 分析音频频率成分
    analyzer.analyzeAudioFrequencies(input_audio);
    
    std::cout << "\n=== 分析完成 ===" << std::endl;
    std::cout << "\n说明:" << std::endl;
    std::cout << "1. PCM数据通过FFT转换为频域" << std::endl;
    std::cout << "2. 每个FFT bin代表约47Hz的频率范围" << std::endl;
    std::cout << "3. EQ频段通过FFT bin来影响特定频率范围" << std::endl;
    std::cout << "4. 调整某个频段的dB值会影响该频段附近的频率" << std::endl;
    
    return 0;
} 