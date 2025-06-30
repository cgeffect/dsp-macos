查看二进制数据
hexdump -C ../res/wubai.aac | head -20



https://blog.csdn.net/weixin_61845324/article/details/137828411
https://blog.csdn.net/weixin_61845324/article/details/137831088

WebRTC 回声消除
https://github.com/cpuimage/WebRTC_AECM.git

3A算法文章
https://www.cnblogs.com/cpuimage/category/1147362.html
audio codec
https://github.com/mackron/miniaudio.git

speexsdp消除回声
https://blog.csdn.net/hxchuan000/article/details/136644939?spm=1001.2101.3001.6650.5&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7Ebaidujs_baidulandingword%7ECtr-5-136644939-blog-137929999.235%5Ev43%5Epc_blog_bottom_relevance_base8&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7Ebaidujs_baidulandingword%7ECtr-5-136644939-blog-137929999.235%5Ev43%5Epc_blog_bottom_relevance_base8&utm_relevant_index=8


总结
RNNoise是C语言库，但可以在C++项目中使用
降噪效果比SpeexDSP好很多，因为用了深度学习
专注于降噪+VAD，没有AGC、AEC等功能
适合与SpeexDSP配合使用 - RNNoise负责降噪，SpeexDSP负责AGC

噪音文件下载
https://ecs.utdallas.edu/loizou/speech/noizeus/
https://datashare.ed.ac.uk/handle/10283/2791