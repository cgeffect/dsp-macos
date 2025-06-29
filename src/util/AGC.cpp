
#include <stdlib.h>

#include "AGC.h"
#include <speex/speex_preprocess.h>

namespace srv {

#include <speex/speex_preprocess.h>

#define FRAME_SIZE 160 // 常见的音频帧大小
#ifdef ENABLE_AGC
int agc_pro() {
    SpeexPreprocessState *state;
    int16_t input_frame[FRAME_SIZE];
    int16_t output_frame[FRAME_SIZE];

    // 初始化预处理状态
    state = speex_preprocess_state_init(FRAME_SIZE, 8000);

    // 设置自动增益控制参数
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_AGC, NULL);
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_AGC_LEVEL, NULL);

    // 模拟输入音频数据
    for (int i = 0; i < FRAME_SIZE; i++) {
        input_frame[i] = (int16_t)(rand() % 32768 - 16384);
    }

    // 进行自动增益处理
    spx_int16_t spx
    speex_preprocess_run(state, input_frame, output_frame);

    // 清理资源
    speex_preprocess_state_destroy(state);

    return 0;
}
#endif
AGC::AGC(/* args */) {
}

AGC::~AGC() {
}
} // namespace srv
