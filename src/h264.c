#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "x264.h"

#include <emscripten/emscripten.h>

x264_picture_t pic;
x264_param_t param;
x264_t *h;
int luma_size = 0, chroma_size = 0, g_w = 0, g_h = 0;

void rgba_to_yuv(uint8_t *array_buffer, size_t width, size_t height, uint8_t *output_buffer)
{
    size_t frame_size = width * height;
    uint8_t *y_plane = output_buffer;
    uint8_t *u_plane = output_buffer + frame_size;
    uint8_t *v_plane = output_buffer + frame_size + frame_size / 4;

    for (size_t j = 0; j < height; j++)
    {
        for (size_t i = 0; i < width; i++)
        {
            size_t rgba_index = (j * width + i) * 4;
            uint8_t r = array_buffer[rgba_index];
            uint8_t g = array_buffer[rgba_index + 1];
            uint8_t b = array_buffer[rgba_index + 2];

            uint8_t y = (uint8_t)((0.299 * r) + (0.587 * g) + (0.114 * b));
            uint8_t u = (uint8_t)((-0.169 * r) - (0.331 * g) + (0.5 * b) + 128);
            uint8_t v = (uint8_t)((0.5 * r) - (0.419 * g) - (0.081 * b) + 128);

            y_plane[j * width + i] = y;

            if (j % 2 == 0 && i % 2 == 0)
            {
                size_t uv_index = (j / 2) * (width / 2) + (i / 2);
                u_plane[uv_index] = u;
                v_plane[uv_index] = v;
            }
        }
    }
}

EMSCRIPTEN_KEEPALIVE
int create(int width, int height)
{
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    /* Configure non-default params */
    param.i_log_level = X264_LOG_NONE;
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;

    x264_picture_alloc(&pic, param.i_csp, param.i_width, param.i_height);
    h = x264_encoder_open(&param);
    g_w = width;
    g_h = height;
    luma_size = g_w * g_h;
    chroma_size = luma_size / 4;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int encoder(uint8_t *data_in, int idx, uint8_t *data_out, size_t *data_out_size)
{
    uint8_t* yuv = malloc(g_w * g_h * 3 / 2);
    rgba_to_yuv(data_in, g_w, g_h, yuv);

    pic.i_pts = idx;
    pic.img.i_csp = X264_CSP_I420;
    pic.img.i_plane = 3;
    pic.img.plane[0] = yuv;                     // Y分量
    pic.img.plane[1] = yuv + luma_size;         // U分量
    pic.img.plane[2] = yuv + luma_size  + chroma_size; // V分量
    pic.img.i_stride[0] = g_w;                     // Y分量的行步幅
    pic.img.i_stride[1] = g_w / 2;                 // U分量的行步幅
    pic.img.i_stride[2] = g_w / 2;                 // V分量的行步幅

    x264_nal_t *nal;
    int i_nal;
    x264_picture_t pic_out;
    int i_frame_size;
    i_frame_size = x264_encoder_encode(h, &nal, &i_nal, &pic, &pic_out);
    free(yuv);
    int frame_size = 0;
    for (int i = 0; i < i_nal; i++)
    {
        memcpy(data_out + frame_size, nal[i].p_payload, nal[i].i_payload);
        frame_size += nal[i].i_payload;
    }
    printf("i_frame_size: %d %d\n", i_frame_size, i_nal);
    memcpy(data_out_size, &i_frame_size, 4);
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int destroy()
{
    x264_encoder_close(h);
    x264_picture_clean(&pic);
    return 0;
}
