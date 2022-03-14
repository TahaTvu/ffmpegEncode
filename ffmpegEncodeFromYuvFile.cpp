

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <chrono>
#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

//#include <vector>
using namespace std::chrono;

#define WIDTH 1920
#define HEIGHT 1080
//
//struct EncodeFrame {
//    unsigned char* frame{ nullptr };
//    int size{ 0 };
//    EncodeFrame(int size_, unsigned char* frame_) :size{ size_ }, frame{ new unsigned char[size] }{       
//        memcpy(frame, frame_, size);
//    }
//};
//
//std::vector<EncodeFrame> vframe;

static void encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt,
    FILE* outfile)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %d \n", frame->pts);

    int64_t ms_in = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::time_t incoming = std::time(nullptr);
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        }
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        int64_t ms_out = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
     //   std::time_t outgoing = std::time(nullptr);
       std::cout << "Incoming time : " << ms_in << ", Outgoing time: " << ms_out <<", Encoding time: " << ms_out - ms_in <<std::endl;
    //    printf("Incoming time: %ld, Outgoing time: %ld, Encoding time: %d \n", ms_in, ms_out, ms_out - ms_in);
        printf("Write packet %d  and packet size %d\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        //vframe.push_back(EncodeFrame(pkt->size, pkt->data));
        av_packet_unref(pkt);
    }
}

int main(int argc, char** argv)
{
    const char* filename, *inputfile, * codec_name;
    const AVCodec* codec;
    AVCodecContext* c = NULL;
    int i, ret, x, y;
    FILE* f;
    AVFrame* frame;
    AVPacket* pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    //avcodec_register_all();
    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <output file> <codec name>\n", argv[0]);
        exit(0);
    }

    inputfile = argv[1];
    filename = argv[2];
    codec_name = argv[3];

    std::cout<<inputfile << std::endl;
    std::cout<<filename << std::endl;
    std::cout<<codec_name << std::endl;
    /* find the mpeg1video encoder */
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    pkt = av_packet_alloc();
    if (!pkt){
	std::cout<<"ERROR: Pkt not alloc.." <<std::endl;
        exit(1);
    }
    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = WIDTH;//352;
    c->height = HEIGHT;//288;
    /* frames per second */
    c->time_base.num = 1;// = (AVRational){ 1, 25 };
    c->time_base.den = 25;
    c->framerate.num = 25;// (AVRational) { 25, 1 };
    c->framerate.den = 1;

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    //c->gop_size = 10;
    //c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec->id == AV_CODEC_ID_H264) {
        c->gop_size = 10;
        c->max_b_frames = 1;
        av_opt_set(c->priv_data, "preset", "slow", 0);
    }
    else if (codec->id == AV_CODEC_ID_MJPEG) {
        c->pix_fmt = AV_PIX_FMT_YUVJ420P;
        c->codec_type = AVMEDIA_TYPE_VIDEO;
        fprintf(stdin, "MJPEG Encoder....\n");
    }

    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        //fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
        exit(1);
    }

     f = fopen(filename, "wb");
    //fopen_s(&f, filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    int ysizeFrame = WIDTH * HEIGHT;
    int uvsizeFrame = (WIDTH / 2) * (HEIGHT / 2);
    unsigned char* yframebuf = new unsigned char[ysizeFrame];
    unsigned char* uframebuf = new unsigned char[uvsizeFrame];
    unsigned char* vframebuf = new unsigned char[uvsizeFrame];

    FILE* fp;
    //fopen_s(&fp, inputfile, "rb");
    fp = fopen(inputfile, "rb");
    i = 0 ;
    
    while (fread(yframebuf, 1, ysizeFrame, fp) == ysizeFrame) {
        if (fread(uframebuf, 1, uvsizeFrame, fp) != uvsizeFrame){
		std::cout<< "ERROR in reading usize.... \n";
	}
	if (fread(vframebuf, 1, uvsizeFrame, fp) != uvsizeFrame){
		std::cout<< "ERROR in reading vsize ... \n";
	}

        fflush(stdout);

        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);

        // Copy data from the 3 input buffers
        memcpy(frame->data[0], yframebuf, frame->linesize[0] * frame->height);
        memcpy(frame->data[1], uframebuf, frame->linesize[1] * frame->height / 2);
        memcpy(frame->data[2], vframebuf, frame->linesize[2] * frame->height / 2);

        frame->pts = i;
        i++;
        /* encode the image */
        encode(c, frame, pkt, f);
    }
    delete []yframebuf;
    delete []uframebuf;
    delete []vframebuf;

    fclose(fp);
    
    /* flush the encoder */
    encode(c, NULL, pkt, f);

    /* Add sequence end code to have a real MPEG file.
       It makes only sense because this tiny examples writes packets
       directly. This is called "elementary stream" and only works for some
       codecs. To create a valid file, you usually need to write packets
       into a proper file format or protocol; see muxing.c.
     */
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}
