
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <va_inf.h>

struct beef_t
{
    char text[4];
    int width;
    int height;
    int bytes_after;
};

#define DO_BEEF_HEADER 1

/******************************************************************************/
static int
save_data(int fd, const char *cdata, int cdata_bytes, struct beef_t *beef)
{
    int error;
    int llen;

    beef->bytes_after = cdata_bytes;
#if DO_BEEF_HEADER
    llen = sizeof(struct beef_t);
    error = write(fd, beef, llen);
    printf("save_data: write rv %d\n", error);
    if (error != llen) return 1;
#endif
    llen = cdata_bytes;
    error = write(fd, cdata, llen);
    printf("save_data: write rv %d\n", error);
    if (error != llen) return 1;
    return 0;
}

int
main(int argc, char **argv)
{
    struct va_inf_funcs fns;
    int llen;
    int error;
    int cdata_bytes;
    int ydata_stride_bytes;
    int uvdata_stride_bytes;
    VI_UINTPTR dev_fd;
    char *cdata;
    void *ydata;
    void *uvdata;
    int rfd;
    int wfd;
    int flags;
    void *enc;
    void *sur;
    struct beef_t beef;

    int fd;
    int fd_width;
    int fd_height;
    int fd_stride;
    int fd_size;
    int fd_bpp;

    (void)argc;
    (void)argv;

    error = va_inf_get_funcs(&fns, VI_VERSION_INT(VI_MAJOR, VI_MINOR));
    printf("main: va_inf_get_funcs rv %d\n", error);
    if (error == VI_SUCCESS)
    {
        int version;
        error = fns.get_version(&version);
        printf("main: get_version rv %d version 0x%8.8X\n", error, version);
     
        dev_fd = open("/dev/dri/renderD128", O_RDWR);
        printf("main: open fd %ld\n", dev_fd);
        rfd = open("test_nv12_640x480.yuv", O_RDONLY);
        printf("main: open rfd %d\n", rfd);
#if DO_BEEF_HEADER
        wfd = open("out.beef", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
#else
        wfd = open("out.h264", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
#endif
        printf("main: open wfd %d\n", wfd);
        error = fns.init(VI_TYPE_DRM, (void*)dev_fd);

        enc = NULL;
        //error = fns.encoder_create(&enc, 640, 480, VI_TYPE_H264, 0);
        error = fns.encoder_create(&enc, 640, 480, VI_TYPE_H264, VI_H264_ENC_FLAGS_PROFILE_MAIN);
        //error = fns.encoder_create(&enc, 640, 480, VI_TYPE_H264, VI_H264_ENC_FLAGS_PROFILE_HIGH);
        printf("main: encoder_create rv %d\n", error);
        if (error != 0) return 1;

        sur = NULL;
        error = fns.surface_create(&sur, 640, 480, VI_BGRX, 0);
        //error = fns.surface_create(&sur, 640, 480, VI_YUY2, 0);
        //error = fns.surface_create(&sur, 640, 480, VI_NV12, 0);
        printf("main: surface_create rv %d\n", error);
        if (error != 0) return 1;

        if (error == VI_SUCCESS)
        {
            cdata = (char*)malloc(1024 * 1024);
            if (cdata == NULL) return 1;

            beef.text[0] = 'B';
            beef.text[1] = 'E';
            beef.text[2] = 'E';
            beef.text[3] = 'F';
            beef.width = 640;
            beef.height = 480;

            flags = 0;
            //flags |= VI_H264_ENC_FLAG_KEYFRAME;

            // frame 1
            error = fns.surface_get_ybuffer(sur, &ydata, &ydata_stride_bytes);
            printf("main: surface_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            llen = ydata_stride_bytes * 480;
            error = read(rfd, ydata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.surface_get_uvbuffer(sur, &uvdata, &uvdata_stride_bytes);
            printf("main: surface_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            llen = uvdata_stride_bytes * (480 / 2);
            error = read(rfd, uvdata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.surface_get_fd_dst(sur, &fd, &fd_width, & fd_height, &fd_stride, &fd_size, &fd_bpp);
            printf("main: surface_get_fd_dst rv %d fd_bpp 0x%X\n", error, fd_bpp);
            if (error != 0) return 1;
            error = fns.encoder_set_fd_src(enc, fd, fd_width, fd_height, fd_stride, fd_size, fd_bpp);
            printf("main: encoder_set_fd_src rv %d\n", error);
            if (error != 0) return 1;
            cdata_bytes = 1024 * 1024;
            error = fns.encoder_encode(enc, cdata, &cdata_bytes, flags);
            printf("main: encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = save_data(wfd, cdata, cdata_bytes, &beef);
            if (error != 0) return 1;

            // frame 2
            error = fns.surface_get_ybuffer(sur, &ydata, &ydata_stride_bytes);
            printf("main: surface_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            llen = ydata_stride_bytes * 480;
            error = read(rfd, ydata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.surface_get_uvbuffer(sur, &uvdata, &uvdata_stride_bytes);
            printf("main: surface_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            llen = uvdata_stride_bytes * (480 / 2);
            error = read(rfd, uvdata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.surface_get_fd_dst(sur, &fd, &fd_width, & fd_height, &fd_stride, &fd_size, &fd_bpp);
            printf("main: surface_get_fd_dst rv %d fd_bpp 0x%X\n", error, fd_bpp);
            if (error != 0) return 1;
            error = fns.encoder_set_fd_src(enc, fd, fd_width, fd_height, fd_stride, fd_size, fd_bpp);
            printf("main: encoder_set_fd_src rv %d\n", error);
            if (error != 0) return 1;
            cdata_bytes = 1024 * 1024;
            error = fns.encoder_encode(enc, cdata, &cdata_bytes, flags);
            printf("main: encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = save_data(wfd, cdata, cdata_bytes, &beef);
            if (error != 0) return 1;

            // frame 3
            error = fns.surface_get_ybuffer(sur, &ydata, &ydata_stride_bytes);
            printf("main: surface_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            llen = ydata_stride_bytes * 480;
            error = read(rfd, ydata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.surface_get_uvbuffer(sur, &uvdata, &uvdata_stride_bytes);
            printf("main: surface_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            llen = uvdata_stride_bytes * (480 / 2);
            error = read(rfd, uvdata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.surface_get_fd_dst(sur, &fd, &fd_width, & fd_height, &fd_stride, &fd_size, &fd_bpp);
            printf("main: surface_get_fd_dst rv %d fd_bpp 0x%X\n", error, fd_bpp);
            if (error != 0) return 1;
            error = fns.encoder_set_fd_src(enc, fd, fd_width, fd_height, fd_stride, fd_size, fd_bpp);
            printf("main: encoder_set_fd_src rv %d\n", error);
            if (error != 0) return 1;
            cdata_bytes = 1024 * 1024;
            error = fns.encoder_encode(enc, cdata, &cdata_bytes, flags);
            printf("main: encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = save_data(wfd, cdata, cdata_bytes, &beef);
            if (error != 0) return 1;

            // frame 4
            error = fns.surface_get_ybuffer(sur, &ydata, &ydata_stride_bytes);
            printf("main: surface_get_ybuffer rv %d ydata_stride_bytes %d \n", error, ydata_stride_bytes);
            if (error != 0) return 1;
            llen = ydata_stride_bytes * 480;
            error = read(rfd, ydata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.surface_get_uvbuffer(sur, &uvdata, &uvdata_stride_bytes);
            printf("main: surface_get_uvbuffer rv %d uvdata_stride_bytes %d \n", error, uvdata_stride_bytes);
            if (error != 0) return 1;
            llen = uvdata_stride_bytes * (480 / 2);
            error = read(rfd, uvdata, llen);
            printf("main: read rv %d\n", error);
            if (error != llen) return 1;
            error = fns.surface_get_fd_dst(sur, &fd, &fd_width, & fd_height, &fd_stride, &fd_size, &fd_bpp);
            printf("main: surface_get_fd_dst rv %d fd_bpp 0x%X\n", error, fd_bpp);
            if (error != 0) return 1;
            error = fns.encoder_set_fd_src(enc, fd, fd_width, fd_height, fd_stride, fd_size, fd_bpp);
            printf("main: encoder_set_fd_src rv %d\n", error);
            if (error != 0) return 1;
            cdata_bytes = 1024 * 1024;
            error = fns.encoder_encode(enc, cdata, &cdata_bytes, flags);
            printf("main: encoder_encode rv %d cdata_bytes %d\n", error, cdata_bytes);
            if (error != 0) return 1;
            error = save_data(wfd, cdata, cdata_bytes, &beef);
            if (error != 0) return 1;

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
