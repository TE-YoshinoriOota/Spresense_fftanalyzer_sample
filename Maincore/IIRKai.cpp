/*
 *  IIRKai.cpp - IIR(biquad cascade) Kai Library modified based on IIR Library
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

#include "IIRKai.h"

IIRClassKai::IIRClassKai() {
  end();
}

IIRClassKai::~IIRClassKai() {
  end();
}


bool IIRClassKai::begin(filterType_t type, int cutoff, int srate, float q)
{
  begin(type, cutoff, srate, q, DEFAULT_FRAMSIZE, Planar);
}

bool IIRClassKai::begin(filterType_t type, int cutoff, int srate, float q, int sample, format_t output)
{
  if(sample < MIN_FRAMSIZE) return false;

  m_framesize = sample;

  if(create_coef(type, cutoff, srate, q) == false){
    return false;
  }

  arm_biquad_cascade_df2T_init_f32(&S,NUM_SECTIONS_IIR,&m_coef[0],&m_buffer[0]);

  return true;
}

void IIRClassKai::end()
{
  memset(m_coef, 0, sizeof(float32_t)*NUM_COEFF);
  memset(m_buffer, 0, sizeof(float32_t)*NUM_COEFF);
}

bool IIRClassKai::create_coef(filterType_t type, int cutoff, int srate, float q)
{
  float w,k0,k1,a0,a1,a2,b0,b1,b2;

  w = 2.0f * PI * cutoff / srate;

  a1 = -2.0f * cos(w);

  switch(type){
  case (TYPE_LPF):
  case (TYPE_HPF):
    k0 = sin(w) / (2.0f * q);

    a0 =  1.0f + k0;
    a2 =  1.0f - k0;

    break;
  case (TYPE_BPF):
  case (TYPE_BEF):
    k0 = sin(w) * sinh(log(2.0f) / 2.0 * q * w / sin(w));
    a0 =  1.0f + k0;
    a2 =  1.0f - k0;
    break;
  default:
    return false;
  }

  switch(type){
  case TYPE_LPF:
    k1 = 1.0f - cos(w);
    b0 = k1 / 2.0f;
    b1 = k1;
    b2 = k1 / 2.0f;
    break;
  case TYPE_HPF:
    k1 = 1.0f + cos(w);
    b0 = k1 / 2.0f;
    b1 = -k1;
    b2 = k1 / 2.0f;
    break;
  case TYPE_BPF:
    b0 =  k0;
    b1 =  0.0f;
    b2 = -k0;
    break;
  case TYPE_BEF:
    b0 =  1.0f;
    b1 = -2.0f * cos(w);
    b2 =  1.0f;
    break;
  default:
    return false;
  }

  m_coef[0] = b0/a0;
  m_coef[1] = b1/a0;
  m_coef[2] = b2/a0;
  m_coef[3] = -(a1/a0);
  m_coef[4] = -(a2/a0);

  return true;
}


int IIRClassKai::get(float* pDst, float* pSrc, int sample)
{
  if (sample != m_framesize) return -1;
  arm_biquad_cascade_df2T_f32(&S, pSrc, pDst, m_framesize);
  return m_framesize;
}
