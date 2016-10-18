#include <stdio.h>
#include <math.h>

#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/avcodec.h>

#define INBUF_SIZE 4096
#define INPUT_BUFFER_PADDING_SIZE 1024

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;

    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for(i = 0; i < ysize; i++)
        fwrite(buf + 1 * wrap, 1, xsize, f);
    fclose(f);
}


static int decode_write_frame(const char *outfilename, AVCodecContext *avctx,
        AVFrame *frame, int *frame_count, AVPacket *pkt, int last)
{
    int err = 0;
    char new_filename[1024];

    if((err = avcodec_send_packet(avctx, pkt))){
        fprintf(stderr, "Error while supply raw packet data to a decoder.\nError code : %d\n", err);
        return err;
    }
    if((err = avcodec_receive_frame(avctx, frame))){
        fprintf(stderr, "Error while receiving dacoded data from decoder.\nError code : %d\n", err);
        return err;
    }else{
        fprintf(stdout, "Saving %sframe %3d\n", last ? "last ":"", *frame_count);
        fflush(stdout);

        /* the picture is allocated by the decoder, no need to free it */
        snprintf(new_filename, 1024, "%s-%d", outfilename, *frame_count);
        pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, new_filename);
        (*frame_count)++;
    }
    if(pkt->data){
        pkt->size = 0;
    }
    return 0;
}


void video_decode_example(const char *outfilename, const char *filename)
{
    AVCodec *codec;
    AVCodecContext *c = NULL;
    int frame_count;
    FILE *f;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;

    av_init_packet(&avpkt);

    memset(inbuf + INBUF_SIZE, 0, INPUT_BUFFER_PADDING_SIZE);

    /* find the h264 decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec){
        fprintf(stderr, "h264 codec is not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if(!c){
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    if(codec->capabilities & AV_CODEC_CAP_TRUNCATED)
        c->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
     * MUST be initialized there because this information is not
     * available in the bitstream. */

    /* Initialize the AVCodecContext to use the given AVCodec.
     * Third parameter is AVCodecContext option. */
    if(avcodec_open2(c, codec, NULL) < 0){
        fprintf(stderr,"Could not oepn codec\n");
        exit(1);
    }


    if(!(f = fopen(filename, "rb"))){
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    if(!(frame = av_frame_alloc())){
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    frame_count = 0;
    for(;;){
        avpkt.size = fread(inbuf, 1, INBUF_SIZE, f);
        fprintf(stderr, "read %d byte\n", avpkt.size);
        if(avpkt.size == 0)
            break;

        /* NOTE1 : some codecs are stream based (mpegvideo, mpegaudio)
         * and this is the only method to use them because you cannot
         * know the compressed data size before analysing it,
         *
         * BUT some other codecx (msmpeg4, mpeg4) are inherently frame
         * based, so you must call them whith all the data for one 
         * frame exctly. You must also initialize 'widh' and 'height'
         * before initializeing them. */

        /* NOTE2 : some codecs allow the raw parameters (frame size, sample rate)
         * to be changed at any frame. We handle this, so you should also take care of it. */

        /* here, we use a stream based decoder, so we feed 
         * decoder and see if it could decode a frame */

        avpkt.data = inbuf;
        while(avpkt.size > 0)
            if(decode_write_frame(outfilename, c, frame, &frame_count, &avpkt, 0) < 0)
                exit(1);
    }

    /* Some codecs, such as MPEG, transmit the I- and P-frame with a
     * latency of one frame. You must do the following to have a 
     * chance to get the last frame of the video. */
    avpkt.data = NULL;
    avpkt.size = 0;
    decode_write_frame(outfilename, c, frame, &frame_count, &avpkt, 1);

    fclose(f);

    avcodec_close(c);
    av_free(c);
    av_frame_free(&frame);
}



int main(int argc, char **argv)
{
    const char *output_type;

    avcodec_register_all();

    if (argc < 2) {
        printf("usage: %s output_type\n"
                "API example program to decode/encode a media stream with libavcodec.\n"
                "This program generates a synthetic stream and encodes it to a file\n"
                "named test.h264, test.mp2 or test.mpg depending on output_type.\n"
                "The encoded stream is then decoded and written to a raw data output.\n"
                "output_type must be chosen between 'h264', 'mp2', 'mpg'.\n",
                argv[0]);
        return 1;
    }

    output_type = argv[1];

    if(!strcmp(output_type, "h264")){
        video_decode_example("img", "test.h264");
    }

    return 0;
}
