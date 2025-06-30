## SpeexDSP 五大功能模块

### 1. **静音检测 (Voice Activity Detection - VAD)**
- **文件**: `speex_preprocess.h`
- **功能**: 检测音频中的语音活动
- **API**: `SPEEX_PREPROCESS_SET_VAD`, `speex_preprocess_run()`
- **特点**: 提供语音概率、可调节阈值

### 2. **降噪 (Noise Suppression)**
- **文件**: `speex_preprocess.h`
- **功能**: 抑制背景噪声
- **API**: `SPEEX_PREPROCESS_SET_DENOISE`, `SPEEX_PREPROCESS_SET_NOISE_SUPPRESS`
- **特点**: 自适应噪声抑制，可调节抑制强度

### 3. **自动增益控制 (Automatic Gain Control - AGC)**
- **文件**: `speex_preprocess.h`
- **功能**: 自动调整音频增益
- **API**: `SPEEX_PREPROCESS_SET_AGC`, `SPEEX_PREPROCESS_SET_AGC_LEVEL`
- **特点**: 动态音量调整，防止过载

### 4. **回声消除 (Echo Cancellation - AEC)**
- **文件**: `speex_echo.h`
- **功能**: 消除音频回声和反馈
- **API**: `speex_echo_state_init()`, `speex_echo_cancellation()`
- **特点**: 支持单通道和多通道，自适应滤波器

### 5. **抖动缓冲 (Jitter Buffer)**
- **文件**: `speex_jitter.h`
- **功能**: 处理网络抖动和包丢失
- **API**: `jitter_buffer_init()`, `jitter_buffer_put()`, `jitter_buffer_get()`
- **特点**: 自适应缓冲，包重排序，丢包补偿

## 额外功能

### 6. **重采样 (Resampler)**
- **文件**: `speex_resampler.h`
- **功能**: 音频采样率转换
- **API**: `speex_resampler_init()`, `speex_resampler_process_int()`
- **特点**: 高质量重采样，支持多种质量级别

### 7. **去混响 (Dereverb)**
- **文件**: `speex_preprocess.h`
- **功能**: 减少混响效果
- **API**: `SPEEX_PREPROCESS_SET_DEREVERB`
- **特点**: 可调节去混响强度和衰减
