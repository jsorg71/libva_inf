#if !defined( __VA_INF_H__)
#define __VA_INF_H__ 1

#if !defined(VI_INT64)
#define VI_INT64 long long
#endif

#if !defined(VI_UINTPTR)
#define VI_UINTPTR size_t
#endif

#define VI_VERSION_INT(_major, _minor) (((_major) << 16) | (_minor))

#define VI_MAJOR                        0
#define VI_MINOR                        0

#define VI_SUCCESS                      0
#define VI_ERROR_MEMORY                 1
#define VI_ERROR_UNIMP                  2
#define VI_ERROR_TYPE                   3
#define VI_ERROR_OTHER                  4

struct va_funcs
{
    int (*get_version)(int *version);
    int (*init)(int type, void *display);
    int (*deinit)(void);
    VI_UINTPTR pad0[20 - 3];
    /* encoder */
    int (*encoder_create)(void **obj, int width, int height, int type, int flags);
    int (*encoder_delete)(void *obj);
    int (*encoder_get_width)(void *obj, int *width);
    int (*encoder_get_height)(void *obj, int *height);
    int (*encoder_resize)(void *obj, int width, int height);
    int (*encoder_get_ybuffer)(void *obj, void **ydata, int *ydata_stride_bytes);
    int (*encoder_get_uvbuffer)(void *obj, void **uvdata, int *uvdata_stride_bytes);
    int (*encoder_set_fd_src)(void *obj, int fd, int fd_width, int fd_height,
                              int fd_stride, int fd_size, int fd_bpp);
    int (*encoder_encode)(void *obj, void *cdata, int *cdata_max_bytes);
    int (*encoder_encode_flags)(void *obj, void *cdata, int *cdata_max_bytes, int flags);
    VI_UINTPTR pad1[20 - 10];
};

typedef int
(*va_inf_get_funcs_proc)(struct va_funcs *funcs, int version);
int
va_inf_get_funcs(struct va_funcs *funcs, int version);

#endif
