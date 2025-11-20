
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <va/va.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>
#include <va/va_enc_h264.h>

#include "va_inf.h"

#define VI_H264_SLICE_TYPE_P    0
#define VI_H264_SLICE_TYPE_B    1
#define VI_H264_SLICE_TYPE_I    2

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
    VASurfaceID enc_surface;
    VAConfigID enc_config;
    VAContextID enc_context;
    unsigned int width_in_mbs;
    unsigned int height_in_mbs;
    unsigned int frame_num;
    unsigned int idr_period;
    VABufferID coded_buf;
    VABufferID seq_param_buf;
    VABufferID pic_param_buf;
    VABufferID slice_param_buf;
    VABufferID rc_param_buf;
};

/*****************************************************************************/
static void
va_inf_encoder_init_picture_list(VAPictureH264* list, size_t count)
{
    size_t index;

    memset(list, 0, sizeof(VAPictureH264) * count);
    for (index = 0; index < count; index++)
    {
        list[index].picture_id = VA_INVALID_SURFACE;
        list[index].flags = VA_PICTURE_H264_INVALID;
    }
}

/*****************************************************************************/
static void
va_inf_encoder_destroy_buffers(struct va_inf_enc_priv* enc)
{
    size_t index;
    VABufferID* buffers[] =
    {
        &enc->seq_param_buf,
        &enc->pic_param_buf,
        &enc->slice_param_buf,
        &enc->rc_param_buf,
        &enc->coded_buf
    };

    for (index = 0; index < sizeof(buffers) / sizeof(buffers[0]); index++)
    {
        if (*(buffers[index]) != VA_INVALID_ID)
        {
            vaDestroyBuffer(g_va_display, *(buffers[index]));
            *(buffers[index]) = VA_INVALID_ID;
        }
    }
}

/*****************************************************************************/
static int
va_inf_encoder_setup_buffers(struct va_inf_enc_priv* enc,
        int width, int height, int flags)
{
    const unsigned int frame_rate = 30;
    unsigned int width_in_mbs;
    unsigned int height_in_mbs;
    unsigned int padded_width;
    unsigned int padded_height;
    unsigned int num_macroblocks;
    unsigned int codedbuf_size;
    unsigned int idr_period;
    uint64_t bitrate_calc;
    unsigned int bitrate;
    VAStatus va_status;
    VAEncSequenceParameterBufferH264 seq;
    VAEncPictureParameterBufferH264 pic;
    VAEncSliceParameterBufferH264 slice;
    struct _rc_param
    {
        VAEncMiscParameterBuffer header;
        VAEncMiscParameterRateControl rc;
    } rc_param;

    (void)flags;
    if ((width <= 0) || (height <= 0))
    {
        return VI_ERROR_OTHER;
    }

    width_in_mbs = (unsigned int) ((width + 15) / 16);
    height_in_mbs = (unsigned int) ((height + 15) / 16);
    if ((width_in_mbs == 0) || (height_in_mbs == 0))
    {
        return VI_ERROR_OTHER;
    }
    padded_width = width_in_mbs * 16;
    padded_height = height_in_mbs * 16;
    num_macroblocks = width_in_mbs * height_in_mbs;
    idr_period = frame_rate * 2;

    bitrate_calc = (uint64_t) width * (uint64_t) height * frame_rate;
    bitrate_calc /= 2;
    if (bitrate_calc < 1000000ULL)
    {
        bitrate = 1000000U;
    }
    else if (bitrate_calc > UINT32_MAX)
    {
        bitrate = UINT32_MAX;
    }
    else
    {
        bitrate = (unsigned int) bitrate_calc;
    }

    va_inf_encoder_destroy_buffers(enc);

    enc->width_in_mbs = width_in_mbs;
    enc->height_in_mbs = height_in_mbs;
    enc->frame_num = 0;
    enc->idr_period = idr_period;

    codedbuf_size = padded_width * padded_height * 4;
    if (codedbuf_size < 16384U)
    {
        codedbuf_size = 16384U;
    }

    va_status = vaCreateBuffer(g_va_display, enc->enc_context,
            VAEncCodedBufferType, codedbuf_size, 1, NULL, &enc->coded_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        enc->coded_buf = VA_INVALID_ID;
        return VI_ERROR_VACREATEBUFFER;
    }

    memset(&seq, 0, sizeof(seq));
    seq.intra_period = idr_period;
    seq.intra_idr_period = idr_period;
    seq.ip_period = 1;
    seq.bits_per_second = bitrate;
    seq.max_num_ref_frames = 1;
    seq.picture_width_in_mbs = width_in_mbs;
    seq.picture_height_in_mbs = height_in_mbs;
    seq.level_idc = 41;
    seq.seq_fields.bits.chroma_format_idc = 1;
    seq.seq_fields.bits.frame_mbs_only_flag = 1;
    seq.seq_fields.bits.direct_8x8_inference_flag = 1;
    seq.seq_fields.bits.log2_max_frame_num_minus4 = 4;
    seq.seq_fields.bits.pic_order_cnt_type = 0;
    seq.seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = 4;
    seq.seq_fields.bits.delta_pic_order_always_zero_flag = 1;
    seq.bit_depth_luma_minus8 = 0;
    seq.bit_depth_chroma_minus8 = 0;
    seq.vui_parameters_present_flag = 1;
    seq.vui_fields.bits.aspect_ratio_info_present_flag = 1;
    seq.vui_fields.bits.timing_info_present_flag = 1;
    seq.vui_fields.bits.fixed_frame_rate_flag = 1;
    seq.aspect_ratio_idc = 1;
    seq.sar_width = 1;
    seq.sar_height = 1;
    seq.num_units_in_tick = 1;
    seq.time_scale = frame_rate * 2;

    va_status = vaCreateBuffer(g_va_display, enc->enc_context,
            VAEncSequenceParameterBufferType, sizeof(seq), 1, &seq,
            &enc->seq_param_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        va_inf_encoder_destroy_buffers(enc);
        return VI_ERROR_VACREATEBUFFER;
    }

    memset(&pic, 0, sizeof(pic));
    pic.CurrPic.picture_id = enc->va_surface[0];
    pic.CurrPic.frame_idx = 0;
    pic.CurrPic.flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
    va_inf_encoder_init_picture_list(pic.ReferenceFrames,
            sizeof(pic.ReferenceFrames) / sizeof(pic.ReferenceFrames[0]));
    pic.coded_buf = enc->coded_buf;
    pic.pic_parameter_set_id = 0;
    pic.seq_parameter_set_id = 0;
    pic.frame_num = 0;
    pic.pic_init_qp = 26;
    pic.num_ref_idx_l0_active_minus1 = 0;
    pic.num_ref_idx_l1_active_minus1 = 0;
    pic.chroma_qp_index_offset = 0;
    pic.second_chroma_qp_index_offset = 0;
    pic.pic_fields.value = 0;
    pic.pic_fields.bits.idr_pic_flag = 1;
    pic.pic_fields.bits.reference_pic_flag = 1;
    pic.pic_fields.bits.entropy_coding_mode_flag = 1;
    pic.pic_fields.bits.deblocking_filter_control_present_flag = 1;
    pic.pic_fields.bits.transform_8x8_mode_flag = 1;

    va_status = vaCreateBuffer(g_va_display, enc->enc_context,
            VAEncPictureParameterBufferType, sizeof(pic), 1, &pic,
            &enc->pic_param_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        va_inf_encoder_destroy_buffers(enc);
        return VI_ERROR_VACREATEBUFFER;
    }

    memset(&slice, 0, sizeof(slice));
    slice.num_macroblocks = num_macroblocks;
    slice.slice_type = VI_H264_SLICE_TYPE_I;
    slice.pic_parameter_set_id = 0;
    slice.idr_pic_id = 0;
    slice.macroblock_info = VA_INVALID_ID;
    slice.pic_order_cnt_lsb = 0;
    va_inf_encoder_init_picture_list(slice.RefPicList0,
            sizeof(slice.RefPicList0) / sizeof(slice.RefPicList0[0]));
    va_inf_encoder_init_picture_list(slice.RefPicList1,
            sizeof(slice.RefPicList1) / sizeof(slice.RefPicList1[0]));
    slice.cabac_init_idc = 0;
    slice.slice_qp_delta = 0;
    slice.disable_deblocking_filter_idc = 0;

    va_status = vaCreateBuffer(g_va_display, enc->enc_context,
            VAEncSliceParameterBufferType, sizeof(slice), 1, &slice,
            &enc->slice_param_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        va_inf_encoder_destroy_buffers(enc);
        return VI_ERROR_VACREATEBUFFER;
    }

    memset(&rc_param, 0, sizeof(rc_param));
    rc_param.header.type = VAEncMiscParameterTypeRateControl;
    rc_param.rc.bits_per_second = bitrate;
    rc_param.rc.target_percentage = 66;
    rc_param.rc.window_size = 1000;
    rc_param.rc.initial_qp = 26;
    rc_param.rc.min_qp = 10;
    rc_param.rc.basic_unit_size = 0;

    va_status = vaCreateBuffer(g_va_display, enc->enc_context,
            VAEncMiscParameterBufferType, sizeof(rc_param), 1, &rc_param,
            &enc->rc_param_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        va_inf_encoder_destroy_buffers(enc);
        return VI_ERROR_VACREATEBUFFER;
    }

    return VI_SUCCESS;
}

/*****************************************************************************/
static void
va_inf_encoder_destroy_priv(struct va_inf_enc_priv* enc)
{
    if (enc == NULL)
    {
        return;
    }
    va_inf_encoder_destroy_buffers(enc);
    if (enc->enc_context != VA_INVALID_ID)
    {
        vaDestroyContext(g_va_display, enc->enc_context);
        enc->enc_context = VA_INVALID_ID;
    }
    if (enc->enc_config != VA_INVALID_ID)
    {
        vaDestroyConfig(g_va_display, enc->enc_config);
        enc->enc_config = VA_INVALID_ID;
    }
}

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
    VAConfigAttrib config_attrib;
    int error;

    enc = (struct va_inf_enc_priv*)
            calloc(1, sizeof(struct va_inf_enc_priv));
    if (enc == NULL)
    {
        return VI_ERROR_MEMORY;
    }
    enc->va_surface[0] = VA_INVALID_SURFACE;
    enc->fd_va_surface[0] = VA_INVALID_SURFACE;
    enc->enc_surface = VA_INVALID_SURFACE;
    enc->coded_buf = VA_INVALID_ID;
    enc->seq_param_buf = VA_INVALID_ID;
    enc->pic_param_buf = VA_INVALID_ID;
    enc->slice_param_buf = VA_INVALID_ID;
    enc->rc_param_buf = VA_INVALID_ID;
    enc->enc_config = VA_INVALID_ID;
    enc->enc_context = VA_INVALID_ID;
    if (type == VI_TYPE_H264)
    {
        memset(&config_attrib, 0, sizeof(config_attrib));
        config_attrib.type = VAConfigAttribRTFormat;
        va_status = vaGetConfigAttributes(g_va_display, VAProfileH264High,
                VAEntrypointEncSlice, &config_attrib, 1);
        if (va_status != VA_STATUS_SUCCESS)
        {
            free(enc);
            return VI_ERROR_VACREATECONFIG;
        }
        if ((config_attrib.value & VA_RT_FORMAT_YUV420) == 0)
        {
            free(enc);
            return VI_ERROR_OTHER;
        }
        config_attrib.value = VA_RT_FORMAT_YUV420;
        va_status = vaCreateConfig(g_va_display, VAProfileH264High,
                VAEntrypointEncSlice, &config_attrib, 1, &enc->enc_config);
        if (va_status != VA_STATUS_SUCCESS)
        {
            free(enc);
            return VI_ERROR_VACREATECONFIG;
        }
        va_status = vaCreateContext(g_va_display, enc->enc_config,
                width, height, VA_PROGRESSIVE, NULL, 0, &enc->enc_context);
        if (va_status != VA_STATUS_SUCCESS)
        {
            va_inf_encoder_destroy_priv(enc);
            free(enc);
            return VI_ERROR_VACREATECONTEXT;
        }
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
            width, height, enc->va_surface, 1, attribs, 1);
    if (va_status != VA_STATUS_SUCCESS)
    {
        va_inf_encoder_destroy_priv(enc);
        free(enc);
        return VI_ERROR_VACREATESURFACES;
    }
    memset(&va_image_format, 0, sizeof(va_image_format));
    va_image_format.fourcc = VA_FOURCC_NV12;
    va_status = vaCreateImage(g_va_display, &va_image_format,
            width, height, enc->va_image);
    if (va_status != VA_STATUS_SUCCESS)
    {
        va_inf_encoder_destroy_priv(enc);
        vaDestroySurfaces(g_va_display, enc->va_surface, 1);
        free(enc);
        return VI_ERROR_VACREATEIMAGE;
    }

    error = va_inf_encoder_setup_buffers(enc, width, height, flags);
    if (error != VI_SUCCESS)
    {
        va_inf_encoder_destroy_priv(enc);
        vaDestroyImage(g_va_display, enc->va_image[0].image_id);
        vaDestroySurfaces(g_va_display, enc->va_surface, 1);
        free(enc);
        return error;
    }

    enc->width = width;
    enc->height = height;
    enc->type = type;
    enc->flags = flags;
    enc->fd_va_surface[0] = VA_INVALID_SURFACE;
    enc->yuvdata = NULL;
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

    va_inf_encoder_destroy_priv(enc);

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
    int error;

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
            width, height, enc->va_surface, 1, attribs, 1);
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
    error = va_inf_encoder_setup_buffers(enc, width, height, enc->flags);
    if (error != VI_SUCCESS)
    {
        return error;
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
                    fd_width, fd_height, &surface, 1, attribs, 2);
            break;
        case VI_YUY2:
            external.pixel_format = VA_FOURCC_YUY2;
            va_status = vaCreateSurfaces(g_va_display, VA_RT_FORMAT_YUV422,
                    fd_width, fd_height, &surface, 1, attribs, 2);
            break;
        default:
            va_status = vaCreateSurfaces(g_va_display, VA_RT_FORMAT_RGB32,
                    fd_width, fd_height, &surface, 1, attribs, 2);
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
va_inf_encoder_encode(void* obj, void* cdata, int* cdata_max_bytes, int flags)
{
    struct va_inf_enc_priv* enc;
    VAStatus va_status;
    int width;
    int height;
    int force_idr;
    void* buf_ptr;
    VAEncPictureParameterBufferH264* pic;
    VAEncSliceParameterBufferH264* slice;
    VABufferID buffers[4];
    unsigned int buffer_count;
    VACodedBufferSegment* segment;
    unsigned char* dst;
    unsigned int dst_size;
    unsigned int bytes_written;

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
    if (enc->fd_va_surface[0] != VA_INVALID_SURFACE)
    {
        enc->enc_surface = enc->fd_va_surface[0];
    }
    else
    {
        width = enc->va_image[0].width;
        height = enc->va_image[0].height;
        va_status = vaPutImage(g_va_display, enc->va_surface[0],
                enc->va_image[0].image_id, 0, 0, width, height, 0, 0,
                width, height);
        if (va_status != VA_STATUS_SUCCESS)
        {
            return VI_ERROR_VAPUTIMAGE;
        }
        enc->enc_surface = enc->va_surface[0];
    }

    va_status = vaSyncSurface(g_va_display, enc->enc_surface);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VASYNCSURFACE;
    }

    if ((cdata == NULL) || (cdata_max_bytes == NULL) || (*cdata_max_bytes <= 0))
    {
        return VI_ERROR_OTHER;
    }

    force_idr = (flags & VI_H264_ENC_FLAG_KEYFRAME) != 0;
    if ((enc->idr_period != 0) && (enc->frame_num % enc->idr_period == 0))
    {
        force_idr = 1;
    }
    if (enc->frame_num == 0)
    {
        force_idr = 1;
    }

    va_status = vaMapBuffer(g_va_display, enc->pic_param_buf, &buf_ptr);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VAMAPBUFFER;
    }
    pic = (VAEncPictureParameterBufferH264*)buf_ptr;
    pic->CurrPic.picture_id = enc->enc_surface;
    pic->CurrPic.frame_idx = enc->frame_num;
    pic->frame_num = enc->frame_num;
    pic->pic_fields.bits.idr_pic_flag = force_idr ? 1 : 0;
    va_status = vaUnmapBuffer(g_va_display, enc->pic_param_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VAUNMAPBUFFER;
    }

    va_status = vaMapBuffer(g_va_display, enc->slice_param_buf, &buf_ptr);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VAMAPBUFFER;
    }
    slice = (VAEncSliceParameterBufferH264*)buf_ptr;
    slice->macroblock_address = 0;
    slice->num_macroblocks = enc->width_in_mbs * enc->height_in_mbs;
    /* Only advertise I/P slices so the driver never generates B frames. */
    slice->slice_type = force_idr ? VI_H264_SLICE_TYPE_I :
            VI_H264_SLICE_TYPE_P;
    slice->idr_pic_id = enc->frame_num & 0xFFFFU;
    va_status = vaUnmapBuffer(g_va_display, enc->slice_param_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VAUNMAPBUFFER;
    }

    va_status = vaBeginPicture(g_va_display, enc->enc_context, enc->enc_surface);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VABEGINPICTURE;
    }

    buffer_count = 0;
    if (enc->seq_param_buf != VA_INVALID_ID)
    {
        buffers[buffer_count++] = enc->seq_param_buf;
    }
    if (enc->rc_param_buf != VA_INVALID_ID)
    {
        buffers[buffer_count++] = enc->rc_param_buf;
    }
    if (enc->pic_param_buf != VA_INVALID_ID)
    {
        buffers[buffer_count++] = enc->pic_param_buf;
    }
    if (enc->slice_param_buf != VA_INVALID_ID)
    {
        buffers[buffer_count++] = enc->slice_param_buf;
    }
    if (buffer_count > 0)
    {
        va_status = vaRenderPicture(g_va_display, enc->enc_context,
                buffers, buffer_count);
        if (va_status != VA_STATUS_SUCCESS)
        {
            vaEndPicture(g_va_display, enc->enc_context);
            return VI_ERROR_VARENDERPICTURE;
        }
    }

    va_status = vaEndPicture(g_va_display, enc->enc_context);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VAENDPICTURE;
    }

    va_status = vaMapBuffer(g_va_display, enc->coded_buf, &buf_ptr);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VAMAPBUFFER;
    }
    segment = (VACodedBufferSegment*)buf_ptr;
    dst = (unsigned char*)cdata;
    dst_size = (unsigned int)(*cdata_max_bytes);
    bytes_written = 0;
    while (segment != NULL)
    {
        if (segment->size > dst_size - bytes_written)
        {
            vaUnmapBuffer(g_va_display, enc->coded_buf);
            return VI_ERROR_OTHER;
        }
        memcpy(dst + bytes_written, segment->buf, segment->size);
        bytes_written += segment->size;
        segment = (VACodedBufferSegment*)segment->next;
    }
    va_status = vaUnmapBuffer(g_va_display, enc->coded_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        return VI_ERROR_VAUNMAPBUFFER;
    }

    *cdata_max_bytes = (int)bytes_written;
    enc->frame_num++;

    return VI_SUCCESS;
}

/*****************************************************************************/
int
va_inf_get_funcs(struct va_funcs* funcs, int version)
{
    if (version >= VI_VERSION_INT(VI_MAJOR, VI_MINOR))
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
        return VI_SUCCESS;
    }
    return VI_ERROR_OTHER;
}
