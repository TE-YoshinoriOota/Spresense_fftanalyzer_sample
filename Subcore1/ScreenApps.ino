#include "AppScreen.h"
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <cmsis/arm_math.h>

/* application for sensor values to the sensor monitor screen */
void appSensorValue(float acc, float vel, float dis) {
  tft.fillRect(MON_ELEM_SIDE, MON_ELEM0_HEAD, MON_BOX_WIDTH-MON_MARGIN*2, MON_BOX_HEIGHT-MON_MARGIN*2+1, ILI9341_BLACK);
  tft.fillRect(MON_ELEM_SIDE, MON_ELEM1_HEAD, MON_BOX_WIDTH-MON_MARGIN*2, MON_BOX_HEIGHT-MON_MARGIN*2+1, ILI9341_BLACK);
  tft.fillRect(MON_ELEM_SIDE, MON_ELEM2_HEAD, MON_BOX_WIDTH-MON_MARGIN*2, MON_BOX_HEIGHT-MON_MARGIN*2+1, ILI9341_BLACK);
  char dataText[10];
  memset(dataText, 0, sizeof(char)*10);
  sprintf(dataText, "%+03.2f", acc);
  putText(MON_ELEM_SIDE, MON_ELEM0_HEAD, dataText, ILI9341_WHITE, 3);
  memset(dataText, 0, sizeof(char)*10);
  sprintf(dataText, "%+03.2f", vel);
  putText(MON_ELEM_SIDE, MON_ELEM1_HEAD, dataText, ILI9341_WHITE, 3);
  memset(dataText, 0, sizeof(char)*10);
  sprintf(dataText, "%+03.2f", dis);
  putText(MON_ELEM_SIDE, MON_ELEM2_HEAD, dataText, ILI9341_WHITE, 3);
}


/* application of drawing WAV-FFT on each graph */
void appDraw2WayGraph(float* pWav, int len0, float *pFft, int len1, float df) {
  int i,j;
  bool scale_update = false;
  static int last_w_amp = -1;
  static int last_f_amp = -1;
  static int last_f_dbv = -1;
    
  pthread_mutex_lock(&mtx);
  int w_amp = wavamp0;
  int f_amp = fftamp1;
  int f_dbv = dbvdisp1;
  pthread_mutex_unlock(&mtx);

#ifdef SCR_DEBUG
  MPLog("len0: %d  len1: %d  df: %1.4f\n", len0, len1, df);  
#endif

  /*********** draw upper graph ***********/
  // I should kick out this routine before this loop
  int gskip, dskip;
  get_scale_param_for_wav(&gskip, &dskip, len0); 

  if (w_amp != last_w_amp) {
    last_w_amp = w_amp;
    scale_update = true;
  }
  
  drawWavGraph(pWav, len0, df, w_amp, gskip, dskip, FFT_GRAPH0_HEAD, scale_update);
  scale_update = false;


  /*********** draw lower graph ***********/
  // I should kick out this routine from this loop
  int flen = get_scale_param_for_freq(&fmaxdisp, &gskip, &dskip, len1, df);
  
  
  if (bLogDisplay == true && bdBVDisplay == false) {
    /* preparation for log graph */
    int interval;
    float f_min_log;
    get_log_scale_param(&interval, &f_min_log, df);

    /* draw vertical scale */
    if (f_amp != last_f_amp) {
      scale_update = true;
      last_f_amp = f_amp;
    }

    drawLogFftGraph(pFft, flen, df, f_amp, gskip, dskip , interval, f_min_log
                  , FFT_GRAPH1_HEAD, scale_update);
    scale_update = false;

  } else if (bLogDisplay == true&& bdBVDisplay == true) { 
    /* preparation for log graph */
    // I should kick out this routine before this loop
    int interval;
    float f_min_log;
    get_log_scale_param(&interval, &f_min_log, df);

    if (f_dbv != last_f_dbv) {
      last_f_dbv = f_dbv;
      scale_update = true;
    }

    drawDbvFftGraph(pFft, flen, df, f_dbv, gskip, dskip, interval, f_min_log
                  , FFT_GRAPH1_HEAD, scale_update);
    scale_update = false;
   
  } else {
    /* draw vertical scale */
    if (f_amp != last_f_amp) {
      last_f_amp = f_amp;
      scale_update = true;
    }
    drawLinearFftGraph(pFft, flen, df, f_amp, gskip, dskip, FFT_GRAPH1_HEAD, scale_update);
    scale_update = false;
  }
}


/* application of drawing FFT-FFT on each graph */
void appDraw2FftGraph(float* pFft, float* pSubFft, int len, float df) {
  int i, j;
  float peakFs, maxValue;
  static int last_f_amp0 = -1;
  static int last_f_amp1 = -1;
  static int last_f_dbv0 = -1;
  static int last_f_dbv1 = -1;
  bool scale_update = false;
  
  pthread_mutex_lock(&mtx);
  int f_amp0 = fftamp0;
  int f_amp1 = fftamp1;
  int f_dbv0 = dbvdisp0;
  int f_dbv1 = dbvdisp1;
  pthread_mutex_unlock(&mtx);

  /*********** draw upper graph ***********/
  /** check range of fmaxdisp **/
  int gskip, dskip;
  int flen = get_scale_param_for_freq(&fmaxdisp, &gskip, &dskip, len, df);

  /************ draw upper graph ************/
  if (bLogDisplay == true && bdBVDisplay == false) {

    /* preparation for log graph */
    int interval;
    float f_min_log;
    get_log_scale_param(&interval, &f_min_log, df);

    if (f_amp0 != last_f_amp0) {
      last_f_amp0 = f_amp0;
      scale_update = true;
    }

    drawLogFftGraph(pFft, flen, df, f_amp0, gskip, dskip , interval, f_min_log
                  , FFT_GRAPH0_HEAD, scale_update, ILI9341_CYAN);
    scale_update = false;

  } else if (bLogDisplay == true && bdBVDisplay == true) {

    /* preparation for log graph */
    // I should kick out this routine before this loop
    int interval;
    float f_min_log;
    get_log_scale_param(&interval, &f_min_log, df);

    if (f_dbv0 != last_f_dbv0) {
      last_f_dbv0 = f_dbv0;
      scale_update = true;
    }

    drawDbvFftGraph(pFft, flen, df, f_dbv0, gskip, dskip, interval, f_min_log
                  , FFT_GRAPH0_HEAD, scale_update, ILI9341_CYAN);
    scale_update = false;

  } else {

    /* draw vertical scale */
    if (f_amp0 != last_f_amp0) {
      last_f_amp0 = f_amp0;
      scale_update = true;
    }
    drawLinearFftGraph(pFft, flen, df, f_amp0, gskip, dskip
                     , FFT_GRAPH0_HEAD, scale_update, ILI9341_CYAN);
    scale_update = false;

  }


  /************ draw lower graph ************/
  if (bLogDisplay == true && bdBVDisplay == false) {

    /* preparation for log graph */
    int interval;
    float f_min_log;
    get_log_scale_param(&interval, &f_min_log, df);

    if (f_amp1 != last_f_amp1) {
      last_f_amp1 = f_amp1;
      scale_update = true;
    }

    drawLogFftGraph(pSubFft, flen, df, f_amp1, gskip, dskip , interval, f_min_log
                  , FFT_GRAPH1_HEAD, scale_update, ILI9341_GREEN
                  , true, false, false);

    /* make fft diff data */
    for (int i = 0; i < flen; ++i) {
      pSubFft[i] = abs(pFft[i] - pSubFft[i]);
    }

    drawLogFftGraph(pSubFft, flen, df, f_amp1, gskip, dskip , interval, f_min_log
                  , FFT_GRAPH1_HEAD, scale_update, ILI9341_RED
                  , false, false, true);

    scale_update = false;

  } else if (bLogDisplay == true && bdBVDisplay == true) {

    /* preparation for log graph */
    // I should kick out this routine before this loop
    int interval;
    float f_min_log;
    get_log_scale_param(&interval, &f_min_log, df);

    if (f_dbv1 != last_f_dbv1) {
      last_f_dbv1 = f_dbv1;
      scale_update = true;
    }

    drawDbvFftGraph(pSubFft, flen, df, f_dbv1, gskip, dskip, interval, f_min_log
                  , FFT_GRAPH1_HEAD, scale_update, ILI9341_GREEN
                  , true, false, false);

    /* make fft diff data */
    for (int i = 0; i < flen; ++i) {
      pSubFft[i] = abs(pFft[i] - pSubFft[i]);
    }

    drawDbvFftGraph(pSubFft, flen, df, f_dbv1, gskip, dskip , interval, f_min_log
                  , FFT_GRAPH1_HEAD, scale_update, ILI9341_RED
                  , false, false, true);

    scale_update = false;

  } else {

    /* draw vertical scale */
    if (f_amp1 != last_f_amp1) {
      last_f_amp1 = f_amp1;
      scale_update = true;
    }

    drawLinearFftGraph(pSubFft, flen, df, f_amp1, gskip, dskip
                     , FFT_GRAPH1_HEAD, scale_update, ILI9341_GREEN
                     , true, false, false);

    /* make fft diff data */
    for (int i = 0; i < flen; ++i) {
      pSubFft[i] = abs(pFft[i] - pSubFft[i]);
    }

    drawLinearFftGraph(pSubFft, flen, df, f_amp1, gskip, dskip
                     , FFT_GRAPH1_HEAD, scale_update, ILI9341_RED
                     , false, false, true);
    scale_update = false;
  }
}


/* application of drawing Orbit trace on the graph */
void appDrawOrbitGraph(struct OrbitData* odata) {

  int val0, val1;
  static int last_o_amp = 0;

  pthread_mutex_lock(&mtx);
  int o_amp = orbitamp;
  pthread_mutex_unlock(&mtx);

  if (o_amp != last_o_amp) {
    // The magnification of the screen is changed. clear the screen.
    memset(orbitBuf, 0, sizeof(uint16_t)*ORBIT_SIZE*ORBIT_SIZE);
  }
  
#ifdef SCR_DEBUG
  MPLog("Plot displacement data to buffer\n");
#endif
  /* no need to care about graph direction with this orbit case */
  val0 = (int)(o_amp*odata->dis0);
  val1 = (int)(o_amp*odata->dis1);

  /* default: 1024um */
  val0 /= (ORBIT_LIMIT_UM/ORBIT_GRAPH_RADIUS);
  val1 /= (ORBIT_LIMIT_UM/ORBIT_GRAPH_RADIUS);
    
  if (val0 >= ORBIT_GRAPH_RADIUS)       val0 = ORBIT_GRAPH_RADIUS-1;
  else if (val0 <= -ORBIT_GRAPH_RADIUS) val0 = -(ORBIT_GRAPH_RADIUS-1);

  if (val1 >= ORBIT_GRAPH_RADIUS)       val1 = ORBIT_GRAPH_RADIUS-1;
  else if (val1 <= -ORBIT_GRAPH_RADIUS) val1 = -(ORBIT_GRAPH_RADIUS-1);

  val0 += ORBIT_GRAPH_RADIUS;
  val1 += ORBIT_GRAPH_RADIUS;

  //orbitBuf[val0][val1] = ILI9341_CYAN;
  *(orbitBuf+val0*ORBIT_SIZE+val1) = ILI9341_CYAN;
  
#ifdef SCR_DEBUG
  MPLog("Write OrBitGraph\n");
#endif
  writeOrBitGraphToBuf(orbitBuf, ORBIT_SIZE/2, ORBIT_SIZE/2
                     , ORBIT_GRAPH_RADIUS, ILI9341_YELLOW);
                     
  int x = ORBIT_GRAPH_XCENTER-ORBIT_SIZE/2;
  int y = ORBIT_GRAPH_YCENTER-ORBIT_SIZE/2;
  tft.drawRGBBitmap(x, y, (uint16_t*)orbitBuf, ORBIT_SIZE, ORBIT_SIZE);

  if (last_o_amp != o_amp) plotorbitscale(o_amp);
  
  last_o_amp = o_amp;
}


/* application of drawind Raw WAV and Filtered WAV on each graph */
void appDraw2WavGraph(float* pWav, float *pSubWav, int len, float df) {
  int i,j;
  static int last_w_amp0 = -1;
  static int last_w_amp1 = -1;
  bool scale_update = false;

  pthread_mutex_lock(&mtx);
  int w_amp0 = wavamp0;
  int w_amp1 = wavamp1;
  pthread_mutex_unlock(&mtx);

  /* draw upper graph */
#ifdef SCR_DEBUG
  MPLog("len: %d  df: %1.4f\n", len, df); 
#endif

  /*********** draw upper graph ***********/
  // I should kick out this routine before this loop
  int gskip, dskip;
  get_scale_param_for_wav(&gskip, &dskip, len); 

  if (w_amp0 != last_w_amp0) {
    last_w_amp0 = w_amp0;
    scale_update = true;
  }
  
  drawWavGraph(pWav, len, df, w_amp0, gskip, dskip, FFT_GRAPH0_HEAD, scale_update);
  scale_update = false;

  /*********** draw lower graph ***********/
  if (w_amp1 != last_w_amp1) {
    last_w_amp1 = w_amp1;
    scale_update = true;
  }
  
  drawWavGraph(pSubWav, len, df, w_amp1, gskip, dskip, FFT_GRAPH1_HEAD, scale_update);
  scale_update = false;

}

float get_peak_to_peak(float* pData, int len, float* minValue, float* maxValue) {
  uint32_t index;

  arm_max_f32(pData, len, maxValue, &index);
  arm_min_f32(pData, len, minValue, &index);

  return maxValue - minValue;
}

float get_peak_frequency(float* pData, int len, float delta_f, float* maxValue) {
  uint32_t index;
  float delta, delta_spr;
  float peakFs;

  arm_max_f32(pData, len, maxValue, &index);

  delta = 0.5*(pData[index-1] - pData[index+1])
    / (pData[index-1] + pData[index+1] - (2.0f * pData[index]));
  peakFs = (index + delta) * delta_f;
  /*
  delta_spr = 0.125*(pData[index-1] - pData[index+1])*(pData[index-1] - pData[index+1])
    / (2.0f * pData[index] - (pData[index-1] + pData[index+1]));
  *maxValue = delta_spr;
  */
  return peakFs;
}

void get_scale_param_for_wav(int* gskip, int* dskip, int len) {
  if (len < FFT_GRAPH_WIDTH) {
    *gskip = FFT_GRAPH_WIDTH / len; if (*gskip == 0) *gskip = 1;
    *dskip = 1;
  } else {
    *gskip = 1;
    *dskip = len / FFT_GRAPH_WIDTH; if (*dskip == 0) *dskip = 1;
  }
#ifdef SCR_DEBUG
  MPLog("len0: %d  gskip: %d  dskip: %d\n", *len, *gskip, *dskip);
#endif
}


int get_scale_param_for_freq(int* fmaxdisp, int* gskip, int* dskip, int len, float df) {
  /* check range of fmaxdisp */
  float available_max_freq = df*(len-1);  /* = sampling_rate/2 */
  if (*fmaxdisp > available_max_freq) *fmaxdisp = available_max_freq; 

  /* calclate the display range */
  int flen;
  int target_fmax = *fmaxdisp;
  int gskip_, dskip_;
  do {
    flen = (int)(target_fmax/df);
    if (flen < FFT_GRAPH_WIDTH) {
      gskip_ = round((float)(FFT_GRAPH_WIDTH) / flen); if (gskip_ == 0) gskip_ = 1;
      dskip_ = 1;
    } else {
      gskip_ = 1;
      dskip_ = round((float)(flen)/FFT_GRAPH_WIDTH); if (dskip_ == 0) dskip_ = 1;
    }
    target_fmax += 1000;
  } while (*fmaxdisp > df*FFT_GRAPH_WIDTH*dskip_/gskip_);
  if (flen > len) flen = len; /* fail safe */
#ifdef SCR_DEBUG
  MPLog("flen: %d  gskip: %d  dskip: %d\n", flen, gskip, dskip);
#endif
  *gskip = gskip_;
  *dskip = dskip_;
  return flen;
}

void get_log_scale_param(int* interval, float* f_min_log, float df) {
  float log_f_unit;
  int interval_; 
  if (fmaxdisp < 4000)       log_f_unit = 2;
  else if (fmaxdisp < 8000)  log_f_unit = 3;
  else if (fmaxdisp < 24000) log_f_unit = 3;
  else if (fmaxdisp < 96000) log_f_unit = 4;
  if (log_f_unit > 1.0) interval_ = (FRAME_HEIGHT-1)/(int16_t)(log_f_unit);
  else interval_ = FRAME_HEIGHT-1;
  *f_min_log = (float)(log10(df)*interval_);
  *interval = interval_; 
  return;
}

void drawWavGraph(float* pWav, int len, float df, int amp, int gskip, int dskip, int head, bool scale_update, int color) {
  int i, j;
  float maxValue, minValue;

  /* draw upper vertical scale */
  if (scale_update) plotvirticalscale(head, amp, true);

  /* draw max voltage and min voltage */
  get_peak_to_peak(pWav, len, &minValue, &maxValue);
  maxValue *= WAV_MAX_VOL; /* mV */
  minValue *= WAV_MAX_VOL; /* mV */
  putMaxMinVoltage(maxValue, minValue, head);

  /* draw upper horizontal scale */
  plottimescale(df, len, head, false);

  /* copy and scale the data to display */
  memset(graphDataBuf, 0, sizeof(float)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len && j < FFT_GRAPH_WIDTH; i += dskip, ++j) {
    graphDataBuf[j] = (float)(amp*pWav[i]*WAV_MAX_VOL);  /* mV */
  }
  
  /* draw upper graph */  
  putBufLinearGraph(frameBuf, graphDataBuf
                  , WAV_MAX_VOL, gskip
                  , FFT_GRAPH_SIDE, head
                  , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, color, df
                  , FFT_GRAPH_HEIGHT/2);
}


void drawLinearFftGraph(float* pFft, int len, float df, int amp, int gskip, int dskip
                      , int head, bool scale_update, int color, bool peakdisp, bool clear, bool display) {
  int i, j;
  float maxValue, minValue;

  /* draw vertical scale */
  if (scale_update) plotvirticalscale(head, amp, false);
  
  /* display peak frequency and its value */
  if (peakdisp == true) {
    float peakFs = get_peak_frequency(pFft, len, df, &maxValue);
    maxValue *= WAV_MAX_VOL; /* mV */
    putPeakFrequencyInLinear(peakFs, maxValue, head);
  }
  
  /* draw horizontal scale */   
  plotlinearscale(df, gskip, dskip, head, false);

  /* copy and scale the data to display */
  if (display == true)  memset(graphDataBuf, 0, sizeof(float)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len && j < FFT_GRAPH_WIDTH-1; i += dskip, ++j) {
    graphDataBuf[j] = (float)(amp*pFft[i]*WAV_MAX_VOL);
  }

  /* draw graph */
  putBufLinearGraph(frameBuf, graphDataBuf
                  , WAV_MAX_VOL, gskip
                  , FFT_GRAPH_SIDE, head
                  , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, color, df, 0
		              , clear, display);
}

void drawLogFftGraph(float* pFft, int len, float df, int amp, int gskip, int dskip
                   , int interval, float f_min_log
                   , int head, bool scale_update, int color, bool peakdisp, bool clear, bool display) {
  int i, j;
  float maxValue, minValue;

  /* draw vertical scale */
  if (scale_update) plotvirticalscale(head, amp, false);

  /* display peak frequency and its value */
  if (peakdisp == true) {
    float peakFs = get_peak_frequency(pFft, len, df, &maxValue);
    maxValue *= WAV_MAX_VOL; /* mV */
    putPeakFrequencyInLinear(peakFs, maxValue, head);
  }

  /* draw horizontal scale */
  plotlogscale(interval, df, f_min_log, FFT_GRAPH1_HEAD, false);
  
  /* copy and scale the data to display buf */
  memset(graphDataBuf, 0, sizeof(float)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len && j < FFT_GRAPH_WIDTH-1; i += dskip, ++j) {
    graphDataBuf[j] = (float)(amp*pFft[i]*WAV_MAX_VOL);
  }
  len = len - (len - j*dskip); // adjustment

  /* draw graph */
  putBufLogGraph(frameBuf, graphDataBuf
               , WAV_MAX_VOL, len, dskip
               , FFT_GRAPH_SIDE, head
               , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
               , color, df, interval, f_min_log
	             , clear, display);
}


void drawDbvFftGraph(float* pFft, int len, float df, int dbvdisp, int gskip, int dskip
                   , int interval, float f_min_log
                   , int head, bool scale_update, int color, bool peakdisp, bool clear, bool display) {
  int i ,j;
  float maxValue;

  /* draw vertical scale */
  int maxdbv = dbvrange[dbvdisp][0];
  int mindbv = dbvrange[dbvdisp][1];
  if (scale_update) {
    plotvirticalscale_dbv(head, maxdbv, mindbv);
    MPLog("%d - %d [dBV]\n", maxdbv, mindbv);
  }

  /* display peak frequency and its value */
  if (peakdisp == true) {
    float peakFs = get_peak_frequency(pFft, len, df, &maxValue);
    maxValue *= WAV_MAX_VOL; /* mV */
    putPeakFrequencyInLinear(peakFs, maxValue, head);
  }

  /* draw horizontal scale */   
  plotlogscale(interval, df, f_min_log, head, false);

  /* copy and scale the data to display */
  memset(graphDataBuf, 0, sizeof(float)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len && j < FFT_GRAPH_WIDTH-1; i += dskip, ++j) {
    graphDataBuf[j] = (float)(20*log10(pFft[i]*WAV_MAX_VOL/1000));
  }
  len = len - (len - j*dskip); // adjustment
    
  /* draw graph */
  putBufdBVGraph(frameBuf, graphDataBuf
               , maxdbv, mindbv, len-1, dskip
               , FFT_GRAPH_SIDE, head
               , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
               , color, df, interval, f_min_log
               , clear, display);
}
