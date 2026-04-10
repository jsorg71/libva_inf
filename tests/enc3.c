
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <yami_inf.h>

#include <wels/codec_api.h>

#if 1
int
decode_single_frame(ISVCDecoder* oh264Decoder, unsigned char *cdata,
        int cdata_bytes, unsigned char **targetBuffer,
        SBufferInfo *targetInfo)
{
    int error;

    error = (*oh264Decoder)->DecodeFrameNoDelay(oh264Decoder, cdata,
            cdata_bytes, targetBuffer, targetInfo);
    printf("decode_single_frame: DecodeFrameNoDelay error %d\n", error);
    if (error == 0)
    {
        printf("main: iBufferStatus %d\n", targetInfo->iBufferStatus);
    }
    if ((error == 0) && (targetInfo->iBufferStatus == 1))
    {
        return 0;
    }
    return 1;
}
#endif

#if 0
int
decode_single_frame(ISVCDecoder* oh264Decoder, unsigned char *cdata,
        int cdata_bytes, unsigned char **targetBuffer,
        SBufferInfo *targetInfo)
{
    int error;

    error = (*oh264Decoder)->DecodeFrame2(oh264Decoder, cdata,
            cdata_bytes, targetBuffer, targetInfo);
    printf("decode_single_frame: DecodeFrame2 error %d\n", error);
    if ((error == 0) && (targetInfo->iBufferStatus == 0))
    {
        error = (*oh264Decoder)->DecodeFrame2(oh264Decoder, NULL, 0,
                targetBuffer, targetInfo);
    }
    if (error == 0)
    {
        printf("decode_single_frame: iBufferStatus %d\n", targetInfo->iBufferStatus);
    }
    if ((error == 0) && (targetInfo->iBufferStatus == 1))
    {
        return 0;
    }
    return 1;
}
    #endif

int
main(int argc, char **argv)
{
    struct yami_funcs fns;
    int llen;
    int error;
    int cdata_bytes;
    int ydata_stride_bytes;
    int uvdata_stride_bytes;
    int fd;
    unsigned char *cdata;
    void *ydata;
    void *uvdata;
    int rfd;
    int wfd;
    int flags;
    ISVCDecoder* oh264Decoder;
    SDecodingParam oh264DecoderParam;
    SBufferInfo targetInfo;
    unsigned char *targetBuffer[3];
    unsigned char **targetBuffer1;

    (void)argc;
    (void)argv;

    targetBuffer1 = (unsigned char **) (&targetBuffer);

    error = WelsCreateDecoder(&oh264Decoder);
    printf("main: WelsCreateDecoder error %d\n", error);

    memset(&oh264DecoderParam, 0, sizeof(SDecodingParam));
    oh264DecoderParam.uiTargetDqLayer = 255;
    oh264DecoderParam.eEcActiveIdc = ERROR_CON_DISABLE;
    oh264DecoderParam.sVideoProperty.size = sizeof(oh264DecoderParam.sVideoProperty);
    oh264DecoderParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
    error = (*oh264Decoder)->Initialize(oh264Decoder, &oh264DecoderParam);

    error = yami_get_funcs(&fns, YI_VERSION_INT(YI_MAJOR, YI_MINOR));
    printf("main: yami_get_funcs rv %d\n", error);
    if (error == YI_SUCCESS)
    {
        int version;
        error = fns.yami_get_version(&version);
        printf("main: yami_get_version rv %d version 0x%8.8X\n", error, version);

        fd = open("/dev/dri/renderD128", O_RDWR);
        printf("main: open fd %d\n", fd);
        rfd = open("test_nv12_640x480.yuv", O_RDONLY);
        printf("main: open rfd %d\n", rfd);
        wfd = open("out.h264", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
        printf("main: open wfd %d\n", wfd);
        void *enc = NULL;
        error = fns.yami_init(YI_TYPE_DRM, (void*)(long)fd);
        //error = fns.yami_encoder_create(&enc, 640, 480, YI_TYPE_H264, 0);
        //error = fns.yami_encoder_create(&enc, 640, 480, YI_TYPE_H264, YI_H264_ENC_FLAGS_PROFILE_MAIN);
        error = fns.yami_encoder_create(&enc, 640, 480, YI_TYPE_H264, YI_H264_ENC_FLAGS_PROFILE_HIGH);
        printf("main: yami_encoder_create rv %d\n", error);

        if (error == YI_SUCCESS)
        {
            cdata_bytes = 1024 * 1024;
            cdata = (unsigned char*)malloc(1024 * 1024);
            if (cdata == NULL) return 1;

            flags = 0;
            //flags |= YI_H264_ENC_FLAG_KEYFRAME;

            // frame 1
            error = fns.yami_encoder_get_ybuffer(enc, &ydata, &ydata_stride_bytes);
            printf("main: yami_encoder_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            llen = ydata_stride_bytes * 480;
            error = read(rfd, ydata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.yami_encoder_get_uvbuffer(enc, &uvdata, &uvdata_stride_bytes);
            printf("main: yami_encoder_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            llen = uvdata_stride_bytes * (480 / 2);
            error = read(rfd, uvdata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.yami_encoder_encode(enc, cdata, &cdata_bytes);
            printf("main: yami_encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = decode_single_frame(oh264Decoder, cdata, cdata_bytes, targetBuffer1, &targetInfo);
            printf("main: decode_single_frame rv %d\n", error);
            printf("\n");

            // frame 2
            error = fns.yami_encoder_get_ybuffer(enc, &ydata, &ydata_stride_bytes);
            printf("main: yami_encoder_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            llen = ydata_stride_bytes * 480;
            error = read(rfd, ydata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.yami_encoder_get_uvbuffer(enc, &uvdata, &uvdata_stride_bytes);
            printf("main: yami_encoder_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            llen = uvdata_stride_bytes * (480 / 2);
            error = read(rfd, uvdata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            cdata_bytes = 1024 * 1024;
            error = fns.yami_encoder_encode(enc, cdata, &cdata_bytes);
            printf("main: yami_encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = decode_single_frame(oh264Decoder, cdata, cdata_bytes, targetBuffer1, &targetInfo);
            printf("main: decode_single_frame rv %d\n", error);
            printf("\n");

            // frame 3
            error = fns.yami_encoder_get_ybuffer(enc, &ydata, &ydata_stride_bytes);
            printf("main: yami_encoder_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            error = read(rfd, ydata, ydata_stride_bytes * 480);
            printf("main: read rv %d\n", error);
            error = fns.yami_encoder_get_uvbuffer(enc, &uvdata, &uvdata_stride_bytes);
            printf("main: yami_encoder_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            error = read(rfd, uvdata, uvdata_stride_bytes * (480 / 2));
            printf("main: read rv %d\n", error);
            cdata_bytes = 1024 * 1024;
            error = fns.yami_encoder_encode(enc, cdata, &cdata_bytes);
            printf("main: yami_encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = decode_single_frame(oh264Decoder, cdata, cdata_bytes, targetBuffer1, &targetInfo);
            printf("main: decode_single_frame rv %d\n", error);
            printf("\n");

            // frame 4
            error = fns.yami_encoder_get_ybuffer(enc, &ydata, &ydata_stride_bytes);
            printf("main: yami_encoder_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            error = read(rfd, ydata, ydata_stride_bytes * 480);
            printf("main: read rv %d\n", error);
            error = fns.yami_encoder_get_uvbuffer(enc, &uvdata, &uvdata_stride_bytes);
            printf("main: yami_encoder_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            error = read(rfd, uvdata, uvdata_stride_bytes * (480 / 2));
            printf("main: read rv %d\n", error);
            cdata_bytes = 1024 * 1024;
            error = fns.yami_encoder_encode(enc, cdata, &cdata_bytes);
            printf("main: yami_encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = decode_single_frame(oh264Decoder, cdata, cdata_bytes, targetBuffer1, &targetInfo);
            printf("main: decode_single_frame rv %d\n", error);
            printf("\n");

            free(cdata);
        }

        if (enc != NULL)
        {
            fns.yami_encoder_delete(enc);
        }

        close(fd);
    }

    return 0;
}
