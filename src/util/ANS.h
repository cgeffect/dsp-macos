#pragma once
#include <speex/speex_preprocess.h>
#include <vector>

// 自适应噪声抑制 (Adaptive Noise Suppression)
namespace srv {

class ANS {
private:
    SpeexPreprocessState* preprocess_state_;
    int frame_size_;
    int sample_rate_;
    bool is_initialized_;
    
    // 噪声抑制参数
    int noise_suppress_;      // 噪声抑制级别 (dB, 负值)
    int echo_suppress_;       // 回声抑制级别 (dB, 负值)
    int echo_suppress_active_; // 主动回声抑制级别 (dB, 负值)
    int agc_level_;          // AGC目标电平
    int agc_increment_;      // AGC增量
    int agc_decrement_;      // AGC减量
    int agc_max_gain_;       // AGC最大增益

public:
    ANS();
    ~ANS();
    
    /**
     * 初始化噪声抑制器
     * @param sample_rate 采样率 (Hz)
     * @param frame_size 帧大小 (样本数，通常对应10-20ms)
     * @return 是否初始化成功
     */
    bool init(int sample_rate = 16000, int frame_size = 160);
    
    /**
     * 处理音频帧进行噪声抑制
     * @param audio_frame 输入音频帧数据 (16位PCM)
     * @param frame_size 帧大小
     * @return 处理后的音频帧数据
     */
    std::vector<spx_int16_t> process_frame(const spx_int16_t* audio_frame, int frame_size);
    
    /**
     * 处理音频帧进行噪声抑制 (使用vector)
     * @param audio_frame 输入音频帧数据
     * @return 处理后的音频帧数据
     */
    std::vector<spx_int16_t> process_frame(const std::vector<spx_int16_t>& audio_frame);
    
    /**
     * 设置噪声抑制参数
     * @param noise_suppress 噪声抑制级别 (dB, 负值，范围-60到0)
     * @param echo_suppress 回声抑制级别 (dB, 负值，范围-60到0)
     * @param echo_suppress_active 主动回声抑制级别 (dB, 负值，范围-60到0)
     */
    void set_noise_suppress_params(int noise_suppress = -15, 
                                  int echo_suppress = -40, 
                                  int echo_suppress_active = -15);
    
    /**
     * 设置AGC参数
     * @param agc_level AGC目标电平 (dB, 范围0到8000)
     * @param agc_increment AGC增量 (dB, 范围0到32768)
     * @param agc_decrement AGC减量 (dB, 范围0到32768)
     * @param agc_max_gain AGC最大增益 (dB, 范围0到32768)
     */
    void set_agc_params(int agc_level = 8000, 
                       int agc_increment = 32768, 
                       int agc_decrement = 32768, 
                       int agc_max_gain = 32768);
    
    /**
     * 启用或禁用噪声抑制
     * @param enabled true启用，false禁用
     */
    void set_noise_suppress_enabled(bool enabled);
    
    /**
     * 启用或禁用AGC
     * @param enabled true启用，false禁用
     */
    void set_agc_enabled(bool enabled);
    
    /**
     * 启用或禁用回声抑制
     * @param enabled true启用，false禁用
     */
    void set_echo_suppress_enabled(bool enabled);
    
    /**
     * 重置噪声抑制器状态
     */
    void reset();
    
    /**
     * 检查是否已初始化
     * @return true如果已初始化
     */
    bool is_initialized() const { return is_initialized_; }
    
    /**
     * 获取帧大小
     * @return 帧大小
     */
    int get_frame_size() const { return frame_size_; }
    
    /**
     * 获取采样率
     * @return 采样率
     */
    int get_sample_rate() const { return sample_rate_; }
    
    /**
     * 获取当前噪声抑制级别
     * @return 噪声抑制级别
     */
    int get_noise_suppress_level() const { return noise_suppress_; }
    
    /**
     * 获取当前AGC目标电平
     * @return AGC目标电平
     */
    int get_agc_level() const { return agc_level_; }
};

} // namespace srv
