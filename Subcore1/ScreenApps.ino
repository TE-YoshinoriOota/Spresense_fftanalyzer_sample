#include "AppScreen.h"

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
  int interval;
  double f_max;
  float log_f_max;
  double f_min_log;
  static int last_w_amp = 0;
  static int last_f_amp = 0;
  
  pthread_mutex_lock(&mtx);
  int w_amp = wavamp0;
  int f_amp = fftamp1;
  pthread_mutex_unlock(&mtx);

  /* draw upper graph */
  MPLog("len0: %d  len1: %d  df: %1.4f\n", len0, len1, df); 
  int gskip, dskip;
  if (len0 < FFT_GRAPH_WIDTH) {
    gskip = FFT_GRAPH_WIDTH / len0; if (gskip == 0) gskip = 1;
    dskip = 1;
  } else {
    gskip = 1;
    dskip = len0/FFT_GRAPH_WIDTH; if (dskip == 0) dskip = 1;
  }

  /* copy and scale the data to display */
  memset(graphDataBuf, 0, sizeof(int)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len0; i += dskip, ++j) {
    graphDataBuf[j] = (int)(w_amp*pWav[i]);
  }

  /* draw vertical graph */
  if (w_amp != last_w_amp) plotvirticalscale(FFT_GRAPH0_HEAD, w_amp, true);
  last_w_amp = w_amp;
  
  MPLog("len0: %d  gskip: %d  dskip: %d\n", len0, gskip, dskip);
  plottimescale(df, len0, FFT_GRAPH0_HEAD, false);
  putBufLinearGraph(frameBuf, graphDataBuf, gskip, FFT_GRAPH_SIDE, FFT_GRAPH0_HEAD
                  , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, ILI9341_CYAN, df
                  , FFT_GRAPH_HEIGHT/2, true);

  /* draw lower graph */
  if (len1 < FFT_GRAPH_WIDTH) {
    gskip = FFT_GRAPH_WIDTH / len1; if (gskip == 0) gskip = 1;
    dskip = 1;
  } else {
    gskip = 1;
    dskip = len1/FFT_GRAPH_WIDTH; if (dskip == 0) dskip = 1;
  }

  /* draw vertical graph */
  if (f_amp != last_f_amp) plotvirticalscale(FFT_GRAPH1_HEAD, f_amp, false);
  last_f_amp = f_amp;

  /* preparation for log graph */
  if (bLogDisplay == true) {
    f_max = df*len1;
    log_f_max = log10(f_max)-1;
    if (log_f_max > 1.0) interval = (FRAME_HEIGHT-1)/(int16_t)(log_f_max);
    else interval = FRAME_HEIGHT;
    f_min_log = log10(df)*interval; 
  }
   
  /* copy and scale the data to display */
  memset(graphDataBuf, 0, sizeof(int)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len1; i += dskip, ++j) {
    graphDataBuf[j] = (int)(f_amp*pFft[i]);
  }

  if (bLogDisplay == false) {
    MPLog("len1: %d  gskip: %d  dskip: %d\n", len1, gskip, dskip);
    plotlinearscale(df, gskip, dskip, FFT_GRAPH1_HEAD, false);
    putBufLinearGraph(frameBuf, graphDataBuf, gskip, FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD
                    , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, ILI9341_MAGENTA, df);
  } else {
    plotlogscale(interval, df, f_min_log, FFT_GRAPH1_HEAD, false);
    putBufLogGraph(frameBuf, graphDataBuf, len1, dskip
                 , FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD
                 , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
                 , ILI9341_MAGENTA, df, interval, f_min_log);
  }
}


/* application of drawing FFT-FFT on each graph */
void appDraw2FftGraph(float* pFft, float* pSubFft, int len, float df) {
  int i, j;
  int interval;
  double f_max;
  float log_f_max;
  double f_min_log;
  static int last_f_amp0 = 0;
  static int last_f_amp1 = 0;
  
  pthread_mutex_lock(&mtx);
  int f_amp0 = fftamp0;
  int f_amp1 = fftamp1;
  pthread_mutex_unlock(&mtx);

  int gskip, dskip;
  if (len < FFT_GRAPH_WIDTH) {
    gskip = FFT_GRAPH_WIDTH / len; if (gskip == 0) gskip = 1;
    dskip = 1;
  } else {
    gskip = 1;
    dskip = len/FFT_GRAPH_WIDTH; if (dskip == 0) dskip = 1;
  }
  MPLog("len: %d  gskip: %d  dskip: %d\n", len, gskip, dskip);

  if (bLogDisplay == true) {
    f_max = df*len;
    log_f_max = log10(f_max)-1;
    if (log_f_max > 1.0) interval = (FRAME_HEIGHT-1)/(int16_t)(log_f_max);
    else interval = FRAME_HEIGHT;
    f_min_log = log10(df)*interval; 
  }
   
  /* copy and scale the ch1 data to display */
  memset(graphDataBuf, 0, sizeof(int)*FFT_GRAPH_WIDTH);
  memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
  for (i = 0, j = 0; i < len; i += dskip, ++j) {
    graphDataBuf[j] = (int)(f_amp0*pFft[i]);
  }

  /* draw upper graph */
  if (bLogDisplay == false) {
    plotlinearscale(df, gskip, dskip, FFT_GRAPH0_HEAD, true);
    putBufLinearGraph(frameBuf, graphDataBuf, gskip
                    , FFT_GRAPH_SIDE, FFT_GRAPH0_HEAD
                    , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
                    , ILI9341_CYAN, df);
  } else {
    plotlogscale(interval, df, f_min_log, FFT_GRAPH0_HEAD, true);
    putBufLogGraph(frameBuf, graphDataBuf, len, dskip
                 , FFT_GRAPH_SIDE, FFT_GRAPH0_HEAD
                 , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
                 , ILI9341_CYAN, df, interval, f_min_log);
  }

  /* copy and scale the ch2 data to display */
  memset(graphDataBuf, 0, sizeof(int)*FFT_GRAPH_WIDTH);
  memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
  for (i = 0, j = 0; i < len; i += dskip, ++j) {
    graphDataBuf[j] = (int)(f_amp1*pSubFft[i]);
  }

  /* draw lower graph */
  if (bLogDisplay == false) {
    plotlinearscale(df, gskip, dskip, FFT_GRAPH1_HEAD);
    putBufLinearGraph(frameBuf, graphDataBuf, gskip
                    , FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD
                    , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
                    , ILI9341_MAGENTA, df, 0, true, false);
  } else {
    plotlogscale(interval, df, f_min_log, FFT_GRAPH1_HEAD);
    putBufLogGraph(frameBuf, graphDataBuf, len, dskip
                 , FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD
                 , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
                 , ILI9341_MAGENTA, df, interval, f_min_log
                 , true, false);
  }

  /* draw vertical graph */
  if (f_amp0 != last_f_amp0) plotvirticalscale(FFT_GRAPH0_HEAD, f_amp0, false);
  last_f_amp0 = f_amp0;

  /* make fft diff data */
  for (int i = 0; i < len; ++i) {
    pSubFft[i] = abs(pFft[i] - pSubFft[i]);
  }

  /* copy and scale the diff data to display */
  memset(graphDataBuf, 0, sizeof(int)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len; i += dskip, ++j) {
    graphDataBuf[j] = (int)(f_amp1*pSubFft[i]);
  }

  /* draw lower graph */
  if (bLogDisplay == false) {
    putBufLinearGraph(frameBuf, graphDataBuf, gskip
                    , FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD
                    , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
                    , ILI9341_RED, df, 0, false, true);
  } else {
    plotlogscale(interval, df, f_min_log, FFT_GRAPH1_HEAD);
    putBufLogGraph(frameBuf, graphDataBuf, len, dskip
                 , FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD
                 , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT
                 , ILI9341_RED, df, interval, f_min_log
                 , false, true);
  }

  /* draw vertical graph */
  if (f_amp1 != last_f_amp1) plotvirticalscale(FFT_GRAPH1_HEAD, f_amp1, false);
  last_f_amp1 = f_amp1;
  
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

  MPLog("Plot displacement data to buffer\n");
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

  orbitBuf[val0][val1] = ILI9341_CYAN;

  MPLog("Write OrBitGraph\n");
  writeOrBitGraphToBuf(orbitBuf, ORBIT_SIZE/2, ORBIT_SIZE/2
                     , ORBIT_GRAPH_RADIUS, ILI9341_YELLOW);
                     
  int x = ORBIT_GRAPH_XCENTER-ORBIT_SIZE/2;
  int y = ORBIT_GRAPH_YCENTER-ORBIT_SIZE/2;
  tft.drawRGBBitmap(x, y, (uint16_t*)orbitBuf, ORBIT_SIZE, ORBIT_SIZE);

  if (last_o_amp != o_amp) plotorbitscale(o_amp);
  
  last_o_amp = o_amp;
}


/* application of drawind Raw WAV and Filtered WAV on each graph */
void appDrawFilterGraph(float* pWav, float *pSubWav, int len, float df) {
  int i,j;
  static int last_w_amp0 = 0;
  static int last_w_amp1 = 0;
  
  pthread_mutex_lock(&mtx);
  int w_amp0 = wavamp0;
  int w_amp1 = wavamp1;
  pthread_mutex_unlock(&mtx);

  /* draw upper graph */
  MPLog("len: %d  df: %1.4f\n", len, df); 
  int gskip, dskip;
  if (len < FFT_GRAPH_WIDTH) {
    gskip = FFT_GRAPH_WIDTH / len; if (gskip == 0) gskip = 1;
    dskip = 1;
  } else {
    gskip = 1;
    dskip = len/FFT_GRAPH_WIDTH; if (dskip == 0) dskip = 1;
  }

  /* copy and scale the data to display */
  memset(graphDataBuf, 0, sizeof(int)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len; i += dskip, ++j) {
    graphDataBuf[j] = (int)(w_amp0*pWav[i]);
  }

  MPLog("len: %d  gskip: %d  dskip: %d\n", len, gskip, dskip);
  plottimescale(df, len, FFT_GRAPH0_HEAD, true);
  putBufLinearGraph(frameBuf, graphDataBuf, gskip, FFT_GRAPH_SIDE, FFT_GRAPH0_HEAD
                  , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, ILI9341_CYAN, df
                  , FFT_GRAPH_HEIGHT/2, true);

  /* draw vertical graph */
  if (w_amp0 != last_w_amp0) plotvirticalscale(FFT_GRAPH0_HEAD, w_amp0, true);
  last_w_amp0 = w_amp0;
  
  /* copy and scale the data to display */
  memset(graphDataBuf, 0, sizeof(int)*FFT_GRAPH_WIDTH);
  for (i = 0, j = 0; i < len; i += dskip, ++j) {
    graphDataBuf[j] = (int)(w_amp1*pSubWav[i]);
  }

  plottimescale(df, len, FFT_GRAPH1_HEAD, false);
  putBufLinearGraph(frameBuf, graphDataBuf, gskip, FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD
                  , FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, ILI9341_CYAN, df
                  , FFT_GRAPH_HEIGHT/2, true);

  /* draw vertical graph */
  if (w_amp1 != last_w_amp1) plotvirticalscale(FFT_GRAPH1_HEAD, w_amp1, true);
  last_w_amp1 = w_amp1;

}
