/*
 *  FFT.h - FFT Library
 *  Copyright 2019 Sony Semiconductor Solutions Corporation
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

#ifndef _FFT_H_
#define _FFT_H_

/* Use CMSIS library */
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>
#include <stdlib.h>
#include <stdio.h>

/*------------------------------------------------------------------*/
/* Type Definition                                                  */
/*------------------------------------------------------------------*/
/* WINDOW TYPE */
typedef enum e_windowType {
  WindowHamming,
  WindowHanning,
  WindowFlattop,
  WindowRectangle
} windowType_t;

/*------------------------------------------------------------------*/
/* Input buffer                                                      */
/*------------------------------------------------------------------*/
class FFTClassKai
{
public:
  FFTClassKai(int FFTLEN);
  ~FFTClassKai();
  bool begin(windowType_t type, float overlap);
  void fft(float* pDst, float* pSrc);
  void fft_amp(float* pDst, float* pSrc);
  void clear();
  void end();

private:
  int m_FFTLEN;
  int m_overlap;
  arm_rfft_fast_instance_f32 S;

  /* Temporary buffer */
  float* coef;
  float* tmpOutBuf;

  void create_coef(windowType_t type);
  bool fft_init();
};

#endif /*_FFT_H_*/
