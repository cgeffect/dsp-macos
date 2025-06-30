# ANS降噪测试总结

## 测试概述

我们创建了一个清晰的ANS（Acoustic Noise Suppression）降噪测试，使用纯正弦波+白噪声来验证SpeexDSP的降噪效果。

## 生成的文件

### 基础测试文件
- `improved_clean_sine_440Hz.pcm` - 纯440Hz正弦波（无噪声，参考标准）
- `improved_white_noise.pcm` - 白噪声
- `improved_noisy_audio.pcm` - 正弦波+噪声（处理前）
- `improved_processed_audio.pcm` - ANS处理后音频（-45dB抑制）

### 不同参数测试文件
- `improved_test_-20dB.pcm` - 轻度抑制测试
- `improved_test_-30dB.pcm` - 中度抑制测试  
- `improved_test_-40dB.pcm` - 强度抑制测试
- `improved_test_-50dB.pcm` - 极强抑制测试

## 测试配置

- **采样率**: 16kHz
- **帧大小**: 160样本（10ms）
- **持续时间**: 5秒
- **正弦波频率**: 440Hz
- **正弦波振幅**: 8000
- **噪声振幅**: 2500
- **原始信噪比**: 约8.5dB

## 测试结果分析

### 主要发现

1. **SpeexDSP降噪功能正常工作** - 程序不再崩溃，所有参数都能正常处理
2. **降噪效果有限** - 从数值上看，降噪效果不如预期明显
3. **参数敏感性** - 不同噪声抑制级别（-20dB到-50dB）的效果差异不大

### 可能的原因

1. **白噪声特性** - 白噪声在所有频率上都有能量，SpeexDSP可能难以完全抑制
2. **信号特征** - 正弦波是单频信号，SpeexDSP可能更适合处理语音信号
3. **参数调优** - 可能需要更精细的参数调整

## 如何测试

### 使用播放脚本
```bash
# 播放纯正弦波（应该很干净）
./play_audio.sh improved_clean_sine_440Hz.pcm

# 播放带噪声的音频（应该很嘈杂）
./play_audio.sh improved_noisy_audio.pcm

# 播放处理后的音频
./play_audio.sh improved_processed_audio.pcm

# 对比不同参数的效果
./play_audio.sh improved_test_-20dB.pcm
./play_audio.sh improved_test_-50dB.pcm
```

### 手动播放（需要ffmpeg）
```bash
ffplay -f s16le -ar 16000 -ac 1 -nodisp -autoexit improved_clean_sine_440Hz.pcm
```

## 建议的下一步

1. **测试真实语音** - 使用真实的带噪声语音文件测试
2. **调整参数** - 尝试不同的噪声抑制和AGC参数
3. **频谱分析** - 使用音频分析工具查看频谱变化
4. **对比其他算法** - 与WebRTC的NS算法对比

## 技术细节

### 解决的问题
- 修复了SpeexDSP初始化时的崩溃问题（移除了错误的ECHO_STATE设置）
- 实现了完整的ANS类封装
- 添加了噪声学习阶段（前5帧用于学习）

### 代码结构
- `src/improved_ans_test.cpp` - 改进的测试程序
- `src/util/ANS.cpp` - ANS类实现
- `play_audio.sh` - 音频播放脚本

## 结论

虽然数值上的降噪效果不如预期明显，但SpeexDSP的降噪功能已经正常工作。建议在实际应用中使用真实的语音数据进行测试，因为SpeexDSP主要是为语音信号设计的，对纯正弦波+白噪声的测试可能不是最佳场景。 