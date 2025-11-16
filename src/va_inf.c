
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <va/va.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>

#include "va_inf.h"

static VADisplay g_va_display = NULL;
static VAConfigID g_vp_config = VA_INVALID_ID;
static VAContextID g_vp_context = VA_INVALID_ID;

struct va_inf_enc_priv
{
    int width;
    int height;
    int type;
    int flags;
    VASurfaceID va_surface[1];
    VASurfaceID fd_va_surface[1];
    int fd_yami_fourcc;
    int pad0;
    VAImage va_image[1];
    char* yuvdata;
};

/*****************************************************************************/
static int
va_inf_get_version(int* version)
{
    *version = VI_VERSION_INT(VI_MAJOR, VI_MINOR);
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_init(int type, void* display)
{
    int fd;
    int major_version;
    int minor_version;
    VAStatus va_status;

    if (type == VI_TYPE_DRM)
    {
        fd = (int) (VI_UINTPTR)display;
        g_va_display = vaGetDisplayDRM(fd);
    }
    else
    {
        return VI_ERROR_TYPE;
    }
    va_status = vaInitialize(g_va_display, &major_version, &minor_version);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VAINITIALIZE;
    }
    va_status = vaCreateConfig(g_va_display, VAProfileNone,
            VAEntrypointVideoProc, NULL, 0, &g_vp_config);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VACREATECONFIG;
    }
    va_status = vaCreateContext(g_va_display, g_vp_config, 0, 0, 0, NULL, 0,
            &g_vp_context);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VACREATECONTEXT;
    }
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_deinit(void)
{
    vaDestroyContext(g_va_display, g_vp_context);
    g_vp_context = VA_INVALID_ID;
    vaDestroyConfig(g_va_display, g_vp_config);
    g_vp_config = VA_INVALID_ID;
    vaTerminate(g_va_display);
    g_va_display = 0;
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_encoder_create(void** obj, int width, int height, int type, int flags)
{
    struct va_inf_enc_priv* enc;
    VASurfaceAttrib attribs[1];
    VAStatus va_status;
    VAImageFormat va_image_format;

    enc = (struct va_inf_enc_priv*)
          calloc(1, sizeof(struct va_inf_enc_priv));
    if (enc == NULL)
    {
        return VI_ERROR_MEMORY;
    }
    if (type == VI_TYPE_H264)
    {
    }
    else
    {
        free(enc);
        return VI_ERROR_TYPE;
    }
    memset(attribs, 0, sizeof(attribs));
    attribs[0].type = VASurfaceAttribPixelFormat;
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].value.type = VAGenericValueTypeInteger;
    attribs[0].value.value.i = VA_FOURCC_NV12;
    va_status = vaCreateSurfaces(g_va_display, VA_RT_FORMAT_YUV420,
                                 width, height,
                                 enc->va_surface, 1, attribs, 1);
    if (va_status != VA_STATUS_SUCCESS)
    {
        free(enc);
        return VI_ERROR_VACREATESURFACES;
    }
    memset(&va_image_format, 0, sizeof(va_image_format));
    va_image_format.fourcc = VA_FOURCC_NV12;
    va_status = vaCreateImage(g_va_display, &va_image_format,
                              width, height, enc->va_image);
    if (va_status != VA_STATUS_SUCCESS)
    {
        vaDestroySurfaces(g_va_display, enc->va_surface, 1);
        free(enc);
        return VI_ERROR_VACREATEIMAGE;
    }
    enc->width = width;
    enc->height = height;
    enc->type = type;
    enc->flags = flags;
    enc->fd_va_surface[0] = VA_INVALID_SURFACE;
    *obj = enc;
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_encoder_delete(void* obj)
{
    struct va_inf_enc_priv* enc;

    enc = (struct va_inf_enc_priv*)obj;
    if (enc == NULL)
    {
        return VI_SUCCESS;
    }
    vaDestroyImage(g_va_display, enc->va_image[0].image_id);
    vaDestroySurfaces(g_va_display, enc->va_surface, 1);
    free(enc);
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_encoder_get_width(void* obj, int* width)
{
    struct va_inf_enc_priv* enc;

    enc = (struct va_inf_enc_priv*)obj;
    *width = enc->width;
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_encoder_get_height(void* obj, int* height)
{
    struct va_inf_enc_priv* enc;

    enc = (struct va_inf_enc_priv*)obj;
    *height = enc->height;
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_encoder_resize(void* obj, int width, int height)
{
    struct va_inf_enc_priv* enc;
    VAStatus va_status;
    VAImageFormat va_image_format;
    VASurfaceAttrib attribs[1];

    enc = (struct va_inf_enc_priv*)obj;

    va_status = vaDestroyImage(g_va_display, enc->va_image[0].image_id);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VADESTROYIMAGE;
    }
    va_status = vaDestroySurfaces(g_va_display, enc->va_surface, 1);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VADESTROYSURFACES;
    }
    memset(attribs, 0, sizeof(attribs));
    attribs[0].type = VASurfaceAttribPixelFormat;
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].value.type = VAGenericValueTypeInteger;
    attribs[0].value.value.i = VA_FOURCC_NV12;
    va_status = vaCreateSurfaces(g_va_display, VA_RT_FORMAT_YUV420,
                                 width, height,
                                 enc->va_surface, 1, attribs, 1);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VACREATESURFACES;
    }
    memset(&va_image_format, 0, sizeof(va_image_format));
    va_image_format.fourcc = VA_FOURCC_NV12;
    va_status = vaCreateImage(g_va_display, &va_image_format,
                              width, height, enc->va_image);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VACREATEIMAGE;
    }
    if (enc->fd_va_surface[0] != VA_INVALID_SURFACE)
    {
        va_status = vaDestroySurfaces(g_va_display, enc->fd_va_surface, 1);
        if (va_status != VA_STATUS_SUCCESS)
        {
            return VI_ERROR_VACREATESURFACES;
        }
        enc->fd_va_surface[0] = VA_INVALID_SURFACE;
    }
    enc->yuvdata = NULL;
    enc->width = width;
    enc->height = height;
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_encoder_get_ybuffer(void* obj, void** ydata, int* ydata_stride_bytes)
{
    struct va_inf_enc_priv* enc;
    VAStatus va_status;
    void* buf_ptr;

    enc = (struct va_inf_enc_priv*)obj;
    if (enc->yuvdata == NULL)
    {
        va_status = vaMapBuffer(g_va_display, enc->va_image[0].buf, &buf_ptr);
        if (va_status != VA_STATUS_SUCCESS)
        {
            return VI_ERROR_VAMAPBUFFER;
        }
        enc->yuvdata = (char*)buf_ptr;
    }
    *ydata = enc->yuvdata + enc->va_image[0].offsets[0];
    *ydata_stride_bytes = enc->va_image[0].pitches[0];
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_encoder_get_uvbuffer(void* obj, void** uvdata, int* uvdata_stride_bytes)
{
    struct va_inf_enc_priv* enc;
    VAStatus va_status;
    void* buf_ptr;

    enc = (struct va_inf_enc_priv*)obj;
    if (enc->yuvdata == NULL)
    {
        va_status = vaMapBuffer(g_va_display, enc->va_image[0].buf, &buf_ptr);
        if (va_status != VA_STATUS_SUCCESS)
        {
            return VI_ERROR_VAMAPBUFFER;
        }
        enc->yuvdata = (char*)buf_ptr;
    }
    *uvdata = enc->yuvdata + enc->va_image[0].offsets[1];
    *uvdata_stride_bytes = enc->va_image[0].pitches[1];
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_encoder_set_fd_src(void* obj, int fd, int fd_width, int fd_height,
        int fd_stride, int fd_size, int fd_bpp)
{
    struct va_inf_enc_priv* enc;
    VASurfaceAttribExternalBuffers external;
    VASurfaceAttrib attribs[2];
    unsigned long fd_handle;
    VAStatus va_status;
    VASurfaceID surface;

    enc = (struct va_inf_enc_priv*)obj;
    fd_handle = (unsigned long) fd;
    memset(&external, 0, sizeof(external));
    external.pixel_format = VA_FOURCC_BGRX;
    external.width = fd_width;
    external.height = fd_height;
    external.data_size = fd_size;
    external.num_planes = 1;
    external.pitches[0] = fd_stride;
    external.buffers = &fd_handle;
    external.num_buffers = 1;
    memset(attribs, 0, sizeof(attribs));
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].type = VASurfaceAttribMemoryType;
    attribs[0].value.type = VAGenericValueTypeInteger;
    attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[1].type = VASurfaceAttribExternalBufferDescriptor;
    attribs[1].value.type = VAGenericValueTypePointer;
    attribs[1].value.value.p = &external;
    switch (fd_bpp)
    {
        case VI_NV12:
            external.pixel_format = VA_FOURCC_NV12;
            external.num_planes = 2;
            external.pitches[1] = fd_stride;
            external.offsets[1] = fd_height * fd_stride;
            va_status = vaCreateSurfaces(g_va_display, VA_RT_FORMAT_YUV420,
                                         fd_width, fd_height, &surface, 1,
                                         attribs, 2);
            break;
        case VI_YUY2:
            external.pixel_format = VA_FOURCC_YUY2;
            va_status = vaCreateSurfaces(g_va_display, VA_RT_FORMAT_YUV422,
                                         fd_width, fd_height, &surface, 1,
                                         attribs, 2);
            break;
        default:
            va_status = vaCreateSurfaces(g_va_display, VA_RT_FORMAT_RGB32,
                                         fd_width, fd_height, &surface, 1,
                                         attribs, 2);
            break;
    }
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VACREATESURFACES;
    }
    if (enc->fd_va_surface[0] != VA_INVALID_SURFACE)
    {
        vaDestroySurfaces(g_va_display, enc->fd_va_surface, 1);
    }
    enc->fd_va_surface[0] = surface;
    return VI_SUCCESS;
}

/*****************************************************************************/
static int
va_inf_encoder_encode_flags(void* obj, void* cdata, int* cdata_max_bytes,
        int flags);

/*****************************************************************************/
static int
va_inf_encoder_encode(void* obj, void* cdata, int* cdata_max_bytes)
{
    return va_inf_encoder_encode_flags(obj, cdata, cdata_max_bytes, 0);
}

/*****************************************************************************/
static int
va_inf_encoder_encode_flags(void* obj, void* cdata, int* cdata_max_bytes,
        int flags)
{
    struct va_inf_enc_priv* enc;
    VAStatus va_status;
    int width;
    int height;

    enc = (struct va_inf_enc_priv*)obj;
    if (enc->yuvdata != NULL)
    {
        va_status = vaUnmapBuffer(g_va_display, enc->va_image[0].buf);
        if (va_status != VA_STATUS_SUCCESS)
        {
            return VI_ERROR_VAUNMAPBUFFER;
        }
        enc->yuvdata = NULL;
    }
    if (flags & VI_H264_ENC_FLAG_KEYFRAME)
    {
    }
    if (enc->fd_va_surface[0] != VA_INVALID_SURFACE)
    {
    }
    else
    {
        width = enc->va_image[0].width;
        height = enc->va_image[0].height;
        va_status = vaPutImage(g_va_display, enc->va_surface[0],
                               enc->va_image[0].image_id,
                               0, 0, width, height,
                               0, 0, width, height);
        if (va_status != VA_STATUS_SUCCESS)
        {
            return VI_ERROR_VAPUTIMAGE;
        }
    }
    return VI_SUCCESS;
}

/*****************************************************************************/
int
va_inf_get_funcs(struct va_funcs* funcs, int version)
{
    if (version >= VI_VERSION_INT(0, 2))
    {
        funcs->get_version = va_inf_get_version;
        funcs->init = va_inf_init;
        funcs->deinit = va_inf_deinit;
        /* encoder */
        funcs->encoder_create = va_inf_encoder_create;
        funcs->encoder_delete = va_inf_encoder_delete;
        funcs->encoder_get_width = va_inf_encoder_get_width;
        funcs->encoder_get_height = va_inf_encoder_get_height;
        funcs->encoder_resize = va_inf_encoder_resize;
        funcs->encoder_get_ybuffer = va_inf_encoder_get_ybuffer;
        funcs->encoder_get_uvbuffer = va_inf_encoder_get_uvbuffer;
        funcs->encoder_set_fd_src = va_inf_encoder_set_fd_src;
        funcs->encoder_encode = va_inf_encoder_encode;
        funcs->encoder_encode_flags = va_inf_encoder_encode_flags;
        return VI_SUCCESS;
    }
    return VI_ERROR_OTHER;
}
