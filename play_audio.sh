#!/bin/bash

# 音频播放脚本
# 使用方法: ./play_audio.sh <pcm_file>

echo "=== 音频播放脚本 ==="
echo ""

# 检查ffplay是否可用
if ! command -v ffplay &> /dev/null; then
    echo "❌ 错误: ffplay 未找到，请安装 ffmpeg"
    echo "macOS: brew install ffmpeg"
    exit 1
fi

# 播放参数
SAMPLE_RATE=16000
CHANNELS=1

echo "播放参数:"
echo "  采样率: ${SAMPLE_RATE} Hz"
echo "  声道数: ${CHANNELS}"
echo ""

# 函数：播放音频文件
play_audio() {
    local file="$1"
    local description="$2"
    
    if [ -f "$file" ]; then
        echo "🎵 播放: $description"
        echo "   文件: $file"
        echo "   命令: ffplay -f s16le -ar ${SAMPLE_RATE} -ac ${CHANNELS} -nodisp -autoexit $file"
        echo ""
        ffplay -f s16le -ar ${SAMPLE_RATE} -ac ${CHANNELS} -nodisp -autoexit "$file" 2>/dev/null
        echo ""
    else
        echo "❌ 文件不存在: $file"
        echo ""
    fi
}

# VAD测试文件
echo "=== VAD测试文件 ==="
play_audio "vad_test_voice.pcm" "语音信号（包含静音段）"
play_audio "vad_test_noise.pcm" "噪声信号"
play_audio "vad_test_mixed.pcm" "语音+噪声混合信号"

# ANS测试文件
echo "=== ANS噪声抑制测试文件 ==="
play_audio "ans_sine_clean.pcm" "纯净正弦波"
play_audio "ans_sine_noisy.pcm" "带噪声的正弦波"
play_audio "ans_sine_processed.pcm" "噪声抑制后的正弦波"

# 真实语音测试文件
echo "=== 真实语音噪声抑制测试文件 ==="
play_audio "real_voice_clean.pcm" "纯净语音"
play_audio "real_voice_white_noise.pcm" "白噪声"
play_audio "real_voice_white_noisy.pcm" "语音+白噪声"
play_audio "real_voice_white_processed.pcm" "白噪声抑制后"

play_audio "real_voice_pink_noise.pcm" "粉红噪声"
play_audio "real_voice_pink_noisy.pcm" "语音+粉红噪声"
play_audio "real_voice_pink_processed.pcm" "粉红噪声抑制后"

play_audio "real_voice_babble_noise.pcm" "人声噪声"
play_audio "real_voice_babble_noisy.pcm" "语音+人声噪声"
play_audio "real_voice_babble_processed.pcm" "人声噪声抑制后"

play_audio "real_voice_machine_noise.pcm" "机器噪声"
play_audio "real_voice_machine_noisy.pcm" "语音+机器噪声"
play_audio "real_voice_machine_processed.pcm" "机器噪声抑制后"

# AGC测试文件
echo "=== AGC自动增益控制测试文件 ==="
echo "原始输入文件（音量变化很大）:"
play_audio "agc_variable_sine_input.pcm" "不同振幅正弦波"
play_audio "agc_fade_sine_input.pcm" "渐变音量正弦波"
play_audio "agc_variable_voice_input.pcm" "不同音量语音"

echo "AGC处理后文件（音量应该更稳定）:"
echo "标准AGC配置:"
play_audio "agc_标准AGC_variable_sine_output.pcm" "标准AGC - 不同振幅正弦波"
play_audio "agc_标准AGC_fade_sine_output.pcm" "标准AGC - 渐变音量正弦波"
play_audio "agc_标准AGC_variable_voice_output.pcm" "标准AGC - 不同音量语音"

echo "强AGC配置:"
play_audio "agc_强AGC_variable_sine_output.pcm" "强AGC - 不同振幅正弦波"
play_audio "agc_强AGC_fade_sine_output.pcm" "强AGC - 渐变音量正弦波"
play_audio "agc_强AGC_variable_voice_output.pcm" "强AGC - 不同音量语音"

echo "弱AGC配置:"
play_audio "agc_弱AGC_variable_sine_output.pcm" "弱AGC - 不同振幅正弦波"
play_audio "agc_弱AGC_fade_sine_output.pcm" "弱AGC - 渐变音量正弦波"
play_audio "agc_弱AGC_variable_voice_output.pcm" "弱AGC - 不同音量语音"

echo "快速AGC配置:"
play_audio "agc_快速AGC_variable_sine_output.pcm" "快速AGC - 不同振幅正弦波"
play_audio "agc_快速AGC_fade_sine_output.pcm" "快速AGC - 渐变音量正弦波"
play_audio "agc_快速AGC_variable_voice_output.pcm" "快速AGC - 不同音量语音"

echo "慢速AGC配置:"
play_audio "agc_慢速AGC_variable_sine_output.pcm" "慢速AGC - 不同振幅正弦波"
play_audio "agc_慢速AGC_fade_sine_output.pcm" "慢速AGC - 渐变音量正弦波"
play_audio "agc_慢速AGC_variable_voice_output.pcm" "慢速AGC - 不同音量语音"

echo "=== 播放完成 ===" 