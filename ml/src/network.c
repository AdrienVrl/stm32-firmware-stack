/**
 ******************************************************************************
 * @file    network.c
 * @author  AST Embedded Analytics Research Platform
 * @date    2026-09-10T22:42:20+0200
 * @brief   AI Tool Automatic Code Generator for Embedded NN computing
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 ******************************************************************************
 */

#include "network.h"

#include "ai_lite_inspect.h"
#include "ai_platform_interface.h"
#include "core_convert.h"
#include "layers.h"
#include "lite_operators.h"
#include "network_data.h"
#include "network_details.h"
#include "stai_events.h"
/*****************************************************************************/
#define STAI_INTERNAL_API_MAJOR (1)
#define STAI_INTERNAL_API_MINOR (0)
#define STAI_INTERNAL_API_MICRO (0)

#define STAI_MAGIC (0xB1C00100)

/*****************************************************************************/
#define _STAI_CONCAT_ARG(a, b) a##b
#define STAI_CONCAT(a, b)      _STAI_CONCAT_ARG(a, b)

/*!  STAI_CAST SECTION                       *********************************/
#define STAI_CAST(type, expr) ((type)(expr))


/*****************************************************************************/
#define STAI_SIZE(_size) ((stai_size)(_size))

/*****************************************************************************/
#define STAI_INIT_BUFFER(_flags, _size, _address)                                                  \
    {                                                                                              \
        .size    = (_size),                                                                        \
        .address = (uintptr_t)(_address),                                                          \
        .flags   = (_flags),                                                                       \
    }

#define STAI_INIT_TENSOR(_name, _flags, _fmt, _size_bytes, _shape, _scale, _zeropoint)             \
    {.size_bytes = (_size_bytes),                                                                  \
     .flags      = (_flags),                                                                       \
     .format     = (stai_format)(_fmt),                                                            \
     .shape      = STAI_PACK(_shape),                                                              \
     .scale      = STAI_PACK(_scale),                                                              \
     .zeropoint  = STAI_PACK(_zeropoint),                                                          \
     .name       = (_name)}

#define STAI_INIT_ARRAY(_size, _ptr) {.size = STAI_SIZE(_size), .data = STAI_PACK(_ptr)}


#define STAI_CAST_ARRAY(_type, _size, _ptr)                                                        \
    {.size = STAI_SIZE(_size), .data = (_type)STAI_PACK(_ptr)}


#define STAI_DECLARE_ARRAY(_type, _size, ...)                                                      \
    {                                                                                              \
        .size = STAI_SIZE(_size), .data = (_type[_size])                                           \
        {                                                                                          \
            STAI_PACK(__VA_ARGS__)                                                                 \
        }                                                                                          \
    }


#define STAI_EMPTY_ARRAY() {.size = 0, .data = NULL}


#define STAI_INIT_VERSION(_major, _minor, _micro)                                                  \
    {.major = (_major), .minor = (_minor), .micro = (_micro), .reserved = 0x0}

/*****************************************************************************/
/**  Getters and setters  **/

#define STAI_GET_ARRAY_SIZE(nd_array) (nd_array.size)


#define STAI_GET_ARRAY_ELEM(nd_array, pos) (nd_array.data[(pos)])

#define _STAI_SET_ERROR(net_ctx, cond, value, exit)                                                \
    {                                                                                              \
        if (!(net_ctx))                                                                            \
        {                                                                                          \
            return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;                                      \
        }                                                                                          \
        if (((uintptr_t)net_ctx) & (_STAI_CONTEXT_ALIGNMENT - 1))                                  \
        {                                                                                          \
            return STAI_ERROR_NETWORK_INVALID_CONTEXT_ALIGNMENT;                                   \
        }                                                                                          \
        if (((value) >= STAI_ERROR_GENERIC) && (cond))                                             \
        {                                                                                          \
            if ((net_ctx)->_return_code == STAI_SUCCESS)                                           \
            {                                                                                      \
                (net_ctx)->_return_code = (value);                                                 \
            }                                                                                      \
            return (exit);                                                                         \
        }                                                                                          \
    }

/*****************************************************************************/
/* TODO REMOVE THESE TWO MACROS */
#define STAI_EVENT_NODE_START_CB
#define STAI_EVENT_NODE_STOP_CB

#ifdef STAI_EVENT_NODE_START_CB
#ifndef _STAI_NETWORK_EVENT_NODE_START_CB
#define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...)                            \
    if (net_ctx->_callback)                                                                        \
    {                                                                                              \
        const stai_event_node_start_stop _start_event = {                                          \
            .node_id = (_node_id),                                                                 \
            .buffers = {                                                                           \
                .size = (_buffers_size),                                                           \
                .data = (stai_ptr const *)(const stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__)}}; \
        net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_START,                       \
                           (const void *)&_start_event);                                           \
    }
#endif
#else
#define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...)                            \
    do                                                                                             \
    {  /* _STAI_NETWORK_EVENT_NODE_START_CB() */                                                   \
    } while (0);
#endif /* STAI_EVENT_NODE_START_CB */

#ifdef STAI_EVENT_NODE_STOP_CB
#ifndef _STAI_NETWORK_EVENT_NODE_STOP_CB
#define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...)                             \
    if (net_ctx->_callback)                                                                        \
    {                                                                                              \
        const stai_event_node_start_stop _stop_event = {                                           \
            .node_id = (_node_id),                                                                 \
            .buffers = {.size = (_buffers_size),                                                   \
                        .data =                                                                    \
                            (stai_ptr const *)(stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__)}};   \
        net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_STOP,                        \
                           (const void *)&_stop_event);                                            \
    }
#endif
#else
#define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...)                             \
    do                                                                                             \
    {  /* _STAI_NETWORK_EVENT_NODE_STOP_CB() */                                                    \
    } while (0);
#endif /* STAI_EVENT_NODE_STOP_CB */


/*****************************************************************************/
#define _STAI_NETWORK_MODEL_SIGNATURE  "0x550d57c1752b06c128a6a340588e4067"
#define _STAI_NETWORK_DATETIME         "2026-09-10T22:42:20+0200"
#define _STAI_NETWORK_COMPILE_DATETIME __DATE__ " " __TIME__

#define _STAI_CONTEXT_ALIGNMENT STAI_NETWORK_CONTEXT_ALIGNMENT

/*****************************************************************************/
#define g_network_activations_1 (NULL)


#if defined(HAVE_NETWORK_INFO)
/*****************************************************************************/
static const stai_network_info g_network_info = {
    .model_signature    = _STAI_NETWORK_MODEL_SIGNATURE,
    .c_compile_datetime = _STAI_NETWORK_COMPILE_DATETIME,
    .c_model_name       = STAI_NETWORK_MODEL_NAME,
    .c_model_datetime   = _STAI_NETWORK_DATETIME,
    .c_model_signature  = 0x0,
    .runtime_version    = STAI_INIT_VERSION(12, 0, 1),
    .tool_version       = STAI_INIT_VERSION(4, 0, 1),
    .api_version        = STAI_INIT_VERSION(1, 0, 0),
    .n_macc             = STAI_NETWORK_MACC_NUM,
    .n_nodes            = STAI_NETWORK_NODES_NUM,
    .flags              = STAI_NETWORK_FLAGS,
    .n_inputs           = STAI_NETWORK_IN_NUM,
    .n_outputs          = STAI_NETWORK_OUT_NUM,
    .n_activations      = STAI_NETWORK_ACTIVATIONS_NUM,
    .n_weights          = STAI_NETWORK_WEIGHTS_NUM,
    .n_states           = STAI_NETWORK_STATES_NUM,
    .inputs =
        (stai_tensor[STAI_NETWORK_IN_NUM]){
            STAI_INIT_TENSOR(STAI_NETWORK_IN_1_NAME, STAI_NETWORK_IN_1_FLAGS,
                             STAI_NETWORK_IN_1_FORMAT, STAI_NETWORK_IN_1_SIZE_BYTES,
                             STAI_DECLARE_ARRAY(int32_t, 4, 1, 49, 10, 1),
                             STAI_DECLARE_ARRAY(float, 1, 0.6043851971626282f),
                             STAI_DECLARE_ARRAY(int16_t, 1, 76)),
        },
    .outputs =
        (stai_tensor[STAI_NETWORK_OUT_NUM]){
            STAI_INIT_TENSOR(
                STAI_NETWORK_OUT_1_NAME, STAI_NETWORK_OUT_1_FLAGS, STAI_NETWORK_OUT_1_FORMAT,
                STAI_NETWORK_OUT_1_SIZE_BYTES, STAI_DECLARE_ARRAY(int32_t, 2, 1, 12),
                STAI_DECLARE_ARRAY(float, 1, 0.00390625f), STAI_DECLARE_ARRAY(int16_t, 1, -128)),
        },
    .activations =
        (stai_tensor[STAI_NETWORK_ACTIVATIONS_NUM]){
            STAI_INIT_TENSOR((NULL), STAI_NETWORK_ACTIVATION_1_FLAGS, STAI_FORMAT_U8,
                             STAI_NETWORK_ACTIVATION_1_SIZE_BYTES,
                             STAI_DECLARE_ARRAY(int32_t, 1, 18252), STAI_EMPTY_ARRAY(),
                             STAI_EMPTY_ARRAY()),
        },
    .weights =
        (stai_tensor[STAI_NETWORK_WEIGHTS_NUM]){
            STAI_INIT_TENSOR((NULL), STAI_NETWORK_WEIGHT_1_FLAGS, STAI_FORMAT_U8,
                             STAI_NETWORK_WEIGHT_1_SIZE_BYTES,
                             STAI_DECLARE_ARRAY(int32_t, 1, 24368), STAI_EMPTY_ARRAY(),
                             STAI_EMPTY_ARRAY()),
        },

    .states = NULL};
#endif

#define _STAI_CONTEXT_ACQUIRE(_net_ctx, _net_handle)                                               \
    _stai_network_context *_net_ctx = (_stai_network_context *)(_net_handle);                      \
    STAI_ASSERT(_net_ctx != NULL)                                                                  \
    _STAI_SET_ERROR(_net_ctx, _net_ctx->_magic != STAI_MAGIC,                                      \
                    STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE, _net_ctx->_return_code)


/*****************************************************************************/
static void _stai_network_check(_stai_network_context *net_ctx)
{
    stai_size idx;

    // Check activations status
    for (idx = 0; idx < STAI_NETWORK_ACTIVATIONS_NUM; idx++)
    {
        if (net_ctx->_activations[idx] == NULL)
            break;
    }
    net_ctx->_flags |=
        (idx == STAI_NETWORK_ACTIVATIONS_NUM) ? STAI_FLAG_ACTIVATIONS : STAI_FLAG_NONE;
    // Check inputs status
    for (idx = 0; idx < STAI_NETWORK_IN_NUM; idx++)
    {
        if (net_ctx->_inputs[idx] == NULL)
            break;
    }
    net_ctx->_flags |= (idx == STAI_NETWORK_IN_NUM) ? STAI_FLAG_INPUTS : STAI_FLAG_NONE;

    // Check outputs status
    for (idx = 0; idx < STAI_NETWORK_OUT_NUM; idx++)
    {
        if (net_ctx->_outputs[idx] == NULL)
            break;
    }
    net_ctx->_flags |= (idx == STAI_NETWORK_OUT_NUM) ? STAI_FLAG_OUTPUTS : STAI_FLAG_NONE;

    // Check weights status
    for (idx = 0; idx < STAI_NETWORK_WEIGHTS_NUM; idx++)
    {
        if (net_ctx->_weights[idx] == NULL)
            break;
    }
    net_ctx->_flags |= (idx == STAI_NETWORK_WEIGHTS_NUM) ? STAI_FLAG_WEIGHTS : STAI_FLAG_NONE;
    STAI_PRINT("  [_stai_network_check] flags: 0x%08x\n", net_ctx->_flags)
}


/*****************************************************************************/
STAI_API_ENTRY
stai_return_code stai_network_init(stai_network *network)
{
    /* Memory where to store internal context is provided by applications as a raw byte buffer */
    _stai_network_context *net_ctx = (_stai_network_context *)(network);
    net_ctx->_return_code          = STAI_SUCCESS;
    STAI_PRINT("[Entering Network Init] network(%p) context_size(%d)\n", net_ctx,
               (int32_t)sizeof(_stai_network_context))

    _STAI_SET_ERROR(net_ctx, STAI_NETWORK_CONTEXT_SIZE != sizeof(_stai_network_context),
                    STAI_ERROR_NETWORK_INVALID_CONTEXT_SIZE, net_ctx->_return_code)

    {
        const _stai_network_context _network_context = {
            ._magic           = STAI_MAGIC,
            ._signature       = STAI_NETWORK_MODEL_SIGNATURE,
            ._flags           = STAI_NETWORK_FLAGS,
            ._return_code     = STAI_SUCCESS,
            ._callback        = NULL,
            ._callback_cookie = NULL,
            ._activations     = {(stai_ptr)g_network_activations_1},
            ._weights         = {(stai_ptr)g_network_weights_array},
            ._inputs          = {NULL},
            ._outputs         = {NULL},
        };

        // Deep copy of internal context to opaque buffer provided by app
        *net_ctx = _network_context;

        _stai_network_check(net_ctx);
    }

    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_deinit(stai_network *network)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)

    /*  Reset flags to initial state  */
    net_ctx->_flags = STAI_NETWORK_FLAGS;
    return net_ctx->_return_code;
}

/*****************************************************************************/


/* Int quant #0 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_8_output_array_intq, AI_STATIC,
                              AI_BUFFER_META_FLAG_SCALE_FLOAT | AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
                              AI_PACK_INTQ_INFO(AI_PACK_INTQ_SCALE(0.2629375755786896f),
                                                AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_9_output_array_intq, AI_STATIC,
                              AI_BUFFER_META_FLAG_SCALE_FLOAT | AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
                              AI_PACK_INTQ_INFO(AI_PACK_INTQ_SCALE(0.0300661101937294f),
                                                AI_PACK_INTQ_ZP(-128)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_10_output_array_intq, AI_STATIC,
                              AI_BUFFER_META_FLAG_SCALE_FLOAT | AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
                              AI_PACK_INTQ_INFO(AI_PACK_INTQ_SCALE(0.2272031307220459f),
                                                AI_PACK_INTQ_ZP(99)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(
    gemm_10_weights_array_intq, AI_STATIC,
    AI_BUFFER_META_FLAG_SCALE_FLOAT | AI_BUFFER_META_FLAG_ZEROPOINT_S8, 12,
    AI_PACK_INTQ_INFO(AI_PACK_INTQ_SCALE(0.007861228659749031f, 0.009282120503485203f,
                                         0.010738077573478222f, 0.00577234523370862f,
                                         0.010637706145644188f, 0.010232983157038689f,
                                         0.011092457920312881f, 0.009092235937714577f,
                                         0.008290989324450493f, 0.0065200114622712135f,
                                         0.006947194691747427f, 0.010335966013371944f),
                      AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))


/* Array#0 */
AI_ARRAY_OBJ_DECLARE(conv2d_8_output_array, AI_ARRAY_FORMAT_S8, NULL, NULL, 8000, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(pool_9_output_array, AI_ARRAY_FORMAT_S8, NULL, NULL, 64, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(gemm_10_output_array, AI_ARRAY_FORMAT_S8, NULL, NULL, 12, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(gemm_10_weights_array, AI_ARRAY_FORMAT_S8, NULL, NULL, 768, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(gemm_10_bias_array, AI_ARRAY_FORMAT_S32, NULL, NULL, 12, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(gemm_10_scratch0_array, AI_ARRAY_FORMAT_S16, NULL, NULL, 124, AI_STATIC)


/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(conv2d_8_output, AI_STATIC, 37, 0x1, AI_SHAPE_INIT(4, 1, 64, 5, 25),
                      AI_STRIDE_INIT(4, 1, 1, 64, 320), 1, &conv2d_8_output_array,
                      &conv2d_8_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(pool_9_output, AI_STATIC, 46, 0x1, AI_SHAPE_INIT(4, 1, 64, 1, 1),
                      AI_STRIDE_INIT(4, 1, 1, 64, 64), 1, &pool_9_output_array,
                      &pool_9_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(gemm_10_bias, AI_STATIC, 40, 0x0, AI_SHAPE_INIT(4, 1, 12, 1, 1),
                      AI_STRIDE_INIT(4, 4, 4, 48, 48), 1, &gemm_10_bias_array, NULL)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(gemm_10_output, AI_STATIC, 41, 0x1, AI_SHAPE_INIT(4, 1, 12, 1, 1),
                      AI_STRIDE_INIT(4, 1, 1, 12, 12), 1, &gemm_10_output_array,
                      &gemm_10_output_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(gemm_10_scratch0, AI_STATIC, 42, 0x0, AI_SHAPE_INIT(4, 1, 124, 1, 1),
                      AI_STRIDE_INIT(4, 2, 2, 248, 248), 1, &gemm_10_scratch0_array, NULL)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(gemm_10_weights, AI_STATIC, 43, 0x1, AI_SHAPE_INIT(4, 64, 12, 1, 1),
                      AI_STRIDE_INIT(4, 1, 64, 768, 768), 1, &gemm_10_weights_array,
                      &gemm_10_weights_array_intq)


AI_TENSOR_CHAIN_OBJ_DECLARE(pool_9_chain, AI_STATIC_CONST, 4,
                            AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_8_output),
                            AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_9_output),
                            AI_TENSOR_LIST_OBJ_EMPTY, AI_TENSOR_LIST_OBJ_EMPTY)

AI_LAYER_OBJ_DECLARE(pool_9_layer, 9, POOL_TYPE, 0x0, NULL, pool, forward_ap_integer_INT8,
                     &pool_9_chain, NULL, &pool_9_layer, AI_STATIC,
                     .pool_size = AI_SHAPE_2D_INIT(5, 25), .pool_stride = AI_SHAPE_2D_INIT(5, 25),
                     .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), )

AI_TENSOR_CHAIN_OBJ_DECLARE(gemm_10_chain, AI_STATIC_CONST, 4,
                            AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_9_output),
                            AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_10_output),
                            AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gemm_10_weights,
                                                    &gemm_10_bias),
                            AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_10_scratch0))

AI_LAYER_OBJ_DECLARE(gemm_10_layer, 10, DENSE_TYPE, 0x0, NULL, dense, forward_dense_integer_SSSA_ch,
                     &gemm_10_chain, NULL, &gemm_10_layer, AI_STATIC, )
/**  Hybrid layers declarations section  *************************************/
void forward_lite_ap_integer_INT8_pool_9(_stai_network_context *net_ctx)
{
    conv2d_8_output_array.data       = AI_PTR(net_ctx->_activations[0] + 4364);
    conv2d_8_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 4364);
    pool_9_output_array.data         = AI_PTR(net_ctx->_activations[0] + 0);
    pool_9_output_array.data_start   = AI_PTR(net_ctx->_activations[0] + 0);
    _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {conv2d_8_output.data->data});
    forward_ap_integer_INT8(&pool_9_layer);
    _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {pool_9_output.data->data});
}
void forward_lite_dense_integer_SSSA_ch_gemm_10(_stai_network_context *net_ctx)
{
    pool_9_output_array.data          = AI_PTR(net_ctx->_activations[0] + 0);
    pool_9_output_array.data_start    = AI_PTR(net_ctx->_activations[0] + 0);
    gemm_10_weights_array.data        = AI_PTR(net_ctx->_weights[0] + 23552);
    gemm_10_weights_array.data_start  = AI_PTR(net_ctx->_weights[0] + 23552);
    gemm_10_bias_array.data           = AI_PTR(net_ctx->_weights[0] + 24320);
    gemm_10_bias_array.data_start     = AI_PTR(net_ctx->_weights[0] + 24320);
    gemm_10_scratch0_array.data       = AI_PTR(net_ctx->_activations[0] + 64);
    gemm_10_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 64);
    gemm_10_output_array.data         = AI_PTR(net_ctx->_activations[0] + 312);
    gemm_10_output_array.data_start   = AI_PTR(net_ctx->_activations[0] + 312);
    _STAI_NETWORK_EVENT_NODE_START_CB(10, 1, {pool_9_output.data->data});
    forward_dense_integer_SSSA_ch(&gemm_10_layer);
    _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, {gemm_10_output.data->data});
}

/*****************************************************************************/


static const ai_u16 conv2d_0_t_in_0_shape_w_const_u16           = 10;
static const ai_u16 conv2d_0_t_in_0_shape_h_const_u16           = 49;
static const ai_u16 conv2d_0_t_in_0_shape_ch_const_u16          = 1;
static const ai_u16 conv2d_0_t_out_0_shape_ch_const_u16         = 64;
static const ai_u16 conv2d_0_t_weight_0_shape_w_const_u16       = 4;
static const ai_u16 conv2d_0_t_weight_0_shape_h_const_u16       = 10;
static const ai_u16 conv2d_0_l_stride_1_const_u16               = 2;
static const ai_u16 conv2d_0_l_stride_0_const_u16               = 2;
static const ai_i32 conv2d_0_l_pad_W_0_const_s32                = 1;
static const ai_i32 conv2d_0_l_pad_H_0_const_s32                = 4;
static const ai_i8 conv2d_0_t_in_0_fmt_zero_const_s8            = 76;
static const ai_i8 conv2d_0_t_out_0_fmt_zero_const_s8           = -128;
static const ai_float conv2d_0_t_in_0_fmt_scale_const_f32       = 0.6043851971626282f;
static const ai_float conv2d_0_t_out_0_fmt_scale_const_f32      = 0.10601875185966492f;
static const ai_float conv2d_0_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(
    0.0010953828459605575f, 0.0010173708433285356f, 0.0007124171243049204f, 0.00012524393969215453f,
    0.0004839893081225455f, 0.0008001849637366831f, 0.0005159427528269589f, 0.0005402021342888474f,
    0.0004453610454220325f, 0.0005226925131864846f, 0.001589129795320332f, 0.0006161550409160554f,
    0.000897917605470866f, 0.0008636401616968215f, 0.0005229328526183963f, 0.0004725162871181965f,
    0.0003175717720296234f, 0.00040051070391200483f, 0.0006352889467962086f, 0.0008304056245833635f,
    0.00046055662096478045f, 0.0007706459145992994f, 0.0005886863800697029f, 0.0005400180234573781f,
    0.0004625786968972534f, 0.0003737754886969924f, 0.001073716557584703f, 0.0006772438646294177f,
    0.00075994071085006f, 0.0007808966911397874f, 0.000695207912940532f, 0.0007571991300210357f,
    0.0014071400510147214f, 0.0007184691494330764f, 0.0011588034685701132f, 0.0004149508895352483f,
    0.0007114938925951719f, 0.0009006517939269543f, 0.00032553059281781316f, 0.0006346011650748551f,
    0.0009435090469196439f, 0.0006396130193024874f, 0.00048690970288589597f, 0.0006586888339370489f,
    0.0006404921878129244f, 0.0007368400110863149f, 0.0007610857719555497f, 0.00117778149433434f,
    0.0002631203387863934f, 0.0007226499146781862f, 0.0005229667876847088f, 0.0008372181910090148f,
    0.0008964381995610893f, 0.00042049388866871595f, 0.0006933383410796523f, 0.0005963041330687702f,
    0.00047959506628103554f, 0.0008217428694479167f, 0.0006680244696326554f, 0.0007099092472344637f,
    0.0003366696764715016f, 0.001191739458590746f, 0.00045963205047883093f, 0.0006768389139324427f);
static const ai_layer_format_type conv2d_0_l_out_ch_format_const_layer_format_type =
    AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_0_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_0_t_out_0_shape_h_const_u16 = 25;

static const ai_i8 conv2d_1_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_1_pad_before_t_in_0_fmt_bitsize_const_s16   = 8;
static const ai_u32 conv2d_1_pad_before_t_in_0_shape_h_const_u32       = 25;

static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16           = 7;
static const ai_u16 conv2d_1_t_in_0_shape_h_const_u16           = 27;
static const ai_u16 conv2d_1_t_in_0_shape_ch_const_u16          = 64;
static const ai_u16 conv2d_1_l_stride_1_const_u16               = 1;
static const ai_u16 conv2d_1_l_stride_0_const_u16               = 1;
static const ai_i8 conv2d_1_t_in_0_fmt_zero_const_s8            = -128;
static const ai_i8 conv2d_1_t_out_0_fmt_zero_const_s8           = -128;
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32       = 0.10601875185966492f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32      = 0.12000501900911331f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(
    0.009476185776293278f, 0.007992769591510296f, 0.008713269606232643f, 0.004837007261812687f,
    0.018911808729171753f, 0.010300683788955212f, 0.014076442457735538f, 0.007690188009291887f,
    0.014194218441843987f, 0.006508346647024155f, 0.011742200702428818f, 0.0074862209148705006f,
    0.01424858532845974f, 0.01401127316057682f, 0.012413840740919113f, 0.0101792486384511f,
    0.009272722527384758f, 0.01097987499088049f, 0.012540258467197418f, 0.010964040644466877f,
    0.005898223724216223f, 0.02129247598350048f, 0.009309301152825356f, 0.014259411953389645f,
    0.01488856878131628f, 0.020930413156747818f, 0.006037757266312838f, 0.012586367316544056f,
    0.014657803811132908f, 0.010762008838355541f, 0.01008851919323206f, 0.009509832598268986f,
    0.01127588376402855f, 0.008757281117141247f, 0.006978197023272514f, 0.007147456053644419f,
    0.01118851825594902f, 0.013171260245144367f, 0.012133876793086529f, 0.01118162926286459f,
    0.019744114950299263f, 0.010614633560180664f, 0.011255858466029167f, 0.006895996630191803f,
    0.010368339717388153f, 0.0066028921864926815f, 0.01577235385775566f, 0.011776172555983067f,
    0.007821165025234222f, 0.007339969277381897f, 0.009733284823596478f, 0.011884145438671112f,
    0.00501478323712945f, 0.016107605770230293f, 0.009192555211484432f, 0.01071314886212349f,
    0.01107263844460249f, 0.012880839407444f, 0.011538336053490639f, 0.009723993018269539f,
    0.012693910859525204f, 0.018367715179920197f, 0.016558131203055382f, 0.011760146357119083f);
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_1_t_out_0_shape_h_const_u16 = 25;

static const ai_u16 conv2d_2_t_in_0_shape_w_const_u16           = 5;
static const ai_u16 conv2d_2_t_in_0_shape_h_const_u16           = 25;
static const ai_u16 conv2d_2_l_stride_1_const_u16               = 1;
static const ai_u16 conv2d_2_l_stride_0_const_u16               = 1;
static const ai_u16 conv2d_2_t_in_0_shape_ch_const_u16          = 64;
static const ai_u16 conv2d_2_t_out_0_shape_ch_const_u16         = 64;
static const ai_i8 conv2d_2_t_in_0_fmt_zero_const_s8            = -128;
static const ai_i8 conv2d_2_t_out_0_fmt_zero_const_s8           = -128;
static const ai_float conv2d_2_t_in_0_fmt_scale_const_f32       = 0.12000501900911331f;
static const ai_float conv2d_2_t_out_0_fmt_scale_const_f32      = 0.08827269822359085f;
static const ai_float conv2d_2_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(
    0.003963490016758442f, 0.002755872905254364f, 0.004067869391292334f, 0.0043415771797299385f,
    0.003715763334184885f, 0.0036767872516065836f, 0.003140085842460394f, 0.003607412800192833f,
    0.001967955846339464f, 0.004737270530313253f, 0.00264090858399868f, 0.004117410164326429f,
    0.0030786737333983183f, 0.0031973456498235464f, 0.0027273453306406736f, 0.004196877591311932f,
    0.003046786179766059f, 0.0027701337821781635f, 0.0027105198241770267f, 0.0025557142216712236f,
    0.003920907620340586f, 0.0035117289517074823f, 0.0041048466227948666f, 0.0033408007584512234f,
    0.0037026037462055683f, 0.0037874802947044373f, 0.0041929916478693485f, 0.003607449820265174f,
    0.0024833178613334894f, 0.002614511176943779f, 0.0031424241606146097f, 0.0030083670280873775f,
    0.0023503191769123077f, 0.003193557495251298f, 0.0032979047391563654f, 0.002320790896192193f,
    0.0026125297881662846f, 0.0032839998602867126f, 0.0032041394151747227f, 0.0028313822112977505f,
    0.003021814161911607f, 0.0029986011795699596f, 0.003500211052596569f, 0.0029350779950618744f,
    0.004062286112457514f, 0.003219494130462408f, 0.0029579265974462032f, 0.002730635227635503f,
    0.002308908384293318f, 0.002342052524909377f, 0.0032265824265778065f, 0.003040324430912733f,
    0.004327393602579832f, 0.004159258212894201f, 0.0034119344782084227f, 0.0037077327724546194f,
    0.0029532129410654306f, 0.0025794461835175753f, 0.0030897115357220173f, 0.0028635591734200716f,
    0.0026733004488050938f, 0.004079430364072323f, 0.0026133668143302202f, 0.003543468192219734f);
static const ai_layer_format_type conv2d_2_l_out_ch_format_const_layer_format_type =
    AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_3_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_3_pad_before_t_in_0_fmt_bitsize_const_s16   = 8;
static const ai_u32 conv2d_3_pad_before_t_in_0_shape_h_const_u32       = 25;

static const ai_u16 conv2d_3_t_in_0_shape_w_const_u16           = 7;
static const ai_u16 conv2d_3_t_in_0_shape_h_const_u16           = 27;
static const ai_u16 conv2d_3_t_in_0_shape_ch_const_u16          = 64;
static const ai_u16 conv2d_3_l_stride_1_const_u16               = 1;
static const ai_u16 conv2d_3_l_stride_0_const_u16               = 1;
static const ai_i8 conv2d_3_t_in_0_fmt_zero_const_s8            = -128;
static const ai_i8 conv2d_3_t_out_0_fmt_zero_const_s8           = -128;
static const ai_float conv2d_3_t_in_0_fmt_scale_const_f32       = 0.08827269822359085f;
static const ai_float conv2d_3_t_out_0_fmt_scale_const_f32      = 0.09993889182806015f;
static const ai_float conv2d_3_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(
    0.006430983077734709f, 0.017153628170490265f, 0.008172271773219109f, 0.009870384819805622f,
    0.008151412941515446f, 0.011643236503005028f, 0.005960006266832352f, 0.009190221317112446f,
    0.0170717965811491f, 0.007036095485091209f, 0.007129925303161144f, 0.009710435755550861f,
    0.005679587833583355f, 0.008473766036331654f, 0.011724391020834446f, 0.010202058590948582f,
    0.013176162727177143f, 0.012233387678861618f, 0.017332691699266434f, 0.0075774104334414005f,
    0.016090869903564453f, 0.009203621186316013f, 0.006274365819990635f, 0.011484040878713131f,
    0.008397572673857212f, 0.007822640240192413f, 0.004946154076606035f, 0.011830611154437065f,
    0.00855720229446888f, 0.013959766365587711f, 0.007699341047555208f, 0.006635550409555435f,
    0.007838292047381401f, 0.00853349082171917f, 0.009238818660378456f, 0.007114894222468138f,
    0.011963951401412487f, 0.005165266804397106f, 0.012342511676251888f, 0.005083670374006033f,
    0.006920337677001953f, 0.009804653003811836f, 0.008623837493360043f, 0.01216980256140232f,
    0.005042268428951502f, 0.005496371071785688f, 0.006960703060030937f, 0.006956654135137796f,
    0.008555966429412365f, 0.010821365751326084f, 0.010477025993168354f, 0.011258051730692387f,
    0.007079053204506636f, 0.0061754449270665646f, 0.00779008911922574f, 0.005782102234661579f,
    0.008867081254720688f, 0.007974298670887947f, 0.00876440480351448f, 0.009480192326009274f,
    0.013686828315258026f, 0.010333091020584106f, 0.01040620356798172f, 0.006096743047237396f);
static const ai_u16 conv2d_3_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_3_t_out_0_shape_h_const_u16 = 25;

static const ai_u16 conv2d_4_t_in_0_shape_w_const_u16           = 5;
static const ai_u16 conv2d_4_t_in_0_shape_h_const_u16           = 25;
static const ai_u16 conv2d_4_l_stride_1_const_u16               = 1;
static const ai_u16 conv2d_4_l_stride_0_const_u16               = 1;
static const ai_u16 conv2d_4_t_in_0_shape_ch_const_u16          = 64;
static const ai_u16 conv2d_4_t_out_0_shape_ch_const_u16         = 64;
static const ai_i8 conv2d_4_t_in_0_fmt_zero_const_s8            = -128;
static const ai_i8 conv2d_4_t_out_0_fmt_zero_const_s8           = -128;
static const ai_float conv2d_4_t_in_0_fmt_scale_const_f32       = 0.09993889182806015f;
static const ai_float conv2d_4_t_out_0_fmt_scale_const_f32      = 0.07527299970388412f;
static const ai_float conv2d_4_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(
    0.0027135093696415424f, 0.0020731945987790823f, 0.0027890612836927176f, 0.002820221707224846f,
    0.0016420644242316484f, 0.004568037576973438f, 0.004581558518111706f, 0.0038642631843686104f,
    0.0027889718767255545f, 0.003145355498418212f, 0.004798292648047209f, 0.002853326965123415f,
    0.0027700220234692097f, 0.002985892351716757f, 0.0020765848457813263f, 0.003336957423016429f,
    0.0023727675434201956f, 0.0032712460961192846f, 0.0029421173967421055f, 0.004149383865296841f,
    0.0036539880093187094f, 0.0022823000326752663f, 0.0030453840736299753f, 0.0031217809300869703f,
    0.0033991611562669277f, 0.004894475918263197f, 0.003503390122205019f, 0.0031373314559459686f,
    0.0021419008262455463f, 0.0034093542490154505f, 0.0038176828529685736f, 0.0032287146896123886f,
    0.004077211022377014f, 0.0031992157455533743f, 0.003507319837808609f, 0.003976774401962757f,
    0.0028490039985626936f, 0.003533195471391082f, 0.002647885587066412f, 0.004225407727062702f,
    0.002997699426487088f, 0.0024334858171641827f, 0.00210359413176775f, 0.004417495336383581f,
    0.003978435415774584f, 0.0037551887799054384f, 0.005134683568030596f, 0.00238763727247715f,
    0.004062139429152012f, 0.0011750654084607959f, 0.0020715869031846523f, 0.003475135425105691f,
    0.001914308057166636f, 0.0025673480704426765f, 0.0031456979922950268f, 0.0019445248181000352f,
    0.0026420624926686287f, 0.0045679593458771706f, 0.0041586835868656635f, 0.0035332890693098307f,
    0.0036369673907756805f, 0.0038360352627933025f, 0.002206296194344759f, 0.00417149206623435f);
static const ai_layer_format_type conv2d_4_l_out_ch_format_const_layer_format_type =
    AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_5_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16   = 8;
static const ai_u32 conv2d_5_pad_before_t_in_0_shape_h_const_u32       = 25;

static const ai_u16 conv2d_5_t_in_0_shape_w_const_u16           = 7;
static const ai_u16 conv2d_5_t_in_0_shape_h_const_u16           = 27;
static const ai_u16 conv2d_5_t_in_0_shape_ch_const_u16          = 64;
static const ai_u16 conv2d_5_l_stride_1_const_u16               = 1;
static const ai_u16 conv2d_5_l_stride_0_const_u16               = 1;
static const ai_i8 conv2d_5_t_in_0_fmt_zero_const_s8            = -128;
static const ai_i8 conv2d_5_t_out_0_fmt_zero_const_s8           = -128;
static const ai_float conv2d_5_t_in_0_fmt_scale_const_f32       = 0.07527299970388412f;
static const ai_float conv2d_5_t_out_0_fmt_scale_const_f32      = 0.07787063717842102f;
static const ai_float conv2d_5_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(
    0.009053298272192478f, 0.008853037841618061f, 0.004339170642197132f, 0.007308301981538534f,
    0.009306405670940876f, 0.0037104233633726835f, 0.005390834994614124f, 0.005346214398741722f,
    0.0063987718895077705f, 0.0068512666039168835f, 0.005725123453885317f, 0.008999169804155827f,
    0.006874947343021631f, 0.011715701781213284f, 0.004494757391512394f, 0.005457875318825245f,
    0.004610867239534855f, 0.005279893055558205f, 0.009861405938863754f, 0.0076576159335672855f,
    0.0035884310491383076f, 0.006549340672791004f, 0.009501714259386063f, 0.0048339697532355785f,
    0.0071243224665522575f, 0.006563055329024792f, 0.0035821155179291964f, 0.005222362466156483f,
    0.015782594680786133f, 0.0071378774009644985f, 0.0048787277191877365f, 0.005474860314279795f,
    0.003674909006804228f, 0.00923983845859766f, 0.005437178537249565f, 0.006282168906182051f,
    0.005941143725067377f, 0.0044320193119347095f, 0.006047272589057684f, 0.009686372242867947f,
    0.008854690007865429f, 0.005728485528379679f, 0.018425285816192627f, 0.006306822877377272f,
    0.00550523167476058f, 0.011167550459504128f, 0.004066616762429476f, 0.010647328570485115f,
    0.0051891920156776905f, 0.04226783663034439f, 0.013102563098073006f, 0.003960825502872467f,
    0.013830640353262424f, 0.006812189240008593f, 0.00567943649366498f, 0.015058964490890503f,
    0.005349782761186361f, 0.004443423822522163f, 0.006428433582186699f, 0.0047588590532541275f,
    0.0058938744477927685f, 0.011215420439839363f, 0.012792836874723434f, 0.00646549416705966f);
static const ai_u16 conv2d_5_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_5_t_out_0_shape_h_const_u16 = 25;

static const ai_u16 conv2d_6_t_in_0_shape_w_const_u16           = 5;
static const ai_u16 conv2d_6_t_in_0_shape_h_const_u16           = 25;
static const ai_u16 conv2d_6_l_stride_1_const_u16               = 1;
static const ai_u16 conv2d_6_l_stride_0_const_u16               = 1;
static const ai_u16 conv2d_6_t_in_0_shape_ch_const_u16          = 64;
static const ai_u16 conv2d_6_t_out_0_shape_ch_const_u16         = 64;
static const ai_i8 conv2d_6_t_in_0_fmt_zero_const_s8            = -128;
static const ai_i8 conv2d_6_t_out_0_fmt_zero_const_s8           = -128;
static const ai_float conv2d_6_t_in_0_fmt_scale_const_f32       = 0.07787063717842102f;
static const ai_float conv2d_6_t_out_0_fmt_scale_const_f32      = 0.056961074471473694f;
static const ai_float conv2d_6_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(
    0.0018432496581226587f, 0.0037960284389555454f, 0.0031696725636720657f, 0.004334271885454655f,
    0.0033289515413343906f, 0.0027708634734153748f, 0.0035625637974590063f, 0.0021311643067747355f,
    0.0037796609103679657f, 0.002088772365823388f, 0.0021839558612555265f, 0.004952239338308573f,
    0.0026996443048119545f, 0.0022640686947852373f, 0.003620152361690998f, 0.0032190352212637663f,
    0.003911955282092094f, 0.003468453884124756f, 0.004956199321895838f, 0.0035683459136635065f,
    0.0038023055531084538f, 0.00464496249333024f, 0.003404441522434354f, 0.002731507644057274f,
    0.0034675325732678175f, 0.002732830820605159f, 0.004459040705114603f, 0.0026127109304070473f,
    0.0032417699694633484f, 0.0040875873528420925f, 0.002594975521788001f, 0.0028799772262573242f,
    0.003385048359632492f, 0.002383168786764145f, 0.0038912948220968246f, 0.0029086191207170486f,
    0.003831245703622699f, 0.003197062760591507f, 0.004469331353902817f, 0.004123328719288111f,
    0.004043919499963522f, 0.0027474185917526484f, 0.0035390311386436224f, 0.004786308854818344f,
    0.0014590035425499082f, 0.005268690176308155f, 0.0024376341607421637f, 0.0015841801650822163f,
    0.0025300385896116495f, 0.0035097047220915556f, 0.0029274008702486753f, 0.0032514960039407015f,
    0.0014007944846525788f, 0.0034610996954143047f, 0.003192467615008354f, 0.00434268219396472f,
    0.0031711156480014324f, 0.005679871421307325f, 0.00215925183147192f, 0.002699106466025114f,
    0.0019817000720649958f, 0.004369467031210661f, 0.002881350927054882f, 0.002501249313354492f);
static const ai_layer_format_type conv2d_6_l_out_ch_format_const_layer_format_type =
    AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_7_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_7_pad_before_t_in_0_fmt_bitsize_const_s16   = 8;
static const ai_u32 conv2d_7_pad_before_t_in_0_shape_h_const_u32       = 25;

static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16           = 7;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16           = 27;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16          = 64;
static const ai_u16 conv2d_7_l_stride_1_const_u16               = 1;
static const ai_u16 conv2d_7_l_stride_0_const_u16               = 1;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8            = -128;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8           = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32       = 0.056961074471473694f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32      = 0.09848222881555557f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(
    0.004199115093797445f, 0.0031683987472206354f, 0.007215726189315319f, 0.0037188020069152117f,
    0.0036819488741457462f, 0.010213126428425312f, 0.003425357397645712f, 0.010177070274949074f,
    0.004632329568266869f, 0.021589195355772972f, 0.00710504874587059f, 0.004780655261129141f,
    0.008033155463635921f, 0.004824831150472164f, 0.0027194833382964134f, 0.005863558035343885f,
    0.004691471811383963f, 0.004821042995899916f, 0.003935585264116526f, 0.004519995301961899f,
    0.007343936245888472f, 0.003881423268467188f, 0.006541970651596785f, 0.011028746142983437f,
    0.003994502127170563f, 0.004877465311437845f, 0.004500470589846373f, 0.016938453540205956f,
    0.006478418130427599f, 0.005873819813132286f, 0.006593191530555487f, 0.006148588377982378f,
    0.003444727510213852f, 0.005598318763077259f, 0.00491905165836215f, 0.004776543937623501f,
    0.007452694699168205f, 0.003948979079723358f, 0.0061589390970766544f, 0.0029126156587153673f,
    0.00519845075905323f, 0.008110293187201023f, 0.007346372120082378f, 0.003741534659639001f,
    0.031140634790062904f, 0.006382977589964867f, 0.0025591684971004725f, 0.02234659716486931f,
    0.006324097514152527f, 0.005375554319471121f, 0.010922390036284924f, 0.004955872893333435f,
    0.04390311613678932f, 0.003956029657274485f, 0.004415792878717184f, 0.004452766850590706f,
    0.009095232002437115f, 0.004136513452976942f, 0.004984394647181034f, 0.0075601572170853615f,
    0.009919692762196064f, 0.00497068976983428f, 0.007703631650656462f, 0.004400250501930714f);
static const ai_u16 conv2d_7_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_7_t_out_0_shape_h_const_u16 = 25;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16           = 5;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16           = 25;
static const ai_u16 conv2d_8_l_stride_1_const_u16               = 1;
static const ai_u16 conv2d_8_l_stride_0_const_u16               = 1;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16          = 64;
static const ai_u16 conv2d_8_t_out_0_shape_ch_const_u16         = 64;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8            = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8           = -128;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32       = 0.09848222881555557f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32      = 0.2629375755786896f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(
    0.01166554819792509f, 0.00734162051230669f, 0.012526342645287514f, 0.00640261871740222f,
    0.009024185128509998f, 0.008653608150780201f, 0.012499815784394741f, 0.012895885854959488f,
    0.00900446716696024f, 0.008137010969221592f, 0.006129717919975519f, 0.009351001121103764f,
    0.009030663408339024f, 0.019503379240632057f, 0.012236570008099079f, 0.007811686024069786f,
    0.007686194963753223f, 0.008599931374192238f, 0.013338305987417698f, 0.009359273128211498f,
    0.009245515801012516f, 0.006473219487816095f, 0.008426409214735031f, 0.008649760857224464f,
    0.008755745366215706f, 0.012533867731690407f, 0.011727864854037762f, 0.005210088100284338f,
    0.010010382160544395f, 0.0063048978336155415f, 0.009405641816556454f, 0.00960275623947382f,
    0.00956809427589178f, 0.011215342208743095f, 0.008577137254178524f, 0.009699815884232521f,
    0.012464207597076893f, 0.011510667391121387f, 0.01077359076589346f, 0.011839736253023148f,
    0.01023457758128643f, 0.011879895813763142f, 0.004753417335450649f, 0.014764453284442425f,
    0.008700372651219368f, 0.007571918424218893f, 0.00937708094716072f, 0.008082053624093533f,
    0.014228133484721184f, 0.0055705476552248f, 0.012838597409427166f, 0.008567171171307564f,
    0.005888978485018015f, 0.011470817029476166f, 0.010259090922772884f, 0.01257992908358574f,
    0.011086429469287395f, 0.015739692375063896f, 0.012603472918272018f, 0.005676951725035906f,
    0.007964556105434895f, 0.008425591513514519f, 0.010108256712555885f, 0.011890439316630363f);
static const ai_layer_format_type conv2d_8_l_out_ch_format_const_layer_format_type =
    AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_u32 nl_11_t_in_0_shape_ch_prod_const_u32 = 12;
STAI_API_ENTRY
stai_return_code stai_network_run(stai_network *network, const stai_run_mode mode)
{
    STAI_UNUSED(mode)
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)

    _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_ACTIVATIONS) != STAI_FLAG_ACTIVATIONS,
                    STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)

    _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_INPUTS) != STAI_FLAG_INPUTS,
                    STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_OUTPUTS) != STAI_FLAG_OUTPUTS,
                    STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)

    _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_WEIGHTS) != STAI_FLAG_WEIGHTS,
                    STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)


    /* LITE_KERNEL_SECTION BEGIN conv2d_0 */
    {
        const ai_i8 *conv2d_0_t_in_0_ptr_const_s8       = (ai_i8 *)(net_ctx->_inputs[0] + 0);
        const ai_i8 *conv2d_0_t_weight_0_ptr_const_s8   = (ai_i8 *)(net_ctx->_weights[0] + 0);
        const ai_i32 *conv2d_0_t_weight_1_ptr_const_s32 = (ai_i32 *)(net_ctx->_weights[0] + 2560);
        ai_i8 *conv2d_0_t_out_0_ptr_s8       = (ai_i8 *)(net_ctx->_activations[0] + 6668);
        ai_i16 *conv2d_0_t_scratch_0_ptr_s16 = (ai_i16 *)(net_ctx->_activations[0] + 492);

        _STAI_NETWORK_EVENT_NODE_START_CB(0, 1, {(stai_ptr)conv2d_0_t_in_0_ptr_const_s8});

        forward_lite_conv2d_sssa8_ch(
            conv2d_0_t_in_0_ptr_const_s8, conv2d_0_t_in_0_shape_w_const_u16,
            conv2d_0_t_in_0_shape_h_const_u16, conv2d_0_t_in_0_shape_ch_const_u16,
            conv2d_0_t_weight_0_ptr_const_s8, conv2d_0_t_out_0_shape_ch_const_u16,
            conv2d_0_t_weight_0_shape_w_const_u16, conv2d_0_t_weight_0_shape_h_const_u16,
            conv2d_0_l_stride_1_const_u16, conv2d_0_l_stride_0_const_u16,
            conv2d_0_l_pad_W_0_const_s32, conv2d_0_l_pad_H_0_const_s32,
            conv2d_0_t_weight_1_ptr_const_s32, conv2d_0_t_in_0_fmt_zero_const_s8,
            conv2d_0_t_out_0_fmt_zero_const_s8, conv2d_0_t_in_0_fmt_scale_const_f32,
            conv2d_0_t_out_0_fmt_scale_const_f32, conv2d_0_t_weight_0_fmt_scale_const_f32,
            conv2d_0_l_out_ch_format_const_layer_format_type, conv2d_0_t_out_0_ptr_s8,
            conv2d_0_t_out_0_shape_w_const_u16, conv2d_0_t_out_0_shape_h_const_u16, 1, 6176,
            conv2d_0_t_scratch_0_ptr_s16);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(0, 1, {(stai_ptr)conv2d_0_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END conv2d_0 */
    /* LITE_KERNEL_SECTION BEGIN conv2d_1_pad_before */
    {
        const ai_ptr conv2d_1_pad_before_t_in_0_ptr_const_ptr =
            (ai_ptr)(net_ctx->_activations[0] + 6668);
        ai_ptr conv2d_1_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 2572);

        _STAI_NETWORK_EVENT_NODE_START_CB(1, 1,
                                          {(stai_ptr)conv2d_1_pad_before_t_in_0_ptr_const_ptr});

        forward_lite_pad_constant(
            conv2d_1_pad_before_t_in_0_ptr_const_ptr, conv2d_1_pad_before_t_out_0_ptr_ptr,
            (ai_handle)(conv2d_1_pad_before_v_pad_constant_value_const_s8),
            conv2d_1_pad_before_t_in_0_fmt_bitsize_const_s16,
            conv2d_1_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(448),
            (ai_i32)(448), (ai_i32)(64), (ai_i32)(64));

        _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr)conv2d_1_pad_before_t_out_0_ptr_ptr});
    }
    /* LITE_KERNEL_SECTION END conv2d_1_pad_before */
    /* LITE_KERNEL_SECTION BEGIN conv2d_1 */
    {
        const ai_i8 *conv2d_1_t_in_0_ptr_const_s8     = (ai_i8 *)(net_ctx->_activations[0] + 2572);
        const ai_i8 *conv2d_1_t_weight_0_ptr_const_s8 = (ai_i8 *)(net_ctx->_weights[0] + 2816);
        const ai_i32 *conv2d_1_t_weight_1_ptr_const_s32 = (ai_i32 *)(net_ctx->_weights[0] + 3392);
        ai_i8 *conv2d_1_t_out_0_ptr_s8       = (ai_i8 *)(net_ctx->_activations[0] + 2252);
        ai_i16 *conv2d_1_t_scratch_0_ptr_s16 = (ai_i16 *)(net_ctx->_activations[0] + 14668);

        _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr)conv2d_1_t_in_0_ptr_const_s8});

        forward_lite_dw_3x3_sssa8_ch(
            conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16,
            conv2d_1_t_in_0_shape_h_const_u16, conv2d_1_t_in_0_shape_ch_const_u16,
            conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_l_stride_1_const_u16,
            conv2d_1_l_stride_0_const_u16, conv2d_1_t_weight_1_ptr_const_s32,
            conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8,
            conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32,
            conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_t_out_0_ptr_s8,
            conv2d_1_t_out_0_shape_w_const_u16, conv2d_1_t_out_0_shape_h_const_u16, 0, 2369,
            conv2d_1_t_scratch_0_ptr_s16);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr)conv2d_1_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END conv2d_1 */
    /* LITE_KERNEL_SECTION BEGIN conv2d_2 */
    {
        const ai_i8 *conv2d_2_t_in_0_ptr_const_s8     = (ai_i8 *)(net_ctx->_activations[0] + 2252);
        const ai_i8 *conv2d_2_t_weight_0_ptr_const_s8 = (ai_i8 *)(net_ctx->_weights[0] + 3648);
        const ai_i32 *conv2d_2_t_weight_1_ptr_const_s32 = (ai_i32 *)(net_ctx->_weights[0] + 7744);
        ai_i8 *conv2d_2_t_out_0_ptr_s8       = (ai_i8 *)(net_ctx->_activations[0] + 10252);
        ai_i16 *conv2d_2_t_scratch_0_ptr_s16 = (ai_i16 *)(net_ctx->_activations[0] + 1356);

        _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr)conv2d_2_t_in_0_ptr_const_s8});

        forward_lite_pw_sssa8_ch(
            conv2d_2_t_in_0_ptr_const_s8, conv2d_2_t_in_0_shape_w_const_u16,
            conv2d_2_t_in_0_shape_h_const_u16, conv2d_2_l_stride_1_const_u16,
            conv2d_2_l_stride_0_const_u16, conv2d_2_t_in_0_shape_ch_const_u16,
            conv2d_2_t_weight_0_ptr_const_s8, conv2d_2_t_out_0_shape_ch_const_u16,
            conv2d_2_t_weight_1_ptr_const_s32, conv2d_2_t_in_0_fmt_zero_const_s8,
            conv2d_2_t_out_0_fmt_zero_const_s8, conv2d_2_t_in_0_fmt_scale_const_f32,
            conv2d_2_t_out_0_fmt_scale_const_f32, conv2d_2_t_weight_0_fmt_scale_const_f32,
            conv2d_2_l_out_ch_format_const_layer_format_type, conv2d_2_t_out_0_ptr_s8, 1, 896,
            conv2d_2_t_scratch_0_ptr_s16);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr)conv2d_2_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END conv2d_2 */
    /* LITE_KERNEL_SECTION BEGIN conv2d_3_pad_before */
    {
        const ai_ptr conv2d_3_pad_before_t_in_0_ptr_const_ptr =
            (ai_ptr)(net_ctx->_activations[0] + 10252);
        ai_ptr conv2d_3_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 6156);

        _STAI_NETWORK_EVENT_NODE_START_CB(3, 1,
                                          {(stai_ptr)conv2d_3_pad_before_t_in_0_ptr_const_ptr});

        forward_lite_pad_constant(
            conv2d_3_pad_before_t_in_0_ptr_const_ptr, conv2d_3_pad_before_t_out_0_ptr_ptr,
            (ai_handle)(conv2d_3_pad_before_v_pad_constant_value_const_s8),
            conv2d_3_pad_before_t_in_0_fmt_bitsize_const_s16,
            conv2d_3_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(448),
            (ai_i32)(448), (ai_i32)(64), (ai_i32)(64));

        _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr)conv2d_3_pad_before_t_out_0_ptr_ptr});
    }
    /* LITE_KERNEL_SECTION END conv2d_3_pad_before */
    /* LITE_KERNEL_SECTION BEGIN conv2d_3 */
    {
        const ai_i8 *conv2d_3_t_in_0_ptr_const_s8     = (ai_i8 *)(net_ctx->_activations[0] + 6156);
        const ai_i8 *conv2d_3_t_weight_0_ptr_const_s8 = (ai_i8 *)(net_ctx->_weights[0] + 8000);
        const ai_i32 *conv2d_3_t_weight_1_ptr_const_s32 = (ai_i32 *)(net_ctx->_weights[0] + 8576);
        ai_i8 *conv2d_3_t_out_0_ptr_s8       = (ai_i8 *)(net_ctx->_activations[0] + 5836);
        ai_i16 *conv2d_3_t_scratch_0_ptr_s16 = (ai_i16 *)(net_ctx->_activations[0] + 0);

        _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr)conv2d_3_t_in_0_ptr_const_s8});

        forward_lite_dw_3x3_sssa8_ch(
            conv2d_3_t_in_0_ptr_const_s8, conv2d_3_t_in_0_shape_w_const_u16,
            conv2d_3_t_in_0_shape_h_const_u16, conv2d_3_t_in_0_shape_ch_const_u16,
            conv2d_3_t_weight_0_ptr_const_s8, conv2d_3_l_stride_1_const_u16,
            conv2d_3_l_stride_0_const_u16, conv2d_3_t_weight_1_ptr_const_s32,
            conv2d_3_t_in_0_fmt_zero_const_s8, conv2d_3_t_out_0_fmt_zero_const_s8,
            conv2d_3_t_in_0_fmt_scale_const_f32, conv2d_3_t_out_0_fmt_scale_const_f32,
            conv2d_3_t_weight_0_fmt_scale_const_f32, conv2d_3_t_out_0_ptr_s8,
            conv2d_3_t_out_0_shape_w_const_u16, conv2d_3_t_out_0_shape_h_const_u16, 0, 2369,
            conv2d_3_t_scratch_0_ptr_s16);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr)conv2d_3_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END conv2d_3 */
    /* LITE_KERNEL_SECTION BEGIN conv2d_4 */
    {
        const ai_i8 *conv2d_4_t_in_0_ptr_const_s8     = (ai_i8 *)(net_ctx->_activations[0] + 5836);
        const ai_i8 *conv2d_4_t_weight_0_ptr_const_s8 = (ai_i8 *)(net_ctx->_weights[0] + 8832);
        const ai_i32 *conv2d_4_t_weight_1_ptr_const_s32 = (ai_i32 *)(net_ctx->_weights[0] + 12928);
        ai_i8 *conv2d_4_t_out_0_ptr_s8       = (ai_i8 *)(net_ctx->_activations[0] + 5516);
        ai_i16 *conv2d_4_t_scratch_0_ptr_s16 = (ai_i16 *)(net_ctx->_activations[0] + 0);

        _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr)conv2d_4_t_in_0_ptr_const_s8});

        forward_lite_pw_sssa8_ch(
            conv2d_4_t_in_0_ptr_const_s8, conv2d_4_t_in_0_shape_w_const_u16,
            conv2d_4_t_in_0_shape_h_const_u16, conv2d_4_l_stride_1_const_u16,
            conv2d_4_l_stride_0_const_u16, conv2d_4_t_in_0_shape_ch_const_u16,
            conv2d_4_t_weight_0_ptr_const_s8, conv2d_4_t_out_0_shape_ch_const_u16,
            conv2d_4_t_weight_1_ptr_const_s32, conv2d_4_t_in_0_fmt_zero_const_s8,
            conv2d_4_t_out_0_fmt_zero_const_s8, conv2d_4_t_in_0_fmt_scale_const_f32,
            conv2d_4_t_out_0_fmt_scale_const_f32, conv2d_4_t_weight_0_fmt_scale_const_f32,
            conv2d_4_l_out_ch_format_const_layer_format_type, conv2d_4_t_out_0_ptr_s8, 1, 896,
            conv2d_4_t_scratch_0_ptr_s16);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr)conv2d_4_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END conv2d_4 */
    /* LITE_KERNEL_SECTION BEGIN conv2d_5_pad_before */
    {
        const ai_ptr conv2d_5_pad_before_t_in_0_ptr_const_ptr =
            (ai_ptr)(net_ctx->_activations[0] + 5516);
        ai_ptr conv2d_5_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 1420);

        _STAI_NETWORK_EVENT_NODE_START_CB(5, 1,
                                          {(stai_ptr)conv2d_5_pad_before_t_in_0_ptr_const_ptr});

        forward_lite_pad_constant(
            conv2d_5_pad_before_t_in_0_ptr_const_ptr, conv2d_5_pad_before_t_out_0_ptr_ptr,
            (ai_handle)(conv2d_5_pad_before_v_pad_constant_value_const_s8),
            conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16,
            conv2d_5_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(448),
            (ai_i32)(448), (ai_i32)(64), (ai_i32)(64));

        _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr)conv2d_5_pad_before_t_out_0_ptr_ptr});
    }
    /* LITE_KERNEL_SECTION END conv2d_5_pad_before */
    /* LITE_KERNEL_SECTION BEGIN conv2d_5 */
    {
        const ai_i8 *conv2d_5_t_in_0_ptr_const_s8     = (ai_i8 *)(net_ctx->_activations[0] + 1420);
        const ai_i8 *conv2d_5_t_weight_0_ptr_const_s8 = (ai_i8 *)(net_ctx->_weights[0] + 13184);
        const ai_i32 *conv2d_5_t_weight_1_ptr_const_s32 = (ai_i32 *)(net_ctx->_weights[0] + 13760);
        ai_i8 *conv2d_5_t_out_0_ptr_s8       = (ai_i8 *)(net_ctx->_activations[0] + 1100);
        ai_i16 *conv2d_5_t_scratch_0_ptr_s16 = (ai_i16 *)(net_ctx->_activations[0] + 13516);

        _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr)conv2d_5_t_in_0_ptr_const_s8});

        forward_lite_dw_3x3_sssa8_ch(
            conv2d_5_t_in_0_ptr_const_s8, conv2d_5_t_in_0_shape_w_const_u16,
            conv2d_5_t_in_0_shape_h_const_u16, conv2d_5_t_in_0_shape_ch_const_u16,
            conv2d_5_t_weight_0_ptr_const_s8, conv2d_5_l_stride_1_const_u16,
            conv2d_5_l_stride_0_const_u16, conv2d_5_t_weight_1_ptr_const_s32,
            conv2d_5_t_in_0_fmt_zero_const_s8, conv2d_5_t_out_0_fmt_zero_const_s8,
            conv2d_5_t_in_0_fmt_scale_const_f32, conv2d_5_t_out_0_fmt_scale_const_f32,
            conv2d_5_t_weight_0_fmt_scale_const_f32, conv2d_5_t_out_0_ptr_s8,
            conv2d_5_t_out_0_shape_w_const_u16, conv2d_5_t_out_0_shape_h_const_u16, 0, 2369,
            conv2d_5_t_scratch_0_ptr_s16);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr)conv2d_5_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END conv2d_5 */
    /* LITE_KERNEL_SECTION BEGIN conv2d_6 */
    {
        const ai_i8 *conv2d_6_t_in_0_ptr_const_s8     = (ai_i8 *)(net_ctx->_activations[0] + 1100);
        const ai_i8 *conv2d_6_t_weight_0_ptr_const_s8 = (ai_i8 *)(net_ctx->_weights[0] + 14016);
        const ai_i32 *conv2d_6_t_weight_1_ptr_const_s32 = (ai_i32 *)(net_ctx->_weights[0] + 18112);
        ai_i8 *conv2d_6_t_out_0_ptr_s8       = (ai_i8 *)(net_ctx->_activations[0] + 9100);
        ai_i16 *conv2d_6_t_scratch_0_ptr_s16 = (ai_i16 *)(net_ctx->_activations[0] + 0);

        _STAI_NETWORK_EVENT_NODE_START_CB(6, 1, {(stai_ptr)conv2d_6_t_in_0_ptr_const_s8});

        forward_lite_pw_sssa8_ch(
            conv2d_6_t_in_0_ptr_const_s8, conv2d_6_t_in_0_shape_w_const_u16,
            conv2d_6_t_in_0_shape_h_const_u16, conv2d_6_l_stride_1_const_u16,
            conv2d_6_l_stride_0_const_u16, conv2d_6_t_in_0_shape_ch_const_u16,
            conv2d_6_t_weight_0_ptr_const_s8, conv2d_6_t_out_0_shape_ch_const_u16,
            conv2d_6_t_weight_1_ptr_const_s32, conv2d_6_t_in_0_fmt_zero_const_s8,
            conv2d_6_t_out_0_fmt_zero_const_s8, conv2d_6_t_in_0_fmt_scale_const_f32,
            conv2d_6_t_out_0_fmt_scale_const_f32, conv2d_6_t_weight_0_fmt_scale_const_f32,
            conv2d_6_l_out_ch_format_const_layer_format_type, conv2d_6_t_out_0_ptr_s8, 1, 896,
            conv2d_6_t_scratch_0_ptr_s16);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, {(stai_ptr)conv2d_6_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END conv2d_6 */
    /* LITE_KERNEL_SECTION BEGIN conv2d_7_pad_before */
    {
        const ai_ptr conv2d_7_pad_before_t_in_0_ptr_const_ptr =
            (ai_ptr)(net_ctx->_activations[0] + 9100);
        ai_ptr conv2d_7_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 5004);

        _STAI_NETWORK_EVENT_NODE_START_CB(7, 1,
                                          {(stai_ptr)conv2d_7_pad_before_t_in_0_ptr_const_ptr});

        forward_lite_pad_constant(
            conv2d_7_pad_before_t_in_0_ptr_const_ptr, conv2d_7_pad_before_t_out_0_ptr_ptr,
            (ai_handle)(conv2d_7_pad_before_v_pad_constant_value_const_s8),
            conv2d_7_pad_before_t_in_0_fmt_bitsize_const_s16,
            conv2d_7_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(448),
            (ai_i32)(448), (ai_i32)(64), (ai_i32)(64));

        _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr)conv2d_7_pad_before_t_out_0_ptr_ptr});
    }
    /* LITE_KERNEL_SECTION END conv2d_7_pad_before */
    /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
    {
        const ai_i8 *conv2d_7_t_in_0_ptr_const_s8     = (ai_i8 *)(net_ctx->_activations[0] + 5004);
        const ai_i8 *conv2d_7_t_weight_0_ptr_const_s8 = (ai_i8 *)(net_ctx->_weights[0] + 18368);
        const ai_i32 *conv2d_7_t_weight_1_ptr_const_s32 = (ai_i32 *)(net_ctx->_weights[0] + 18944);
        ai_i8 *conv2d_7_t_out_0_ptr_s8       = (ai_i8 *)(net_ctx->_activations[0] + 4684);
        ai_i16 *conv2d_7_t_scratch_0_ptr_s16 = (ai_i16 *)(net_ctx->_activations[0] + 0);

        _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr)conv2d_7_t_in_0_ptr_const_s8});

        forward_lite_dw_3x3_sssa8_ch(
            conv2d_7_t_in_0_ptr_const_s8, conv2d_7_t_in_0_shape_w_const_u16,
            conv2d_7_t_in_0_shape_h_const_u16, conv2d_7_t_in_0_shape_ch_const_u16,
            conv2d_7_t_weight_0_ptr_const_s8, conv2d_7_l_stride_1_const_u16,
            conv2d_7_l_stride_0_const_u16, conv2d_7_t_weight_1_ptr_const_s32,
            conv2d_7_t_in_0_fmt_zero_const_s8, conv2d_7_t_out_0_fmt_zero_const_s8,
            conv2d_7_t_in_0_fmt_scale_const_f32, conv2d_7_t_out_0_fmt_scale_const_f32,
            conv2d_7_t_weight_0_fmt_scale_const_f32, conv2d_7_t_out_0_ptr_s8,
            conv2d_7_t_out_0_shape_w_const_u16, conv2d_7_t_out_0_shape_h_const_u16, 0, 2369,
            conv2d_7_t_scratch_0_ptr_s16);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr)conv2d_7_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END conv2d_7 */
    /* LITE_KERNEL_SECTION BEGIN conv2d_8 */
    {
        const ai_i8 *conv2d_8_t_in_0_ptr_const_s8     = (ai_i8 *)(net_ctx->_activations[0] + 4684);
        const ai_i8 *conv2d_8_t_weight_0_ptr_const_s8 = (ai_i8 *)(net_ctx->_weights[0] + 19200);
        const ai_i32 *conv2d_8_t_weight_1_ptr_const_s32 = (ai_i32 *)(net_ctx->_weights[0] + 23296);
        ai_i8 *conv2d_8_t_out_0_ptr_s8       = (ai_i8 *)(net_ctx->_activations[0] + 4364);
        ai_i16 *conv2d_8_t_scratch_0_ptr_s16 = (ai_i16 *)(net_ctx->_activations[0] + 0);

        _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr)conv2d_8_t_in_0_ptr_const_s8});

        forward_lite_pw_sssa8_ch(
            conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16,
            conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_l_stride_1_const_u16,
            conv2d_8_l_stride_0_const_u16, conv2d_8_t_in_0_shape_ch_const_u16,
            conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_out_0_shape_ch_const_u16,
            conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8,
            conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32,
            conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32,
            conv2d_8_l_out_ch_format_const_layer_format_type, conv2d_8_t_out_0_ptr_s8, 1, 896,
            conv2d_8_t_scratch_0_ptr_s16);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr)conv2d_8_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END conv2d_8 */
    /* LITE_KERNEL_SECTION BEGIN pool_9 */
    {

        forward_lite_ap_integer_INT8_pool_9(net_ctx);
    }
    /* LITE_KERNEL_SECTION END pool_9 */
    /* LITE_KERNEL_SECTION BEGIN gemm_10 */
    {

        forward_lite_dense_integer_SSSA_ch_gemm_10(net_ctx);
    }
    /* LITE_KERNEL_SECTION END gemm_10 */
    /* LITE_KERNEL_SECTION BEGIN nl_11 */
    {
        ai_i8 *nl_11_t_out_0_ptr_s8            = (ai_i8 *)(net_ctx->_outputs[0] + 0);
        const ai_i8 *nl_11_t_in_0_ptr_const_s8 = (ai_i8 *)(net_ctx->_activations[0] + 312);
        ai_i32 *nl_11_t_scratch_0_ptr_s32      = (ai_i32 *)(net_ctx->_activations[0] + 324);

        _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr)nl_11_t_in_0_ptr_const_s8});

        forward_lite_nl_softmax_is8os8(nl_11_t_out_0_ptr_s8, nl_11_t_in_0_ptr_const_s8,
                                       nl_11_t_in_0_shape_ch_prod_const_u32, 1, 12, 1951660032, 24,
                                       -124, nl_11_t_scratch_0_ptr_s32);

        _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr)nl_11_t_out_0_ptr_s8});
    }
    /* LITE_KERNEL_SECTION END nl_11 */
    return net_ctx->_return_code;
}

/*****************************************************************************/
/*  Getters APIs Section  */
STAI_API_ENTRY
stai_size stai_network_get_context_size()
{
    return (stai_size)STAI_NETWORK_CONTEXT_SIZE;
}

#if defined(HAVE_NETWORK_INFO)
STAI_API_ENTRY
stai_return_code stai_network_get_info(stai_network *network, stai_network_info *info)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    _STAI_SET_ERROR(net_ctx, info == NULL, STAI_ERROR_NETWORK_INVALID_INFO, net_ctx->_return_code)

    // Copy of network info struct
    *info = g_network_info;

    return STAI_SUCCESS;
}
#endif


STAI_API_ENTRY
stai_return_code stai_network_get_activations(stai_network *network, stai_ptr *activations,
                                              stai_size *n_activations)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)

    _STAI_SET_ERROR(net_ctx, !n_activations, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS,
                    net_ctx->_return_code)
    *n_activations = STAI_NETWORK_ACTIVATIONS_NUM;
    for (stai_size idx = 0; activations && (idx < STAI_NETWORK_ACTIVATIONS_NUM); idx++)
    {
        // get address of the activations buffers
        activations[idx] = net_ctx->_activations[idx];
    }
    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_weights(stai_network *network, stai_ptr *weights,
                                          stai_size *n_weights)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    _STAI_SET_ERROR(net_ctx, !n_weights, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS,
                    net_ctx->_return_code)
    *n_weights = STAI_NETWORK_WEIGHTS_NUM;
    for (stai_size idx = 0; weights && (idx < STAI_NETWORK_WEIGHTS_NUM); idx++)
    {
        // get address of the weights buffers
        weights[idx] = net_ctx->_weights[idx];
    }
    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_inputs(stai_network *network, stai_ptr *inputs,
                                         stai_size *n_inputs)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    _STAI_SET_ERROR(net_ctx, !n_inputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS,
                    net_ctx->_return_code)
    *n_inputs = STAI_NETWORK_IN_NUM;
    for (stai_size idx = 0; inputs && (idx < STAI_NETWORK_IN_NUM); idx++)
    {
        inputs[idx] = net_ctx->_inputs[idx];
    }
    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_outputs(stai_network *network, stai_ptr *outputs,
                                          stai_size *n_outputs)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    _STAI_SET_ERROR(net_ctx, !n_outputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS,
                    net_ctx->_return_code)
    *n_outputs = STAI_NETWORK_OUT_NUM;
    for (stai_size idx = 0; outputs && (idx < STAI_NETWORK_OUT_NUM); idx++)
    {
        outputs[idx] = net_ctx->_outputs[idx];
    }
    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_error(stai_network *network)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)

    /* return 1st generated error or STAI_SUCCESS if no errors so far */
    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_states(stai_network *network, stai_ptr *states,
                                         stai_size *n_states)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    _STAI_SET_ERROR(net_ctx, !n_states, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS,
                    net_ctx->_return_code)
    /* get the number of internals states (supporting multi-heap also for internal states) */
    *n_states = STAI_NETWORK_STATES_NUM;

    STAI_UNUSED(states)
    return net_ctx->_return_code;
}


/*****************************************************************************/
/*  Setters APIs Section  */

STAI_API_ENTRY
stai_return_code stai_network_set_activations(stai_network *network, const stai_ptr *activations,
                                              const stai_size n_activations)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    const uintptr_t _activations_alignment[] = STAI_NETWORK_ACTIVATIONS_ALIGNMENTS;
    STAI_PRINT("  [stai_network_set_activations] network(%p) activations[%d]: %p\n\n", net_ctx,
               n_activations, activations)
    _STAI_SET_ERROR(net_ctx, !activations, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS,
                    net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, n_activations != STAI_NETWORK_ACTIVATIONS_NUM,
                    STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_NUM, net_ctx->_return_code)

    for (stai_size idx = 0; activations && idx < STAI_NETWORK_ACTIVATIONS_NUM; idx++)
    {
        STAI_PRINT("  activation[%d]: %p\n", idx, activations[idx])
        _STAI_SET_ERROR(net_ctx, activations[idx] == NULL,
                        STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)
        _STAI_SET_ERROR(net_ctx, ((uintptr_t)activations[idx]) & (_activations_alignment[idx] - 1),
                        STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
        net_ctx->_activations[idx] = activations[idx];
    }
    net_ctx->_inputs[0] = activations[0] + 0;

    net_ctx->_outputs[0] = activations[0] + 0;
    _stai_network_check(net_ctx);
    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_weights(stai_network *network, const stai_ptr *weights,
                                          const stai_size n_weights)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    const uintptr_t _weights_alignment[] = STAI_NETWORK_WEIGHTS_ALIGNMENTS;
    _STAI_SET_ERROR(net_ctx, !weights, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS,
                    net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, n_weights != STAI_NETWORK_WEIGHTS_NUM,
                    STAI_ERROR_NETWORK_INVALID_WEIGHTS_NUM, net_ctx->_return_code)
    for (stai_size idx = 0; weights && idx < STAI_NETWORK_WEIGHTS_NUM; idx++)
    {
        STAI_PRINT("  weight[%d]: %p\n", idx, weights[idx])
        _STAI_SET_ERROR(net_ctx, weights[idx] == NULL, STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR,
                        net_ctx->_return_code)
        _STAI_SET_ERROR(net_ctx, ((uintptr_t)weights[idx]) & (_weights_alignment[idx] - 1),
                        STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
        net_ctx->_weights[idx] = weights[idx];
    }
    _stai_network_check(net_ctx);
    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_inputs(stai_network *network, const stai_ptr *inputs,
                                         const stai_size n_inputs)
{
    const uintptr_t _inputs_alignment[] = STAI_NETWORK_IN_ALIGNMENTS;
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    _STAI_SET_ERROR(net_ctx, !inputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS,
                    net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, n_inputs != STAI_NETWORK_IN_NUM, STAI_ERROR_NETWORK_INVALID_IN_NUM,
                    net_ctx->_return_code)

    for (stai_size idx = 0; inputs && idx < STAI_NETWORK_IN_NUM; idx++)
    {
        STAI_PRINT("  input[%d]: %p\n", idx, inputs[idx])
        _STAI_SET_ERROR(net_ctx, inputs[idx] == NULL, STAI_ERROR_NETWORK_INVALID_IN_PTR,
                        net_ctx->_return_code)
        _STAI_SET_ERROR(net_ctx, ((uintptr_t)inputs[idx]) & (_inputs_alignment[idx] - 1),
                        STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
        net_ctx->_inputs[idx] = inputs[idx];
    }

    _stai_network_check(net_ctx);
    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_outputs(stai_network *network, const stai_ptr *outputs,
                                          const stai_size n_outputs)
{
    const uintptr_t _outputs_alignment[] = STAI_NETWORK_OUT_ALIGNMENTS;
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    _STAI_SET_ERROR(net_ctx, !outputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS,
                    net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, n_outputs != STAI_NETWORK_OUT_NUM, STAI_ERROR_NETWORK_INVALID_OUT_NUM,
                    net_ctx->_return_code)

    for (stai_size idx = 0; outputs && idx < n_outputs; idx++)
    {
        STAI_PRINT("  output[%d]: %p\n", idx, outputs[idx])
        _STAI_SET_ERROR(net_ctx, outputs[idx] == NULL, STAI_ERROR_NETWORK_INVALID_OUT_PTR,
                        net_ctx->_return_code)
        _STAI_SET_ERROR(net_ctx, ((uintptr_t)outputs[idx]) & (_outputs_alignment[idx] - 1),
                        STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
        net_ctx->_outputs[idx] = outputs[idx];
    }

    _stai_network_check(net_ctx);
    return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_states(stai_network *network, const stai_ptr *states,
                                         const stai_size n_states)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)

    STAI_UNUSED(states)
    STAI_UNUSED(n_states)
    _stai_network_check(net_ctx);
    return net_ctx->_return_code;
}

STAI_API_ENTRY
stai_return_code stai_network_set_callback(stai_network *network, const stai_event_cb cb,
                                           void *cb_cookie)
{
    _STAI_CONTEXT_ACQUIRE(net_ctx, network)
    STAI_PRINT("  set_callback %p cb %p cookie %p\n", net_ctx, cb, cb_cookie)
    // _STAI_SET_ERROR(net_ctx, cb==NULL, STAI_ERROR_NETWORK_INVALID_CALLBACK,
    // net_ctx->_return_code)
    net_ctx->_callback        = cb;
    net_ctx->_callback_cookie = cb_cookie;
    return net_ctx->_return_code;
}

#undef _STAI_SET_ERROR
#undef _STAI_CONTEXT_ALIGNMENT
#undef _STAI_CONTEXT_ACQUIRE
#undef _STAI_NETWORK_EVENT_NODE_START_CB
#undef _STAI_NETWORK_EVENT_NODE_STOP_CB
#undef _STAI_NETWORK_MODEL_SIGNATURE
#undef _STAI_NETWORK_DATETIME
#undef _STAI_NETWORK_COMPILE_DATETIME
