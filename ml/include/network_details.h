/**
 ******************************************************************************
 * @file    network.h
 * @date    2026-09-10T22:42:20+0200
 * @brief   ST.AI Tool Automatic Code Generator for Embedded NN computing
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
#ifndef STAI_NETWORK_DETAILS_H
#define STAI_NETWORK_DETAILS_H

#include "layers.h"
#include "stai.h"

const stai_network_details g_network_details = {
    .tensors = (const stai_tensor[17]){{.size_bytes = 490,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 49, 10, 1}},
                                        .scale     = {1, (const float[1]){0.6043851971626282}},
                                        .zeropoint = {1, (const int16_t[1]){76}},
                                        .name      = "serving_default_keras_tensor_300_output"},
                                       {.size_bytes = 8000,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 25, 5, 64}},
                                        .scale     = {1, (const float[1]){0.10601875185966492}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_0_output"},
                                       {.size_bytes = 12096,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 27, 7, 64}},
                                        .scale     = {1, (const float[1]){0.10601875185966492}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_1_pad_before_output"},
                                       {.size_bytes = 8000,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 25, 5, 64}},
                                        .scale     = {1, (const float[1]){0.12000501900911331}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_1_output"},
                                       {.size_bytes = 8000,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 25, 5, 64}},
                                        .scale     = {1, (const float[1]){0.08827269822359085}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_2_output"},
                                       {.size_bytes = 12096,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 27, 7, 64}},
                                        .scale     = {1, (const float[1]){0.08827269822359085}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_3_pad_before_output"},
                                       {.size_bytes = 8000,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 25, 5, 64}},
                                        .scale     = {1, (const float[1]){0.09993889182806015}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_3_output"},
                                       {.size_bytes = 8000,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 25, 5, 64}},
                                        .scale     = {1, (const float[1]){0.07527299970388412}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_4_output"},
                                       {.size_bytes = 12096,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 27, 7, 64}},
                                        .scale     = {1, (const float[1]){0.07527299970388412}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_5_pad_before_output"},
                                       {.size_bytes = 8000,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 25, 5, 64}},
                                        .scale     = {1, (const float[1]){0.07787063717842102}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_5_output"},
                                       {.size_bytes = 8000,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 25, 5, 64}},
                                        .scale     = {1, (const float[1]){0.056961074471473694}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_6_output"},
                                       {.size_bytes = 12096,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 27, 7, 64}},
                                        .scale     = {1, (const float[1]){0.056961074471473694}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_7_pad_before_output"},
                                       {.size_bytes = 8000,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 25, 5, 64}},
                                        .scale     = {1, (const float[1]){0.09848222881555557}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_7_output"},
                                       {.size_bytes = 8000,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 25, 5, 64}},
                                        .scale     = {1, (const float[1]){0.2629375755786896}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "conv2d_8_output"},
                                       {.size_bytes = 64,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {4, (const int32_t[4]){1, 1, 1, 64}},
                                        .scale     = {1, (const float[1]){0.0300661101937294}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "pool_9_output"},
                                       {.size_bytes = 12,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {2, (const int32_t[2]){1, 12}},
                                        .scale     = {1, (const float[1]){0.2272031307220459}},
                                        .zeropoint = {1, (const int16_t[1]){99}},
                                        .name      = "gemm_10_output"},
                                       {.size_bytes = 12,
                                        .flags     = (STAI_FLAG_HAS_BATCH | STAI_FLAG_CHANNEL_LAST),
                                        .format    = STAI_FORMAT_S8,
                                        .shape     = {2, (const int32_t[2]){1, 12}},
                                        .scale     = {1, (const float[1]){0.00390625}},
                                        .zeropoint = {1, (const int16_t[1]){-128}},
                                        .name      = "nl_11_output"}},
    .nodes =
        (const stai_node_details[16]){
            {.id             = 0,
             .type           = AI_LAYER_CONV2D_TYPE,
             .input_tensors  = {1, (const int32_t[1]){0}},
             .output_tensors = {1, (const int32_t[1]){1}}}, /* conv2d_0 */
            {.id             = 1,
             .type           = AI_LAYER_PAD_TYPE,
             .input_tensors  = {1, (const int32_t[1]){1}},
             .output_tensors = {1, (const int32_t[1]){2}}}, /* conv2d_1_pad_before */
            {.id             = 1,
             .type           = AI_LAYER_CONV2D_TYPE,
             .input_tensors  = {1, (const int32_t[1]){2}},
             .output_tensors = {1, (const int32_t[1]){3}}}, /* conv2d_1 */
            {.id             = 2,
             .type           = AI_LAYER_CONV2D_TYPE,
             .input_tensors  = {1, (const int32_t[1]){3}},
             .output_tensors = {1, (const int32_t[1]){4}}}, /* conv2d_2 */
            {.id             = 3,
             .type           = AI_LAYER_PAD_TYPE,
             .input_tensors  = {1, (const int32_t[1]){4}},
             .output_tensors = {1, (const int32_t[1]){5}}}, /* conv2d_3_pad_before */
            {.id             = 3,
             .type           = AI_LAYER_CONV2D_TYPE,
             .input_tensors  = {1, (const int32_t[1]){5}},
             .output_tensors = {1, (const int32_t[1]){6}}}, /* conv2d_3 */
            {.id             = 4,
             .type           = AI_LAYER_CONV2D_TYPE,
             .input_tensors  = {1, (const int32_t[1]){6}},
             .output_tensors = {1, (const int32_t[1]){7}}}, /* conv2d_4 */
            {.id             = 5,
             .type           = AI_LAYER_PAD_TYPE,
             .input_tensors  = {1, (const int32_t[1]){7}},
             .output_tensors = {1, (const int32_t[1]){8}}}, /* conv2d_5_pad_before */
            {.id             = 5,
             .type           = AI_LAYER_CONV2D_TYPE,
             .input_tensors  = {1, (const int32_t[1]){8}},
             .output_tensors = {1, (const int32_t[1]){9}}}, /* conv2d_5 */
            {.id             = 6,
             .type           = AI_LAYER_CONV2D_TYPE,
             .input_tensors  = {1, (const int32_t[1]){9}},
             .output_tensors = {1, (const int32_t[1]){10}}}, /* conv2d_6 */
            {.id             = 7,
             .type           = AI_LAYER_PAD_TYPE,
             .input_tensors  = {1, (const int32_t[1]){10}},
             .output_tensors = {1, (const int32_t[1]){11}}}, /* conv2d_7_pad_before */
            {.id             = 7,
             .type           = AI_LAYER_CONV2D_TYPE,
             .input_tensors  = {1, (const int32_t[1]){11}},
             .output_tensors = {1, (const int32_t[1]){12}}}, /* conv2d_7 */
            {.id             = 8,
             .type           = AI_LAYER_CONV2D_TYPE,
             .input_tensors  = {1, (const int32_t[1]){12}},
             .output_tensors = {1, (const int32_t[1]){13}}}, /* conv2d_8 */
            {.id             = 9,
             .type           = AI_LAYER_POOL_TYPE,
             .input_tensors  = {1, (const int32_t[1]){13}},
             .output_tensors = {1, (const int32_t[1]){14}}}, /* pool_9 */
            {.id             = 10,
             .type           = AI_LAYER_DENSE_TYPE,
             .input_tensors  = {1, (const int32_t[1]){14}},
             .output_tensors = {1, (const int32_t[1]){15}}}, /* gemm_10 */
            {.id             = 11,
             .type           = AI_LAYER_SM_TYPE,
             .input_tensors  = {1, (const int32_t[1]){15}},
             .output_tensors = {1, (const int32_t[1]){16}}} /* nl_11 */
        },
    .n_nodes = 16};
#endif
