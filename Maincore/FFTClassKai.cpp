#include "FFTClassKai.h"


/*------------------------------------------------------------------*/
/* Input buffer                                                      */
/*------------------------------------------------------------------*/

FFTClassKai::FFTClassKai(int samples) {
  m_FFTLEN = samples;
  tmpOutBuf = (float*)malloc(sizeof(float)*m_FFTLEN);
  coef = (float*)malloc(sizeof(float)*m_FFTLEN);
}

FFTClassKai::~FFTClassKai() {
  if (tmpOutBuf != NULL) free((float*)tmpOutBuf);
  if (coef != NULL) free((float*)coef);
  tmpOutBuf = NULL;
  coef = NULL;
}

bool FFTClassKai::begin(windowType_t type, float overlap){

  m_overlap = m_FFTLEN*overlap;

  clear();
  create_coef(type);
  if (!fft_init()) {
     return false;
  }

  return true;
}

void FFTClassKai::clear(){
  memset(tmpOutBuf, 0, sizeof(float)*m_FFTLEN);
}

void FFTClassKai::end(){}

void FFTClassKai::create_coef(windowType_t type){
  for (int i = 0; i < m_FFTLEN/2; i++){
    if (type == WindowHamming) {
      coef[i] = 0.54f - (0.46f * arm_cos_f32(2 * PI * (float)i / (m_FFTLEN - 1)));
    } else if(type == WindowHanning) {
      coef[i] = 0.54f - (1.0f * arm_cos_f32(2 * PI * (float)i / (m_FFTLEN - 1)));
    } else if (type == WindowFlattop) {
        coef[i] = 0.21557895f - (0.41663158f  * arm_cos_f32(2 * PI * (float)i / (m_FFTLEN - 1)))
                              + (0.277263158f * arm_cos_f32(4 * PI * (float)i / (m_FFTLEN - 1)))
                              - (0.083578947f * arm_cos_f32(6 * PI * (float)i / (m_FFTLEN - 1)))
                              + (0.006947368f * arm_cos_f32(8 * PI * (float)i / (m_FFTLEN - 1)));

    } else {
      coef[i] = 1;
    }
    coef[m_FFTLEN-1-i] = coef[i];
  }
}

bool FFTClassKai::fft_init(){
  printf("m_FFTLEN: %d\n", m_FFTLEN);
  switch (m_FFTLEN){
  case 256:
    arm_rfft_256_fast_init_f32(&S);
    break;
  case 512:
    arm_rfft_512_fast_init_f32(&S);
    break;
  case 1024:
    arm_rfft_1024_fast_init_f32(&S);
    break;
  case 2048:
    arm_rfft_2048_fast_init_f32(&S);
    break;
  case 4096:
    arm_rfft_4096_fast_init_f32(&S);
    break;
  default:
    printf("Error: %d no support\n", m_FFTLEN);
    return false;
  }
  return true;
}

void FFTClassKai::fft(float* pDst, float* pSrc){
  arm_rfft_fast_f32(&S, pSrc, pDst, 0);
}

void FFTClassKai::fft_amp(float* pDst, float* pSrc){
  /* calculation */
  arm_rfft_fast_f32(&S, pSrc, tmpOutBuf, 0);
  arm_cmplx_mag_f32(&tmpOutBuf[2], &pDst[1], m_FFTLEN/2 - 1);
  pDst[0] = tmpOutBuf[0];
  pDst[m_FFTLEN/2] = tmpOutBuf[1];
}
