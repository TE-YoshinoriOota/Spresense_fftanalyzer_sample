/*
 *  IIRKai.h - IIRKai(biquad cascade) Library Header
 *  Copyright 2021 Sony Semiconductor Solutions Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _IIR_KAI_H_
#define _IIR_KAI_H_

/* Use CMSIS library */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>

#include "RingBuff.h"

/*------------------------------------------------------------------*/
/* Configurations                                                   */
/*------------------------------------------------------------------*/
/* Select Data Bit length */

#define BITLEN 16
//#define BITLEN 32

/* Execute sample of frame */
#define DEFAULT_FRAMSIZE 768

/* Min sample of frame */
#define MIN_FRAMSIZE 256

/* Execute sample of frame */
#define FRAMSIZE DEFAULT_FRAMSIZE

#define NUM_SECTIONS_IIR 1
#define NUM_COEFF 5

/*------------------------------------------------------------------*/
/* Type Definition                                                  */
/*------------------------------------------------------------------*/
/* FILTER TYPE */
typedef enum e_filterType {
  TYPE_LPF,
  TYPE_HPF,
  TYPE_BPF,
  TYPE_BEF
} filterType_t;

/* FILTER TYPE */
typedef enum e_format {
  Interleave,
  Planar
} format_t;

/*------------------------------------------------------------------*/
/* Input buffer                                                      */
/*------------------------------------------------------------------*/
class IIRClassKai
{
public:
  IIRClassKai();
  ~IIRClassKai();
  bool begin(filterType_t type, int cutoff, int srate, float q);
  bool begin(filterType_t type, int cutoff, int srate, float q, int sample, format_t output);
  int  get(float* pDst, float* pSrc, int sample);
  void end();

private:
  int m_framesize;

  arm_biquad_cascade_df2T_instance_f32 S;

  float32_t m_coef[NUM_COEFF];
  float32_t m_buffer[NUM_COEFF];

  bool create_coef(filterType_t, int cutoff, int srate, float q);
};

#endif /*_IIR_KAI_H_*/
