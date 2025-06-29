#pragma once
#include <speex/speex_preprocess.h>
#include <vector>

// 语音活动检测
namespace srv {

class VAD {
private:
    SpeexPreprocessState* preprocess_state_;
    int frame_size_;
    int sample_rate_;
    bool is_initialized_;
    
    // VAD配置参数
    int vad_enabled_;
    int prob_start_;      // 从静音到语音的概率阈值
    int prob_continue_;   // 保持语音状态的概率阈值
    int noise_suppress_;  // 噪声抑制级别

public:
    VAD();
    ~VAD();
    
    /**
     * 初始化VAD
     * @param sample_rate 采样率 (Hz)
     * @param frame_size 帧大小 (样本数，通常对应10-20ms)
     * @return 是否初始化成功
     */
    bool init(int sample_rate = 16000, int frame_size = 160);
    
    /**
     * 检测音频帧中是否有语音活动
     * @param audio_frame 音频帧数据 (16位PCM)
     * @param frame_size 帧大小
     * @return 1表示有语音，0表示静音
     */
    int detect_voice_activity(spx_int16_t* audio_frame, int frame_size);
    
    /**
     * 检测音频帧中是否有语音活动 (使用vector)
     * @param audio_frame 音频帧数据
     * @return 1表示有语音，0表示静音
     */
    int detect_voice_activity(const std::vector<spx_int16_t>& audio_frame);
    
    /**
     * 设置VAD参数
     * @param prob_start 从静音到语音的概率阈值 (0-100)
     * @param prob_continue 保持语音状态的概率阈值 (0-100)
     * @param noise_suppress 噪声抑制级别 (dB, 负值)
     */
    void set_vad_params(int prob_start = 80, int prob_continue = 80, int noise_suppress = -15);
    
    /**
     * 启用或禁用VAD
     * @param enabled true启用VAD，false禁用
     */
    void set_vad_enabled(bool enabled);
    
    /**
     * 获取当前VAD状态
     * @return true如果VAD已启用
     */
    bool is_vad_enabled() const;
    
    /**
     * 获取语音概率
     * @return 语音概率 (0-100)
     */
    int get_speech_probability();
    
    /**
     * 重置VAD状态
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
};

} // namespace srv
