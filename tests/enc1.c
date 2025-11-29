
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <va_inf.h>

int
main(int argc, char **argv)
{
    struct va_funcs fns;
    int error;
    int cdata_bytes;
    int ydata_stride_bytes;
    VI_UINTPTR fd;
    char *cdata;
    void *ydata;
    int rfd;

    error = va_inf_get_funcs(&fns, VI_VERSION_INT(VI_MAJOR, VI_MINOR));
    printf("main: va_inf_get_funcs rv %d\n", error);
    if (error == VI_SUCCESS)
    {
        int version;
        error = fns.get_version(&version);
        printf("main: get_version rv %d version 0x%8.8X\n", error, version);
     
        fd = open("/dev/dri/renderD128", O_RDWR);
        rfd = open("/dev/urandom", O_RDONLY);
        printf("main: open fd %ld\n", fd);
        error = fns.init(VI_TYPE_DRM, (void*)fd);

        void *enc;
        error = fns.encoder_create(&enc, 640, 480, VI_TYPE_H264,
                VI_H264_ENC_FLAGS_PROFILE_MAIN);
        printf("main: encoder_create rv %d\n", error);

        if (error == VI_SUCCESS)
        {
            cdata_bytes = 1024 * 1024;
            cdata = (char*)malloc(1024 * 1024);
            error = fns.encoder_get_ybuffer(enc, &ydata, &ydata_stride_bytes);
            printf("main: encoder_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            //memset(ydata, 0xFF, ydata_stride_bytes);
            (void)read(rfd, ydata, ydata_stride_bytes * 1);
            error = fns.encoder_encode(enc, cdata, &cdata_bytes, 0);
            printf("main: encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            free(cdata);

        }

        fns.encoder_delete(enc);

        close(fd);
    }

    return 0;
}