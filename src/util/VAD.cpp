#include "VAD.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace srv {

VAD::VAD() 
    : preprocess_state_(nullptr)
    , frame_size_(160)
    , sample_rate_(16000)
    , is_initialized_(false)
    , vad_enabled_(1)
    , prob_start_(80)
    , prob_continue_(80)
    , noise_suppress_(-15) {
}

VAD::~VAD() {
    if (preprocess_state_) {
        speex_preprocess_state_destroy(preprocess_state_);
        preprocess_state_ = nullptr;
    }
}

bool VAD::init(int sample_rate, int frame_size) {
    // 如果已经初始化，先清理
    if (preprocess_state_) {
        speex_preprocess_state_destroy(preprocess_state_);
        preprocess_state_ = nullptr;
    }
    
    // 参数验证
    if (sample_rate <= 0 || frame_size <= 0) {
        std::cerr << "VAD init failed: invalid parameters" << std::endl;
        return false;
    }
    
    // 创建speex预处理器状态
    preprocess_state_ = speex_preprocess_state_init(frame_size, sample_rate);
    if (!preprocess_state_) {
        std::cerr << "VAD init failed: cannot create preprocess state" << std::endl;
        return false;
    }
    
    // 保存参数
    frame_size_ = frame_size;
    sample_rate_ = sample_rate;
    is_initialized_ = true;
    
    // 设置默认VAD参数
    set_vad_params(prob_start_, prob_continue_, noise_suppress_);
    set_vad_enabled(true);
    
    std::cout << "VAD initialized successfully: sample_rate=" << sample_rate 
              << ", frame_size=" << frame_size << std::endl;
    
    return true;
}

int VAD::detect_voice_activity(spx_int16_t* audio_frame, int frame_size) {
    if (!is_initialized_ || !preprocess_state_) {
        std::cerr << "VAD not initialized" << std::endl;
        return 0;
    }
    
    if (!audio_frame || frame_size != frame_size_) {
        std::cerr << "VAD detect failed: invalid audio frame or frame size" << std::endl;
        return 0;
    }
    
    // 使用speex预处理器进行VAD检测
    // 注意：speex_preprocess_run会修改输入的音频数据
    // 如果需要保留原始数据，请先复制一份
    int vad_result = speex_preprocess_run(preprocess_state_, audio_frame);
    
    return vad_result;
}

int VAD::detect_voice_activity(const std::vector<spx_int16_t>& audio_frame) {
    if (audio_frame.size() != static_cast<size_t>(frame_size_)) {
        std::cerr << "VAD detect failed: frame size mismatch" << std::endl;
        return 0;
    }
    
    // 创建临时缓冲区，避免修改原始数据
    std::vector<spx_int16_t> temp_frame(audio_frame);
    return detect_voice_activity(temp_frame.data(), frame_size_);
}

void VAD::set_vad_params(int prob_start, int prob_continue, int noise_suppress) {
    if (!is_initialized_ || !preprocess_state_) {
        std::cerr << "VAD not initialized, cannot set parameters" << std::endl;
        return;
    }
    
    // 参数范围验证
    prob_start = std::max(0, std::min(100, prob_start));
    prob_continue = std::max(0, std::min(100, prob_continue));
    noise_suppress = std::max(-60, std::min(0, noise_suppress));
    
    // 保存参数
    prob_start_ = prob_start;
    prob_continue_ = prob_continue;
    noise_suppress_ = noise_suppress;
    
    // 设置speex预处理器参数
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_PROB_START, &prob_start_);
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &prob_continue_);
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noise_suppress_);
    
    std::cout << "VAD parameters set: prob_start=" << prob_start_ 
              << ", prob_continue=" << prob_continue_ 
              << ", noise_suppress=" << noise_suppress_ << std::endl;
}

void VAD::set_vad_enabled(bool enabled) {
    if (!is_initialized_ || !preprocess_state_) {
        std::cerr << "VAD not initialized, cannot set enabled state" << std::endl;
        return;
    }
    
    vad_enabled_ = enabled ? 1 : 0;
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_VAD, &vad_enabled_);
    
    std::cout << "VAD " << (enabled ? "enabled" : "disabled") << std::endl;
}

bool VAD::is_vad_enabled() const {
    return vad_enabled_ == 1;
}

int VAD::get_speech_probability() {
    if (!is_initialized_ || !preprocess_state_) {
        return 0;
    }
    
    int prob = 0;
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_GET_PROB, &prob);
    return prob;
}

void VAD::reset() {
    if (!is_initialized_ || !preprocess_state_) {
        return;
    }
    
    // 重新初始化预处理器状态
    speex_preprocess_state_destroy(preprocess_state_);
    preprocess_state_ = speex_preprocess_state_init(frame_size_, sample_rate_);
    
    if (preprocess_state_) {
        // 重新设置参数
        set_vad_params(prob_start_, prob_continue_, noise_suppress_);
        set_vad_enabled(vad_enabled_ == 1);
        std::cout << "VAD state reset" << std::endl;
    } else {
        is_initialized_ = false;
        std::cerr << "VAD reset failed" << std::endl;
    }
}

} // namespace srv
