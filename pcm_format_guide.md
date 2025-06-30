# SpeexDSP VAD PCM数据格式规范

## 1. 基本格式要求

### 数据类型
- **16位有符号整数** (`spx_int16_t`)
- 数值范围：-32768 到 32767
- 字节序：小端序 (Little-endian)

### 采样率
- **推荐：16kHz** (16000 Hz)
- **支持：8kHz** (8000 Hz)
- 其他采样率也可以使用，但效果可能不如推荐值

### 声道
- **单声道** (Mono)
- 多声道音频需要分别处理每个声道

### 帧大小
- **推荐：160样本** (10ms @ 16kHz)
- **支持范围：80-512样本**
- 必须与VAD初始化时指定的frame_size一致

## 2. 数据布局

### 内存布局
```
每个样本占用2字节 (16位)
[低字节][高字节] 或 [高字节][低字节] (取决于系统)
```

### 文件格式
```
PCM文件 = 原始音频数据 (无头部信息)
文件大小 = 样本数 × 2字节
```

## 3. 常见PCM格式对比

| 格式 | 位深度 | 采样率 | 声道 | 兼容性 |
|------|--------|--------|------|--------|
| 16-bit PCM | 16位 | 16kHz | 单声道 | ✅ 推荐 |
| 16-bit PCM | 16位 | 8kHz | 单声道 | ✅ 支持 |
| 16-bit PCM | 16位 | 44.1kHz | 单声道 | ⚠️ 需要重采样 |
| 24-bit PCM | 24位 | 16kHz | 单声道 | ❌ 不兼容 |
| 32-bit float | 32位 | 16kHz | 单声道 | ❌ 不兼容 |

## 4. 音频文件转换示例

### 使用ffmpeg转换
```bash
# WAV转PCM (16kHz, 16位, 单声道)
ffmpeg -i input.wav -f s16le -acodec pcm_s16le -ar 16000 -ac 1 output.pcm

# MP3转PCM
ffmpeg -i input.mp3 -f s16le -acodec pcm_s16le -ar 16000 -ac 1 output.pcm

# 其他格式转PCM
ffmpeg -i input.m4a -f s16le -acodec pcm_s16le -ar 16000 -ac 1 output.pcm
```

### 使用sox转换
```bash
# WAV转PCM
sox input.wav -r 16000 -c 1 -b 16 output.pcm

# MP3转PCM
sox input.mp3 -r 16000 -c 1 -b 16 output.pcm
```

## 5. 代码中的格式验证

```cpp
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
    }
    
    // 检查数据范围
    for (spx_int16_t sample : audio_data) {
        if (sample < -32768 || sample > 32767) {
            std::cerr << "错误：样本值超出16位范围" << std::endl;
            return false;
        }
    }
    
    return true;
}
```

## 6. 实际应用建议

### 音频录制
```cpp
// 录制音频时使用以下参数
int sample_rate = 16000;    // 16kHz
int channels = 1;           // 单声道
int bits_per_sample = 16;   // 16位
int frame_size = 160;       // 10ms帧
```

### 音频播放
```cpp
// 播放PCM数据时确保格式匹配
// 采样率、声道数、位深度必须一致
```

### 文件处理
```cpp
// 读取PCM文件时注意字节序
// 在跨平台应用中可能需要字节序转换
```

## 7. 常见问题

### Q: 为什么推荐16kHz采样率？
A: SpeexDSP的VAD算法针对语音信号优化，16kHz能够很好地捕获人声频率范围(300Hz-3400Hz)。

### Q: 可以使用立体声吗？
A: 不可以，VAD需要单声道输入。立体声需要先转换为单声道。

### Q: 24位或32位音频可以吗？
A: 不可以，必须转换为16位。可以使用音频工具进行转换。

### Q: 帧大小可以随意设置吗？
A: 不可以，必须与VAD初始化时的frame_size参数一致，推荐160样本(10ms@16kHz)。

## 8. 性能考虑

- **内存使用**：16kHz单声道，1秒音频 = 16000 × 2字节 = 32KB
- **处理延迟**：10ms帧大小，延迟约10ms
- **CPU占用**：相对较低，适合实时处理 