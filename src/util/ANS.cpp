#include "ANS.h"
#include <speex/speex_preprocess.h>
#include <iostream>
#include <cstring>
#include <algorithm>

namespace srv {

ANS::ANS() 
    : preprocess_state_(nullptr)
    , frame_size_(160)
    , sample_rate_(16000)
    , is_initialized_(false)
    , noise_suppress_(-15)
    , echo_suppress_(-40)
    , echo_suppress_active_(-15)
    , agc_level_(8000)
    , agc_increment_(32768)
    , agc_decrement_(32768)
    , agc_max_gain_(32768) {
}

ANS::~ANS() {
    if (preprocess_state_) {
        speex_preprocess_state_destroy(preprocess_state_);
        preprocess_state_ = nullptr;
    }
}

bool ANS::init(int sample_rate, int frame_size) {
    std::cout << "ANS::init called with sample_rate=" << sample_rate << ", frame_size=" << frame_size << std::endl;
    
    // 如果已经初始化，先清理
    if (preprocess_state_) {
        std::cout << "Destroying existing preprocess state" << std::endl;
        speex_preprocess_state_destroy(preprocess_state_);
        preprocess_state_ = nullptr;
    }
    
    // 参数验证
    if (sample_rate <= 0 || frame_size <= 0) {
        std::cerr << "ANS init failed: invalid parameters" << std::endl;
        return false;
    }
    
    std::cout << "Creating speex preprocess state..." << std::endl;
    
    // 创建speex预处理器状态
    preprocess_state_ = speex_preprocess_state_init(frame_size, sample_rate);
    
    std::cout << "speex_preprocess_state_init returned: " << (preprocess_state_ ? "valid pointer" : "null") << std::endl;
    
    if (!preprocess_state_) {
        std::cerr << "ANS init failed: cannot create preprocess state" << std::endl;
        return false;
    }
    
    // 保存参数
    frame_size_ = frame_size;
    sample_rate_ = sample_rate;
    is_initialized_ = true;
    
    std::cout << "Setting default parameters..." << std::endl;
    
    // 设置默认参数
    set_noise_suppress_params(noise_suppress_, echo_suppress_, echo_suppress_active_);
    set_agc_params(agc_level_, agc_increment_, agc_decrement_, agc_max_gain_);
    
    // 默认启用所有功能
    set_noise_suppress_enabled(true);
    set_agc_enabled(true);
    set_echo_suppress_enabled(true);
    
    std::cout << "ANS initialized successfully: sample_rate=" << sample_rate 
              << ", frame_size=" << frame_size << std::endl;
    
    return true;
}

std::vector<spx_int16_t> ANS::process_frame(const spx_int16_t* audio_frame, int frame_size) {
    std::cout << "ANS::process_frame called with frame_size=" << frame_size << std::endl;
    
    if (!is_initialized_ || !preprocess_state_) {
        std::cerr << "ANS not initialized" << std::endl;
        return std::vector<spx_int16_t>();
    }
    
    if (!audio_frame || frame_size != frame_size_) {
        std::cerr << "ANS process failed: invalid audio frame or frame size" << std::endl;
        std::cerr << "  audio_frame: " << (audio_frame ? "valid" : "null") << std::endl;
        std::cerr << "  frame_size: " << frame_size << ", expected: " << frame_size_ << std::endl;
        return std::vector<spx_int16_t>();
    }
    
    try {
        std::cout << "Creating output frame with size " << frame_size << std::endl;
        
        // 创建输出缓冲区
        std::vector<spx_int16_t> output_frame(frame_size);
        std::cout << "Output frame created, size: " << output_frame.size() << std::endl;
        
        std::cout << "Copying audio data..." << std::endl;
        std::memcpy(output_frame.data(), audio_frame, frame_size * sizeof(spx_int16_t));
        std::cout << "Audio data copied successfully" << std::endl;
        
        // 检查前几个样本值
        std::cout << "First 5 samples: ";
        for (int i = 0; i < std::min(5, frame_size); ++i) {
            std::cout << output_frame[i] << " ";
        }
        std::cout << std::endl;
        
        std::cout << "Calling speex_preprocess_run..." << std::endl;
        
        // 使用speex预处理器进行噪声抑制
        // 注意：speex_preprocess_run会修改输入的音频数据
        int result = speex_preprocess_run(preprocess_state_, output_frame.data());
        
        std::cout << "speex_preprocess_run completed with result: " << result << std::endl;
        
        if (result != 0) {
            std::cerr << "ANS process warning: speex_preprocess_run returned " << result << std::endl;
        }
        
        std::cout << "Returning processed frame" << std::endl;
        return output_frame;
    } catch (const std::exception& e) {
        std::cerr << "ANS process exception: " << e.what() << std::endl;
        return std::vector<spx_int16_t>();
    } catch (...) {
        std::cerr << "ANS process unknown exception" << std::endl;
        return std::vector<spx_int16_t>();
    }
}

std::vector<spx_int16_t> ANS::process_frame(const std::vector<spx_int16_t>& audio_frame) {
    if (audio_frame.size() != static_cast<size_t>(frame_size_)) {
        std::cerr << "ANS process failed: frame size mismatch" << std::endl;
        return std::vector<spx_int16_t>();
    }
    
    return process_frame(audio_frame.data(), frame_size_);
}

void ANS::set_noise_suppress_params(int noise_suppress, int echo_suppress, int echo_suppress_active) {
    if (!is_initialized_ || !preprocess_state_) {
        std::cerr << "ANS not initialized, cannot set parameters" << std::endl;
        return;
    }
    
    // 参数范围验证
    noise_suppress = std::max(-60, std::min(0, noise_suppress));
    echo_suppress = std::max(-60, std::min(0, echo_suppress));
    echo_suppress_active = std::max(-60, std::min(0, echo_suppress_active));
    
    // 保存参数
    noise_suppress_ = noise_suppress;
    echo_suppress_ = echo_suppress;
    echo_suppress_active_ = echo_suppress_active;
    
    // 设置speex预处理器参数
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noise_suppress_);
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &echo_suppress_);
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &echo_suppress_active_);
    
    std::cout << "ANS noise suppress parameters set: noise_suppress=" << noise_suppress_ 
              << ", echo_suppress=" << echo_suppress_ 
              << ", echo_suppress_active=" << echo_suppress_active_ << std::endl;
}

void ANS::set_agc_params(int agc_level, int agc_increment, int agc_decrement, int agc_max_gain) {
    if (!is_initialized_ || !preprocess_state_) {
        std::cerr << "ANS not initialized, cannot set AGC parameters" << std::endl;
        return;
    }
    
    // 参数范围验证
    agc_level = std::max(0, std::min(32768, agc_level));
    agc_increment = std::max(0, std::min(32768, agc_increment));
    agc_decrement = std::max(0, std::min(32768, agc_decrement));
    agc_max_gain = std::max(0, std::min(32768, agc_max_gain));
    
    // 保存参数
    agc_level_ = agc_level;
    agc_increment_ = agc_increment;
    agc_decrement_ = agc_decrement;
    agc_max_gain_ = agc_max_gain;
    
    // 设置speex预处理器AGC参数
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level_);
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &agc_increment_);
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &agc_decrement_);
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &agc_max_gain_);
    
    std::cout << "ANS AGC parameters set: agc_level=" << agc_level_ 
              << ", agc_increment=" << agc_increment_ 
              << ", agc_decrement=" << agc_decrement_ 
              << ", agc_max_gain=" << agc_max_gain_ << std::endl;
}

void ANS::set_noise_suppress_enabled(bool enabled) {
    if (!is_initialized_ || !preprocess_state_) {
        std::cerr << "ANS not initialized, cannot set noise suppress state" << std::endl;
        return;
    }
    
    int enable = enabled ? 1 : 0;
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_DENOISE, &enable);
    
    std::cout << "ANS noise suppress " << (enabled ? "enabled" : "disabled") << std::endl;
}

void ANS::set_agc_enabled(bool enabled) {
    if (!is_initialized_ || !preprocess_state_) {
        std::cerr << "ANS not initialized, cannot set AGC state" << std::endl;
        return;
    }
    
    int enable = enabled ? 1 : 0;
    speex_preprocess_ctl(preprocess_state_, SPEEX_PREPROCESS_SET_AGC, &enable);
    
    std::cout << "ANS AGC " << (enabled ? "enabled" : "disabled") << std::endl;
}

void ANS::set_echo_suppress_enabled(bool enabled) {
    if (!is_initialized_ || !preprocess_state_) {
        std::cerr << "ANS not initialized, cannot set echo suppress state" << std::endl;
        return;
    }
    
    // echo suppress功能只需设置echo_suppress参数，无需设置ECHO_STATE指针
    std::cout << "ANS echo suppress " << (enabled ? "enabled" : "disabled") << std::endl;
}

void ANS::reset() {
    if (!is_initialized_ || !preprocess_state_) {
        return;
    }
    
    // 重新初始化预处理器状态
    speex_preprocess_state_destroy(preprocess_state_);
    preprocess_state_ = speex_preprocess_state_init(frame_size_, sample_rate_);
    
    if (preprocess_state_) {
        // 重新设置参数
        set_noise_suppress_params(noise_suppress_, echo_suppress_, echo_suppress_active_);
        set_agc_params(agc_level_, agc_increment_, agc_decrement_, agc_max_gain_);
        
        // 重新启用功能
        set_noise_suppress_enabled(true);
        set_agc_enabled(true);
        set_echo_suppress_enabled(true);
        
        std::cout << "ANS state reset" << std::endl;
    } else {
        is_initialized_ = false;
        std::cerr << "ANS reset failed" << std::endl;
    }
}

} // namespace srv
