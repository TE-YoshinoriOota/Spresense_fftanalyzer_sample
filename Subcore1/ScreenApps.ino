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
  static int last_f_dbv = -1;
  static int last_amp   = -1;
    
  pthread_mutex_lock(&mtx);
  int g_amp = amp;
  pthread_mutex_unlock(&mtx);

  if (g_amp != last_amp) {
    last_amp = g_amp;
    scale_update = true;
  }
  
#ifdef SCR_DEBUG
  MPLog("len0: %d  len1: %d  df: %1.4f\n", len0, len1, df);  
#endif

  /*********** draw upper graph ***********/
  // I should kick out this routine before this loop
  int gskip, dskip;
  get_scale_param_for_wav(&gskip, &dskip, len0); 
  drawWavGraph(pWav, len0, df, g_amp, gskip, dskip, FFT_GRAPH0_HEAD, scale_update);

  /*********** draw lower graph ***********/
  // I should kick out this routine from this loop
  int flen = get_scale_param_for_freq(&fmaxdisp, &gskip, &dskip, len1, df);
  
  if (bLogDisplay == true && bdBVDisplay == false) {
    /* preparation for log graph */
    int interval;
    float f_min_log;
    get_log_scale_param(&interval, &f_min_log, df);
    drawLogFftGraph(pFft, flen, df, g_amp, gskip, dskip 
                  , interval, f_min_log
                  , FFT_GRAPH1_HEAD, scale_update, ILI9341_MAGENTA);

  } else if (bLogDisplay == true&& bdBVDisplay == true) { 
    /* preparation for log graph */
    // I should kick out this routine before this loop
    int interval;
    float f_min_log;
    get_log_scale_param(&interval, &f_min_log, df);
    /* the top is the full scale of the waveform in dBV (rounded up to 10 dB): */
    /* a sine filling the waveform graph has its peak (RMS) 3 dB below the top */
    float full_scale = WAV_MAX_VOL/g_amp/1000.0f;  /* V */
    int maxdbv = (int)ceilf(20*log10f(full_scale)/DBV_TOP_STEP)*DBV_TOP_STEP;
    drawDbvFftGraph(pFft, flen, df, gskip, dskip
                  , maxdbv, maxdbv - DBV_RANGE
                  , interval, f_min_log
                  , FFT_GRAPH1_HEAD, scale_update, ILI9341_MAGENTA);
  
  } else {

    drawLinearFftGraph(pFft, flen, df, g_amp, gskip, dskip, FFT_GRAPH1_HEAD, scale_update, ILI9341_MAGENTA);

  }
  scale_update = false;
}


/* application of drawing FFT-FFT on each graph */
void appDraw2FftGraph(float* pFft, float* pSubFft, int len, float df) {
  int i, j;
  static int last_amp    = -1;
  static int last_offset = -1;
  
  pthread_mutex_lock(&mtx);
  int g_amp     = amp;
  int d_offset  = dbvoffset;
  pthread_mutex_unlock(&mtx);
  
  
  if (g_amp != last_amp || d_offset != last_offset) {
    MPLog("Param Changed\n");
    last_amp    = g_amp;
    last_offset = d_offset;
    scale_update = true;
  }
  
  int maxdbv = d_offset;
  int mindbv = d_offset-DBV_RANGE;

  /*********** draw upper graph: CH1 and CH2 on the same graph ***********/
  /** check range of fmaxdisp **/
  int gskip, dskip;
  int flen = get_scale_param_for_freq(&fmaxdisp, &gskip, &dskip, len, df);
  int interval = 0;
  float f_min_log = 0;
  if (bLogDisplay == true) get_log_scale_param(&interval, &f_min_log, df);

  if (bLogDisplay == true && bdBVDisplay == false) {
    drawLogFftGraph(pFft, flen, df, g_amp, gskip, dskip, interval, f_min_log
                  , FFT_GRAPH0_HEAD, scale_update, ILI9341_CYAN, true, true, bMonoFile);
    if (!bMonoFile)
    drawLogFftGraph(pSubFft, flen, df, g_amp, gskip, dskip, interval, f_min_log
                  , FFT_GRAPH0_HEAD, false, ILI9341_GREEN, false, false, true);

  } else if (bLogDisplay == true && bdBVDisplay == true) {
    drawDbvFftGraph(pFft, flen, df, gskip, dskip, maxdbv, mindbv, interval, f_min_log
                  , FFT_GRAPH0_HEAD, scale_update, ILI9341_CYAN, true, true, bMonoFile);
    if (!bMonoFile)
    drawDbvFftGraph(pSubFft, flen, df, gskip, dskip, maxdbv, mindbv, interval, f_min_log
                  , FFT_GRAPH0_HEAD, false, ILI9341_GREEN, false, false, true);

  } else {
    drawLinearFftGraph(pFft, flen, df, g_amp, gskip, dskip
                     , FFT_GRAPH0_HEAD, scale_update, ILI9341_CYAN, true, true, bMonoFile);
    if (!bMonoFile)
    drawLinearFftGraph(pSubFft, flen, df, g_amp, gskip, dskip
                     , FFT_GRAPH0_HEAD, false, ILI9341_GREEN, false, false, true);
  }

  /************ draw lower graph: the difference CH1 - CH2 ************/
  if (!bMonoFile)
  drawDiffFftGraph(pFft, pSubFft, flen, df, g_amp, gskip, dskip, interval, f_min_log
                 , FFT_GRAPH1_HEAD, scale_update);
  scale_update = false;
}




/* application of drawind Raw WAV and Filtered WAV on each graph */
void appDraw2WavGraph(float* pWav, float *pSubWav, int len, float df) {
  int i,j;

  static int last_w_amp0 = -1;
  static int last_w_amp1 = -1;
  static int last_amp = -1;

  if (frameBuf == NULL) return;  /* the screen has no graph buffer (e.g. LISSAJOUS) */

  pthread_mutex_lock(&mtx);
  int g_amp  = amp;
  pthread_mutex_unlock(&mtx);

  /* draw upper graph */
#ifdef SCR_DEBUG
  MPLog("len: %d  df: %1.4f\n", len, df); 
#endif

  /*********** draw upper graph ***********/
  // I should kick out this routine before this loop
  int gskip, dskip;
  get_scale_param_for_wav(&gskip, &dskip, len); 

  if (g_amp != last_amp) {
    last_amp = g_amp;
    scale_update = true;
  }
  
  drawWavGraph(pWav, len, df, g_amp, gskip, dskip, FFT_GRAPH0_HEAD, scale_update);

  /*********** draw lower graph ***********/
  if (g_amp != last_amp) {
    last_amp = g_amp;
    scale_update = true;
  }
  
  /* CH2 is green (D-FILTER MONITOR: the filtered CH1 is blue as CH1) */
  if (!bMonoFile) drawWavGraph(pSubWav, len, df, g_amp, gskip, dskip, FFT_GRAPH1_HEAD, scale_update
                             , (scrType == SCR_TYPE_FLTR) ? ILI9341_CYAN : ILI9341_GREEN);
  scale_update = false;

}

/* application of drawing the recording status on the REC MONITOR screen */
/*   upper line: elapsed time (red: recording, yellow with "!": frames dropped) */
/*   lower line: channels and the file name, or the error message             */
/* force: redraw everything after REC/STOP is toggled or Maincore replied     */
void appRecStatus(bool force) {
  static uint32_t last_sec = 0;
  static bool     last_drop = false;
  static int      last_state = REC_STATE_IDLE;
  uint32_t sec  = 0;
  bool     drop = false;
  int      state = REC_STATE_IDLE;
  if (recStat != NULL) {
    if (recStat->rate > 0) sec = recStat->frames / recStat->rate;
    drop = (recStat->dropped > 0);
    state = recStat->state;
  }
  /* Maincore changes the state while recording (e.g. AUDIO ERROR), so redraw all */
  if (state != last_state) force = true;
  if (!force && sec == last_sec && drop == last_drop) return;
  last_sec  = sec;
  last_drop = drop;
  last_state = state;

  char text[24];
  int color = bRecording ? ILI9341_RED : ILI9341_WHITE;
  if (drop) color = ILI9341_YELLOW;
  sprintf(text, "%02d:%02d:%02d%s", (int)(sec/3600), (int)((sec/60)%60), (int)(sec%60), drop ? "!" : "");
  tft.fillRect(REC_STAT_SIDE, REC_STAT0_HEAD, REC_STAT_WIDTH, 16, ILI9341_BLACK);
  putText(REC_STAT_SIDE, REC_STAT0_HEAD, text, color, 2);

  if (!force) return;

  tft.fillRect(REC_STAT_SIDE, REC_STAT1_HEAD, REC_STAT_WIDTH, 8, ILI9341_BLACK);
  if (recStat != NULL && recStat->state == REC_STATE_ERROR) {
    putText(REC_STAT_SIDE, REC_STAT1_HEAD, recStat->message, ILI9341_RED, 1);
  } else {
    if (recStat != NULL) sprintf(text, "%s %s", recChText, recStat->filename);
    else                 sprintf(text, "%s", recChText);
    putText(REC_STAT_SIDE, REC_STAT1_HEAD, text, ILI9341_WHITE, 1);
  }

  putButtonLabel(0, bRecording ? "STOP" : "REC");
}

/*****************************************************************/
/* application of drawind Raw WAV and Filtered WAV on each graph */
/*****************************************************************/
void appDrawSpectroGraph(float* pFft, int len, float df) {
  int i,j;
  static int last_amp = -1;

#ifdef SCR_DEBUG
  MPLog("len: %d  df: %1.4f\n", len, df); 
#endif

  pthread_mutex_lock(&mtx);
  int g_amp = spcamp;
  pthread_mutex_unlock(&mtx);

  if (g_amp != last_amp) {
    last_amp = g_amp;
    scale_update = true;
  }

  /*********** draw upper graph ***********/
  // I should kick out this routine from this loop
  int gskip, dskip;
  int flen = get_scale_param_for_spectro(&gskip, &dskip, &fmaxdisp, len, df); 

#ifdef SCR_DEBUG
  MPLog("flen: %d gskip: %d dskip: %d fmaxdisp %d\n", flen, gskip, dskip, fmaxdisp);
#endif

  // draw Spectrogram
  drawSpectroGraph(pFft, flen, df, gskip, dskip, g_amp, scale_update);
}



/***************** draw graph functions ************************/

void drawWavGraph(float* pWav, int len, float df, int amp, int gskip, int dskip
                , int head, bool scale_update, int color) {
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
  if (scale_update) plottimescale(df, len, head);

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


/* draw the signed difference of two spectra (CH1 - CH2) with 0 at the center */
/* the difference is calculated in this buffer: pFft and pSubFft belong to Maincore */
/* and are saved as CSV by the screen capture, so they must not be overwritten */

void drawDiffFftGraph(float* pFft, float* pSubFft, int len, float df, int amp, int gskip, int dskip
                    , int interval, float f_min_log, int head, bool scale_update) {
  if (diffFftBuf == NULL) return;
  int i, j;
  if (len > MAX_FFT_BINS) len = MAX_FFT_BINS;

  if (bLogDisplay == true && bdBVDisplay == true) {
    /* the difference in dB: 20log10(CH1/CH2) */
    if (scale_update) {
      plotvirticalscale_dbv(head, DIFF_DB_RANGE, -DIFF_DB_RANGE);
      plotlogscale(interval, df, f_min_log, head);
    }
    for (i = 0; i <= len; ++i) {
      diffFftBuf[i] = 20*log10f_fast((pFft[i] + DIFF_DB_FLOOR)/(pSubFft[i] + DIFF_DB_FLOOR));
    }
    putBufLogGraph(frameBuf, diffFftBuf, 1.0, DIFF_DB_RANGE, len, 1
                 , FFT_GRAPH_SIDE, head, FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
                 , ILI9341_RED, df, interval, f_min_log, true, true, FRAME_WIDTH/2);

  } else if (bLogDisplay == true) {
    if (scale_update) {
      plotvirticalscale(head, amp, true);
      plotlogscale(interval, df, f_min_log, head);
    }
    for (i = 0; i <= len; ++i) diffFftBuf[i] = pFft[i] - pSubFft[i];
    putBufLogGraph(frameBuf, diffFftBuf, (float)amp*WAV_MAX_VOL, WAV_MAX_VOL, len, 1
                 , FFT_GRAPH_SIDE, head, FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
                 , ILI9341_RED, df, interval, f_min_log, true, true, FRAME_WIDTH/2);

  } else {
    if (scale_update) {
      plotvirticalscale(head, amp, true);
      plotlinearscale(df, gskip, dskip, head);
    }
    memset(graphDataBuf, 0, sizeof(float)*FFT_GRAPH_WIDTH);
    for (i = 0, j = 0; i < len && j < FFT_GRAPH_WIDTH-1; i += dskip, ++j) {
      graphDataBuf[j] = (float)(amp*(pFft[i] - pSubFft[i])*WAV_MAX_VOL);
    }
    putBufLinearGraph(frameBuf, graphDataBuf
                    , WAV_MAX_VOL, gskip
                    , FFT_GRAPH_SIDE, head
                    , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, ILI9341_RED, df, FFT_GRAPH_HEIGHT/2
                    , true, true);
  }
}


void drawLinearFftGraph(float* pFft, int len, float df, int amp, int gskip, int dskip
                      , int head, bool scale_update, int color, bool peakdisp, bool clr, bool disp) {
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
  if (scale_update) plotlinearscale(df, gskip, dskip, head);

  /* copy and scale the data to display */
  if (disp == true)  memset(graphDataBuf, 0, sizeof(float)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len && j < FFT_GRAPH_WIDTH-1; i += dskip, ++j) {
    graphDataBuf[j] = (float)(amp*pFft[i]*WAV_MAX_VOL);
  }

  /* draw graph */
  putBufLinearGraph(frameBuf, graphDataBuf
                  , WAV_MAX_VOL, gskip
                  , FFT_GRAPH_SIDE, head
                  , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, color, df, 0
		              , clr, disp);
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
  if (scale_update) plotlogscale(interval, df, f_min_log, head);
  
  /* draw graph: every bin from df (bin 1) */
  putBufLogGraph(frameBuf, pFft, (float)amp*WAV_MAX_VOL
               , WAV_MAX_VOL, len, 1
               , FFT_GRAPH_SIDE, head
               , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
               , color, df, interval, f_min_log
	             , clear, display);
}


void drawDbvFftGraph(float* pFft, int len, float df, int gskip, int dskip
                   , int maxdbv, int mindbv
                   , int interval, float f_min_log
                   , int head, bool scale_update, int color, bool peakdisp, bool clr, bool disp) {
  int i ,j;
  float maxValue;

  /* draw vertical scale */
  if (scale_update) {
    plotvirticalscale_dbv(head, maxdbv, mindbv);
#ifdef SCR_DEBUG
    MPLog("%d - %d [dBV]\n", maxdbv, mindbv);
#endif
  }

  /* display peak frequency and its value */
  if (peakdisp == true) {
    float peakFs = get_peak_frequency(pFft, len, df, &maxValue);
    maxValue *= WAV_MAX_VOL; /* mV */
    putPeakFrequencyInLinear(peakFs, maxValue, head);
  }

  /* draw horizontal scale */ 
  if (scale_update) plotlogscale(interval, df, f_min_log, head);

  /* draw graph: every bin from df (bin 1) */
  putBufdBVGraph(frameBuf, pFft
               , maxdbv, mindbv, len-1, 1
               , FFT_GRAPH_SIDE, head
               , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
               , color, df, interval, f_min_log
               , clr, disp);
}


void drawSpectroGraph(float* pFft, int len, float df, int gskip, int dskip, int amp, bool scale_update) {
  int i ,j;
  const static int wait_for_stable = 5;
  static uint32_t last_time_mills = 0;
  static uint32_t duration = 0;
  float maxValue;
  
  /* draw horizontal scale */
  if (loop_counter < wait_for_stable) {
    uint32_t cur_time_mills = millis();
    duration = cur_time_mills - last_time_mills;
    last_time_mills = cur_time_mills;
    ++loop_counter;
  } else if (loop_counter == wait_for_stable) {
    MPLog("duration %d\n", duration);
    plottimescale_for_spc(duration);
    ++loop_counter;
  }

  /* draw vertical scale */
  if (scale_update) {
    plotverticalscale_spc(df, len, gskip, dskip);
    putColorRange(amp);
  }
  
  /* display peak frequency and its value */
  float peakFs = get_peak_frequency(pFft, len, df, &maxValue);
  maxValue *= WAV_MAX_VOL; // mV 
  putPeakFrequencySpc(peakFs, maxValue);

  /* copy and scale the data to display */
  memset(spcDataBuf, 0, sizeof(float)*SPC_GRAPH_HEIGHT);
  for (i = 0, j = 0; i < len && j < SPC_GRAPH_HEIGHT-1; i += dskip) {
    for (int n = 0; n < gskip; ++n, ++j) spcDataBuf[j] = (float)(amp)*pFft[i];
  }
  
  /* draw graph */
  putBufSpcGraph(spcFrameBuf, spcDataBuf
               , len, gskip, dskip, df
               , SPC_GRAPH_SIDE, SPC_GRAPH_HEAD
               , SPC_GRAPH_WIDTH, SPC_GRAPH_HEIGHT);
} 


/************** utility functions *********************/

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
  float log_f_offset;
  int interval_; 
  log_f_offset = log10f_fast(df);
  log_f_unit = log10f_fast(fmaxdisp)-log_f_offset;
  if (log_f_unit > 1.0) interval_ = (round)(FRAME_HEIGHT/log_f_unit);
  else interval_ = FRAME_HEIGHT-1;
  *f_min_log = (float)(log_f_offset*interval_);
  *interval = interval_; 
  MPLog("interval: %d\n", interval_);
  return;
}

int get_scale_param_for_spectro(int* gskip, int* dskip, int* fmaxdisp, int len, float df) {
  /* check range of fmaxdisp */
  int fmaxdisp_ = *fmaxdisp;
  int available_max_freq = (int)(df*(len-1));  /* = sampling_rate/2 */
  if (fmaxdisp_ > available_max_freq) fmaxdisp_ = available_max_freq; 

  int flen = len*fmaxdisp_/available_max_freq;
  
  /* calclate the display range */
  int gskip_, dskip_;
  if (flen < SPC_GRAPH_HEIGHT) {
    gskip_ = round((float)(SPC_GRAPH_HEIGHT)/flen); if (gskip_ == 0) gskip_ = 1;
    dskip_ = 1;
  } else {
    gskip_ = 1;
    dskip_ = round((float)(flen)/SPC_GRAPH_HEIGHT); if (dskip_ == 0) dskip_ = 1;
  }
  if (flen > len) flen = len;

#ifdef SCR_DEBUG
  MPLog("flen: %d  gskip: %d  dskip: %d\n", flen, gskip, dskip);
#endif
  *gskip = gskip_;
  *dskip = dskip_;
  return flen;
}


/* application of showing the result of saving the data (- and + pressed together) */
/*   "SAVED wf24003.csv #2" on the top right of the graph for SAVE_TEXT_MS          */
/*   (on the right of the graph on the LISSAJOUS screen: no space above the graph)   */
static uint32_t saveTextMs = 0;
static int saveTextX = 0;
static int saveTextY = 0;
static int saveTextW = 0;

static void clearSaveText() {
  if (saveTextW > 0) tft.fillRect(saveTextX, saveTextY, saveTextW, 8, ILI9341_BLACK);
  saveTextW = 0;
  saveTextMs = 0;
}

void appSaveStatus(struct SaveStatus* st) {
  char text[32];
  int color;
  if (st->state == SAVE_STATE_OK) {
    sprintf(text, "SAVED %s #%d", st->filename, st->rows);
    color = ILI9341_YELLOW;
  } else {
    strcpy(text, "SAVE ERROR");
    color = ILI9341_RED;
  }
  MPLog("%s\n", text);
  clearSaveText();
  saveTextW = strlen(text)*6;  /* the font of size 1 is 6 dots wide */
  if (scrType == SCR_TYPE_LSJU) {
    saveTextX = LSJ_TEXT_SIDE;
    saveTextY = LSJ_HEAD + 100;
  } else {
    saveTextX = FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH - saveTextW;
    saveTextY = (scrType == SCR_TYPE_SPCT) ? SPC_FPEAK_HEAD - 8 : FFT_GRAPH0_HEAD - 9;
  }
  putText(saveTextX, saveTextY, text, color, 1);
  saveTextMs = millis();
  if (saveTextMs == 0) saveTextMs = 1;
}

/* called in the main loop */
void appSaveStatusTimeout() {
  if (saveTextMs != 0 && millis() - saveTextMs >= SAVE_TEXT_MS) clearSaveText();
}

/* the page is cleared: do not erase the area on the next page */
void appForgetSaveStatus() {
  saveTextW = 0;
  saveTextMs = 0;
}
