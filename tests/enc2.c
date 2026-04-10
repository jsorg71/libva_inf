
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <va_inf.h>

#include <wels/codec_api.h>

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

int
main(int argc, char **argv)
{
    struct va_funcs fns;
    int llen;
    int error;
    int cdata_bytes;
    int ydata_stride_bytes;
    int uvdata_stride_bytes;
    VI_UINTPTR fd;
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

    error = va_inf_get_funcs(&fns, VI_VERSION_INT(VI_MAJOR, VI_MINOR));
    printf("main: va_inf_get_funcs rv %d\n", error);
    if (error == VI_SUCCESS)
    {
        int version;
        error = fns.get_version(&version);
        printf("main: get_version rv %d version 0x%8.8X\n", error, version);

        fd = open("/dev/dri/renderD128", O_RDWR);
        printf("main: open fd %ld\n", fd);
        rfd = open("test_nv12_640x480.yuv", O_RDONLY);
        printf("main: open rfd %d\n", rfd);
        wfd = open("out.h264", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
        printf("main: open wfd %d\n", wfd);
        void *enc = NULL;
        error = fns.init(VI_TYPE_DRM, (void*)fd);
        //error = fns.encoder_create(&enc, 640, 480, VI_TYPE_H264, 0);
        error = fns.encoder_create(&enc, 640, 480, VI_TYPE_H264, VI_H264_ENC_FLAGS_PROFILE_MAIN);
        //error = fns.encoder_create(&enc, 640, 480, VI_TYPE_H264, VI_H264_ENC_FLAGS_PROFILE_HIGH);
        printf("main: encoder_create rv %d\n", error);

        if (error == VI_SUCCESS)
        {
            cdata_bytes = 1024 * 1024;
            cdata = (unsigned char*)malloc(1024 * 1024);
            if (cdata == NULL) return 1;

            flags = 0;
            flags |= VI_H264_ENC_FLAG_KEYFRAME;

            // frame 1
            error = fns.encoder_get_ybuffer(enc, &ydata, &ydata_stride_bytes);
            printf("main: encoder_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            llen = ydata_stride_bytes * 480;
            error = read(rfd, ydata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.encoder_get_uvbuffer(enc, &uvdata, &uvdata_stride_bytes);
            printf("main: encoder_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            llen = uvdata_stride_bytes * (480 / 2);
            error = read(rfd, uvdata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.encoder_encode(enc, cdata, &cdata_bytes, flags);
            printf("main: encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = decode_single_frame(oh264Decoder, cdata, cdata_bytes, targetBuffer1, &targetInfo);
            printf("main: decode_single_frame rv %d\n", error);
            printf("\n");

            // frame 2
            error = fns.encoder_get_ybuffer(enc, &ydata, &ydata_stride_bytes);
            printf("main: encoder_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            llen = ydata_stride_bytes * 480;
            error = read(rfd, ydata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.encoder_get_uvbuffer(enc, &uvdata, &uvdata_stride_bytes);
            printf("main: encoder_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            llen = uvdata_stride_bytes * (480 / 2);
            error = read(rfd, uvdata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            cdata_bytes = 1024 * 1024;
            error = fns.encoder_encode(enc, cdata, &cdata_bytes, flags);
            printf("main: encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = decode_single_frame(oh264Decoder, cdata, cdata_bytes, targetBuffer1, &targetInfo);
            printf("main: decode_single_frame rv %d\n", error);
            printf("\n");

            // frame 3
            error = fns.encoder_get_ybuffer(enc, &ydata, &ydata_stride_bytes);
            printf("main: encoder_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            error = read(rfd, ydata, ydata_stride_bytes * 480);
            printf("main: read rv %d\n", error);
            error = fns.encoder_get_uvbuffer(enc, &uvdata, &uvdata_stride_bytes);
            printf("main: encoder_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            error = read(rfd, uvdata, uvdata_stride_bytes * (480 / 2));
            printf("main: read rv %d\n", error);
            cdata_bytes = 1024 * 1024;
            error = fns.encoder_encode(enc, cdata, &cdata_bytes, flags);
            printf("main: encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = decode_single_frame(oh264Decoder, cdata, cdata_bytes, targetBuffer1, &targetInfo);
            printf("main: decode_single_frame rv %d\n", error);
            printf("\n");

            // frame 4
            error = fns.encoder_get_ybuffer(enc, &ydata, &ydata_stride_bytes);
            printf("main: encoder_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            error = read(rfd, ydata, ydata_stride_bytes * 480);
            printf("main: read rv %d\n", error);
            error = fns.encoder_get_uvbuffer(enc, &uvdata, &uvdata_stride_bytes);
            printf("main: encoder_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            error = read(rfd, uvdata, uvdata_stride_bytes * (480 / 2));
            printf("main: read rv %d\n", error);
            cdata_bytes = 1024 * 1024;
            error = fns.encoder_encode(enc, cdata, &cdata_bytes, flags);
            printf("main: encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = decode_single_frame(oh264Decoder, cdata, cdata_bytes, targetBuffer1, &targetInfo);
            printf("main: decode_single_frame rv %d\n", error);
            printf("\n");

            free(cdata);
        }

        if (enc != NULL)
        {
            fns.encoder_delete(enc);
        }

        close(fd);
    }

    return 0;
}
