# ffmpegEncode

1- Build it with below command:
gcc -std=c++11 ffmpegEncodeFromYuvFile.cpp -o ffmpegEncodeFromYuvFile -lavcodec -lavutil -lstdc++

2- It will take following parameter: <input yuv file> <output file name> <encoding format> 
 Example: ffmpegEncodeFromYuvFile input1920x1080.yuv output.mjpeg mjpeg
