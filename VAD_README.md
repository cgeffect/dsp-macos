# SpeexDSP VAD 静音检测功能

本项目使用SpeexDSP库实现了语音活动检测(VAD)功能，可以准确检测音频中的语音和静音部分。

## 功能特性

- 基于SpeexDSP的成熟VAD算法
- 支持多种采样率和帧大小配置
- 可调节的检测敏感度参数
- 提供语音概率信息
- 支持实时音频流处理
- 跨平台支持

## 编译要求

- CMake 3.26.1+
- C++17 编译器
- SpeexDSP库 (已包含在3rdparty目录中)
- macOS 14.0+ (当前配置)

## 编译方法

```bash
# 在项目根目录执行
./build.sh
```

或者使用Xcode：

```bash
./build-xcode.sh
```

## 使用方法

### 基本使用

```cpp
#include "util/VAD.h"

// 1. 创建VAD实例
srv::VAD vad;

// 2. 初始化 (16kHz采样率，160样本/帧 = 10ms)
if (!vad.init(16000, 160)) {
    // 处理初始化失败
    return -1;
}

// 3. 配置参数 (可选)
vad.set_vad_params(80, 80, -15);

// 4. 检测音频帧
std::vector<spx_int16_t> audio_frame(160);
// ... 填充音频数据 ...

int result = vad.detect_voice_activity(audio_frame);
if (result == 1) {
    std::cout << "检测到语音" << std::endl;
} else {
    std::cout << "检测到静音" << std::endl;
}

// 5. 获取语音概率
int prob = vad.get_speech_probability();
std::cout << "语音概率: " << prob << "%" << std::endl;
```

### 参数说明

#### 初始化参数
- `sample_rate`: 音频采样率 (Hz)，推荐16kHz
- `frame_size`: 帧大小 (样本数)，推荐160 (10ms@16kHz)

#### VAD参数
- `prob_start`: 从静音到语音的概率阈值 (0-100)
  - 值越高，越不容易从静音切换到语音
  - 推荐值：70-90
- `prob_continue`: 保持语音状态的概率阈值 (0-100)
  - 值越高，越不容易从语音切换到静音
  - 推荐值：70-90
- `noise_suppress`: 噪声抑制级别 (dB，负值)
  - 值越小，噪声抑制越强
  - 推荐值：-15 到 -25

### 高级功能

#### 动态调整参数
```cpp
// 根据环境噪声调整参数
if (noisy_environment) {
    vad.set_vad_params(85, 85, -20);  // 更严格的检测
} else {
    vad.set_vad_params(75, 75, -10);  // 更宽松的检测
}
```

#### 批量处理
```cpp
// 处理长音频数据
std::vector<spx_int16_t> long_audio = load_audio_file("audio.wav");
int frame_size = vad.get_frame_size();

for (size_t i = 0; i < long_audio.size(); i += frame_size) {
    std::vector<spx_int16_t> frame;
    // 提取当前帧...
    
    int vad_result = vad.detect_voice_activity(frame);
    // 处理结果...
}
```

#### 重置状态
```cpp
// 重置VAD内部状态，适用于处理新的音频流
vad.reset();
```

## API参考

### 构造函数
```cpp
VAD();
```

### 初始化
```cpp
bool init(int sample_rate = 16000, int frame_size = 160);
```

### 检测方法
```cpp
int detect_voice_activity(spx_int16_t* audio_frame, int frame_size);
int detect_voice_activity(const std::vector<spx_int16_t>& audio_frame);
```

### 参数设置
```cpp
void set_vad_params(int prob_start = 80, int prob_continue = 80, int noise_suppress = -15);
void set_vad_enabled(bool enabled);
```

### 状态查询
```cpp
bool is_vad_enabled() const;
int get_speech_probability();
bool is_initialized() const;
int get_frame_size() const;
int get_sample_rate() const;
```

### 控制方法
```cpp
void reset();
```

## 示例程序

项目包含两个示例程序：

1. `dsp_macos.cpp` - 完整的VAD演示程序
   - 生成各种测试音频
   - 演示不同参数的效果
   - 实时显示检测结果

2. `VAD_example.cpp` - 简单的使用示例
   - 基本的使用方法
   - 适合快速上手

## 性能说明

- 处理延迟：约10ms (取决于帧大小)
- CPU占用：低，适合实时应用
- 内存占用：约几KB
- 检测准确率：在正常环境下可达90%+

## 注意事项

1. 音频数据必须是16位PCM格式
2. 帧大小必须与初始化时指定的值一致
3. 采样率建议使用16kHz或8kHz
4. VAD检测会修改输入的音频数据，如需保留原始数据请先复制
5. 在嘈杂环境下可能需要调整参数以获得更好的效果

## 故障排除

### 常见问题

1. **初始化失败**
   - 检查SpeexDSP库是否正确链接
   - 确认参数值在有效范围内

2. **检测不准确**
   - 调整VAD参数
   - 检查音频质量和采样率
   - 考虑使用噪声抑制预处理

3. **编译错误**
   - 确认C++17支持
   - 检查CMake配置
   - 验证依赖库路径

## 许可证

本项目使用与SpeexDSP相同的许可证。 