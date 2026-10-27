#include "FFTClassKai.h"


/*------------------------------------------------------------------*/
/* Input buffer                                                      */
/*------------------------------------------------------------------*/

FFTClassKai::FFTClassKai(int samples) {
  m_FFTLEN = samples;
  if (m_FFTLEN > FFT_CMSIS_MAX) {
    tmpOutBuf = NULL;
    coef = NULL;
    return;
  }
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
  m_window = type;
  if (m_FFTLEN > FFT_CMSIS_MAX) {
    /* S.Sint is the complex FFT of 2048 */
    arm_rfft_4096_fast_init_f32(&S);
    return (m_FFTLEN == 2*FFT_CMSIS_MAX || m_FFTLEN == 4*FFT_CMSIS_MAX);
  }

  clear();
  create_coef(type);
  if (!fft_init()) {
     return false;
  }

  return true;
}

void FFTClassKai::clear(){
  if (tmpOutBuf == NULL) return;
  memset(tmpOutBuf, 0, sizeof(float)*m_FFTLEN);
}

void FFTClassKai::end(){}

void FFTClassKai::create_coef(windowType_t type){
  for (int i = 0; i < m_FFTLEN/2; i++){
    if (type == WindowHamming) {
      coef[i] = 0.54f - (0.46f * arm_cos_f32(2 * PI * (float)i / (m_FFTLEN - 1)));
    } else if(type == WindowHanning) {
      coef[i] = 0.5f - (0.5f * arm_cos_f32(2 * PI * (float)i / (m_FFTLEN - 1)));
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
  arm_mult_f32(pSrc, coef, pSrc, m_FFTLEN); /* apply the window function */
  arm_rfft_fast_f32(&S, pSrc, pDst, 0);
}

void FFTClassKai::fft_amp(float* pDst, float* pSrc){
  /* calculation */
  arm_mult_f32(pSrc, coef, pSrc, m_FFTLEN); /* apply the window function */
  arm_rfft_fast_f32(&S, pSrc, tmpOutBuf, 0);
  arm_cmplx_mag_f32(&tmpOutBuf[2], &pDst[1], m_FFTLEN/2 - 1);
  pDst[0] = tmpOutBuf[0];
  pDst[m_FFTLEN/2] = tmpOutBuf[1];
}

void FFTClassKai::fft_scaled_amp(float* pDst, float* pSrc) {
  if (m_FFTLEN > FFT_CMSIS_MAX) {
    long_scaled_amp(pDst, pSrc);
    return;
  }
  float signal_voltage, maxValue;
  int index;
  /* which is better, RMS or MAX? */
  arm_rms_f32(pSrc, m_FFTLEN, &signal_voltage); 
  // arm_max_f32(pSrc, m_FFTLEN, &signal_voltage, &index); 
  arm_mult_f32(pSrc, coef, pSrc, m_FFTLEN); /* apply the window function after taking the RMS */
  arm_rfft_fast_f32(&S, pSrc, tmpOutBuf, 0);
  /* 
   * see the below site to get the reason for the strange arguments for arm_cmplx_mag_f32 
   * https://stackoverflow.com/questions/42299932/dsp-libraries-rfft-strange-results
   */  
  arm_cmplx_mag_f32(&tmpOutBuf[2], &pDst[1], m_FFTLEN/2 - 1);
  pDst[0] = tmpOutBuf[0];
  pDst[m_FFTLEN/2] = tmpOutBuf[1];
  arm_max_f32(pDst, m_FFTLEN/2, &maxValue, &index);
  float scale = signal_voltage/maxValue;
  arm_scale_f32(pDst, scale, tmpOutBuf, m_FFTLEN/2);
  memcpy(pDst, tmpOutBuf, m_FFTLEN*sizeof(float)/2);
}


/*------------------------------------------------------------------*/
/* The long FFT (8192, 16384) for the WAV file                      */
/*------------------------------------------------------------------*/

/* the bit reversed number of q (bits: the number of the bits) */
static inline int bitrev(int q, int bits) {
  int r = 0;
  for (int i = 0; i < bits; ++i) r = (r << 1) | ((q >> i) & 1);
  return r;
}

/* the window coefficient of the sample i: the same as create_coef() */
float FFTClassKai::window(int i) {
  float t = 2 * PI * (float)i / (m_FFTLEN - 1);
  switch (m_window) {
  case WindowHamming:
    return 0.54f - 0.46f * cosf(t);
  case WindowHanning:
    return 0.5f - 0.5f * cosf(t);
  case WindowFlattop:
    return 0.21557895f - 0.41663158f * cosf(t) + 0.277263158f * cosf(2 * t)
         - 0.083578947f * cosf(3 * t) + 0.006947368f * cosf(4 * t);
  default:
    return 1.0f;
  }
}

/*
 * The same result as fft_scaled_amp() for N = 8192 and 16384.
 * The N real samples are M = N/2 complex numbers z[n] = x[2n] + j x[2n+1].
 * The complex FFT of M is made of the stages of the decimation in frequency down to
 * the blocks of B = 2048 and the complex FFT of CMSIS on each block: Z[m] is in the block
 * of the bit reversed (m mod 2^S) at m >> S (S: the number of the stages).
 * The spectrum of the real samples is made from Z[k] and Z[M-k].
 * The rotation factors are made by the recurrence and calculated again every 32 steps.
 */
void FFTClassKai::long_scaled_amp(float* pDst, float* pSrc) {
  const int N = m_FFTLEN;
  const int M = N / 2;
  const int B = S.Sint.fftLen;
  float signal_voltage, maxValue;
  uint32_t index;

  arm_rms_f32(pSrc, N, &signal_voltage);
  for (int i = 0; i < N / 2; ++i) {
    float w = window(i);
    pSrc[i] *= w;
    pSrc[N - 1 - i] *= w;
  }

  float* z = pSrc;
  int stages = 0;
  for (int L = M; L > B; L >>= 1) {
    const int half = L / 2;
    const float sr = cosf(-2 * PI / L), si = sinf(-2 * PI / L);
    for (int base = 0; base < M; base += L) {
      float wr = 1.0f, wi = 0.0f;
      for (int k = 0; k < half; ++k) {
        if ((k & 31) == 0) {
          float t = -2 * PI * k / L;
          wr = cosf(t);
          wi = sinf(t);
        }
        float* a = &z[2 * (base + k)];
        float* b = &z[2 * (base + k + half)];
        float dr = a[0] - b[0], di = a[1] - b[1];
        a[0] += b[0];
        a[1] += b[1];
        b[0] = dr * wr - di * wi;
        b[1] = dr * wi + di * wr;
        float nr = wr * sr - wi * si;
        wi = wr * si + wi * sr;
        wr = nr;
      }
    }
    ++stages;
  }
  for (int blk = 0; blk < (1 << stages); ++blk) {
    arm_cfft_f32(&S.Sint, z + 2 * B * blk, 0, 1);
  }

  /* the spectrum of the real samples */
  const int qmask = (1 << stages) - 1;
  const float sr = cosf(-2 * PI / N), si = sinf(-2 * PI / N);
  float wr = 1.0f, wi = 0.0f;
  for (int k = 1; k < M; ++k) {
    if (((k - 1) & 31) == 0) {
      float t = -2 * PI * k / N;
      wr = cosf(t);
      wi = sinf(t);
    }
    const float* zk = &z[2 * (bitrev(k & qmask, stages) * B + (k >> stages))];
    const float* zm = &z[2 * (bitrev((M - k) & qmask, stages) * B + ((M - k) >> stages))];
    float er = 0.5f * (zk[0] + zm[0]), ei = 0.5f * (zk[1] - zm[1]);
    float orr = 0.5f * (zk[0] - zm[0]), oi = 0.5f * (zk[1] + zm[1]);
    float xr = er + (wr * oi + wi * orr);
    float xi = ei - (wr * orr - wi * oi);
    pDst[k] = sqrtf(xr * xr + xi * xi);
    float nr = wr * sr - wi * si;
    wi = wr * si + wi * sr;
    wr = nr;
  }
  pDst[0] = z[0] + z[1];  /* Z[0] is at the top of the first block */
  pDst[M] = z[0] - z[1];

  arm_max_f32(pDst, M, &maxValue, &index);
  arm_scale_f32(pDst, signal_voltage / maxValue, pDst, M);
}
