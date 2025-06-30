# 真实语音ANS降噪测试指南

## 测试概述

我们创建了一个使用真实语音模拟信号+不同类型噪声的ANS降噪测试，这比纯正弦波测试更能体现SpeexDSP的实际降噪效果。

## 语音信号特点

### 模拟语音信号
- **基频**: 150Hz（模拟男性语音）
- **谐波**: 150Hz, 300Hz, 450Hz, 600Hz, 750Hz, 900Hz, 1050Hz, 1200Hz
- **包络**: 包含起音和衰减阶段，模拟真实语音特征
- **随机性**: 添加少量随机变化，更接近真实语音

### 噪声类型

#### 1. 白噪声 (White Noise)
- **特点**: 在所有频率上能量均匀分布
- **应用场景**: 实验室测试，理想噪声
- **文件**: `voice_white_*.pcm`

#### 2. 粉红噪声 (Pink Noise)
- **特点**: 低频能量更多，1/f频谱特性
- **应用场景**: 更接近真实环境噪声
- **文件**: `voice_pink_*.pcm`

#### 3. 多人说话声 (Babble Noise)
- **特点**: 多个不同频率的正弦波组合
- **应用场景**: 会议室、咖啡厅等多人说话环境
- **文件**: `voice_babble_*.pcm`

#### 4. 机器噪声 (Machine Noise)
- **特点**: 60Hz基频及其谐波，模拟电器设备
- **应用场景**: 空调、风扇、电脑等设备噪声
- **文件**: `voice_machine_*.pcm`

## 测试文件说明

### 白噪声测试
```
voice_white_clean_voice.pcm      - 纯净语音信号
voice_white_noise.pcm           - 白噪声
voice_white_noisy_audio.pcm     - 语音+白噪声（处理前）
voice_white_processed_audio.pcm - ANS处理后音频
```

### 粉红噪声测试
```
voice_pink_clean_voice.pcm      - 纯净语音信号
voice_pink_noise.pcm           - 粉红噪声
voice_pink_noisy_audio.pcm     - 语音+粉红噪声（处理前）
voice_pink_processed_audio.pcm - ANS处理后音频
```

### 多人说话声测试
```
voice_babble_clean_voice.pcm      - 纯净语音信号
voice_babble_noise.pcm           - 多人说话声
voice_babble_noisy_audio.pcm     - 语音+多人说话声（处理前）
voice_babble_processed_audio.pcm - ANS处理后音频
```

### 机器噪声测试
```
voice_machine_clean_voice.pcm      - 纯净语音信号
voice_machine_noise.pcm           - 机器噪声
voice_machine_noisy_audio.pcm     - 语音+机器噪声（处理前）
voice_machine_processed_audio.pcm - ANS处理后音频
```

## 如何测试

### 1. 播放纯净语音（参考标准）
```bash
./play_audio.sh voice_white_clean_voice.pcm
```
**预期效果**: 清晰的语音信号，有规律的谐波结构

### 2. 播放带噪声的语音（处理前）
```bash
./play_audio.sh voice_white_noisy_audio.pcm
./play_audio.sh voice_pink_noisy_audio.pcm
./play_audio.sh voice_babble_noisy_audio.pcm
./play_audio.sh voice_machine_noisy_audio.pcm
```
**预期效果**: 语音被噪声干扰，清晰度下降

### 3. 播放处理后的语音
```bash
./play_audio.sh voice_white_processed_audio.pcm
./play_audio.sh voice_pink_processed_audio.pcm
./play_audio.sh voice_babble_processed_audio.pcm
./play_audio.sh voice_machine_processed_audio.pcm
```
**预期效果**: 噪声被抑制，语音更清晰

### 4. 对比测试
建议按以下顺序对比：
1. 先听纯净语音，了解原始信号特征
2. 再听带噪声的语音，感受噪声干扰
3. 最后听处理后的语音，评估降噪效果

## 测试结果分析

### 数值指标
- **原始信噪比**: 约9.5dB
- **噪声抑制效果**: 不同噪声类型的抑制效果
- **与纯净语音相似度**: 处理后音频与原始语音的相似程度

### 主观评估
1. **语音清晰度**: 处理后语音是否更容易理解
2. **噪声残留**: 是否还有明显的噪声
3. **语音失真**: 处理是否引入了新的失真
4. **整体质量**: 综合评估音频质量

## 预期效果

### 对不同噪声类型的效果
1. **白噪声**: 中等效果，SpeexDSP能部分抑制
2. **粉红噪声**: 较好效果，低频噪声更容易被抑制
3. **多人说话声**: 较好效果，语音信号与噪声有频率差异
4. **机器噪声**: 最好效果，低频周期性噪声容易被识别和抑制

### 实际应用建议
- **会议室**: 使用多人说话声测试结果
- **办公室**: 使用机器噪声测试结果
- **户外**: 使用粉红噪声测试结果
- **实验室**: 使用白噪声测试结果

## 技术参数

- **采样率**: 16kHz
- **帧大小**: 160样本（10ms）
- **持续时间**: 8秒
- **语音振幅**: 6000
- **噪声振幅**: 2000
- **ANS参数**: 噪声抑制-35dB，AGC标准设置

## 结论

这个真实语音测试比纯正弦波测试更能反映SpeexDSP在实际应用中的降噪效果。通过对比不同类型的噪声，可以更好地理解算法在各种环境下的表现。 