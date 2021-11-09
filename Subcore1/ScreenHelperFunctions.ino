#include "AppScreen.h"

const static int STRLEN = 7;
char strbuf[STRLEN] = {};

/* Error Notifier */
void error_notifier(int n) {
  
  if (n & LED0_MASK)  digitalWrite(LED0, HIGH);
  if (n & LED1_MASK)  digitalWrite(LED1, HIGH);
  if (n & LED2_MASK)  digitalWrite(LED2, HIGH);
  if (n & LED3_MASK)  digitalWrite(LED3, HIGH);
  delay(100);

  if (n & LED0_MASK)  digitalWrite(LED0, LOW);
  if (n & LED1_MASK)  digitalWrite(LED1, LOW);
  if (n & LED2_MASK)  digitalWrite(LED2, LOW);
  if (n & LED3_MASK)  digitalWrite(LED3, LOW);
  delay(100);
}


/* Graphic related helper functions */
/* Decoration Line */
void putHorizonLine(int h, int color) {
  tft.drawLine(0, h, SCREEN_WIDTH-1, h, color);
}


/* Cursor on a menu or a page screen */
void putItemCursor(int x, int y, int color) {
  tft.fillRect(x, y, MENU_CUR_SIZE, MENU_CUR_SIZE, color);
}


/* Helper function to put texts */
bool putText(int x, int y, String str, int color, int tsize) {
  if ((x >= 0 && x < SCREEN_WIDTH) 
  &&  (y >= 0 && y < SCREEN_HEIGHT)
  &&  (color >= 0x0000 && color <= 0xFFFF)) 
  {
    tft.setCursor(x, y);
    tft.setTextColor(color);
    tft.setTextSize(tsize);
    tft.println(str);
#ifdef SCR_DEBUG
    MPLog("Draw: %s\n", str.c_str());
#endif
    return true;
  }

#ifdef SCR_DEBUG
  MPLog("putText error\n");
#endif
  return false;
}

void putMaxMinVoltage(float maxVoltage, float minVoltage, int head) {
  tft.fillRect(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4
               , head + FFT_GRAPH_HEIGHT/4 -4 
               , 50 , 10 , ILI9341_BLACK);
  tft.fillRect(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4
               , head + FFT_GRAPH_HEIGHT*3/4 -4 
               , 50 , 10 , ILI9341_BLACK);
  
  /* omitted this, because of no space for display */
  /*
  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +46
               , head + FFT_GRAPH_HEIGHT/4 -4
               , String("mV"), ILI9341_CYAN, 1);   
  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +46
               , head + FFT_GRAPH_HEIGHT*3/4 -4
               , String("mV"), ILI9341_BLUE, 1);   
  */
  
  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +10
               , head + FFT_GRAPH_HEIGHT/4 -4
               , String("+") + String(maxVoltage,2), ILI9341_CYAN, 1);   
  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +10
               , head+ FFT_GRAPH_HEIGHT*3/4 -4
               , String(minVoltage,2), ILI9341_BLUE, 1);     
}

void putPeakFrequencyInLinear(float peakFs, float value, int head) {

  char tmpbuf[STRLEN] = {};
  tft.fillRect(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4
        , head + FFT_GRAPH_HEIGHT*2/4 -4 
        , 40 , 30 , ILI9341_BLACK);
  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +10
        , head + FFT_GRAPH_HEIGHT*1/4 -4
        , String("= PEAK ="), ILI9341_WHITE, 1);
         
  putText(GRAPH_UNIT_SIDE
        , head + FFT_GRAPH_HEIGHT*2/4 -4
        , "Hz", ILI9341_GREEN, 1);
  /*
  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +46
        , head + FFT_GRAPH_HEIGHT*3/4 -10
        , "mV", ILI9341_GREEN, 1);
  */
  putText(GRAPH_UNIT_SIDE
        , head + FFT_GRAPH_HEIGHT*3/4 -10
        , "mV", ILI9341_GREEN, 1);

  memset(strbuf, '0', STRLEN*sizeof(char));
  sprintf(strbuf, "%5d", (int)(peakFs)); 
  sprintf_right_justifier(strbuf, STRLEN);
  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4
        , head + FFT_GRAPH_HEIGHT*2/4 -4
        , String(strbuf), ILI9341_GREEN, 1);

  memset(strbuf, '0', STRLEN*sizeof(char));
  sprintf(strbuf, "%3.1f", value);
  sprintf_right_justifier(strbuf, STRLEN);

  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4
        , head + FFT_GRAPH_HEIGHT*3/4 -10
        , String(strbuf), ILI9341_GREEN, 1);
  
}


void putPeakFrequencySpc(float peakFs, float value) {

  
  putText(SPC_FPEAK_SIDE, SPC_FPEAK_HEAD
        , String("PEAK: "), ILI9341_WHITE, 1);

  tft.fillRect(SPC_FPEAK_SIDE + 30
        , SPC_FPEAK_HEAD 
        , 40, 10, ILI9341_BLACK);
  sprintf(strbuf, "%5d", (int)(peakFs)); 
  sprintf_right_justifier(strbuf, STRLEN);
  putText(SPC_FPEAK_SIDE + 30
        , SPC_FPEAK_HEAD
        , String(strbuf), ILI9341_GREEN, 1);
  putText(SPC_FPEAK_SIDE + 70
        , SPC_FPEAK_HEAD
        , "Hz", ILI9341_GREEN, 1);


  tft.fillRect(SPC_FPEAK_SIDE + 100
        , SPC_FPEAK_HEAD 
        , 40, 10 , ILI9341_BLACK);

  memset(strbuf, '0', STRLEN*sizeof(char));
  sprintf(strbuf, "%3.1f", value);
  sprintf_right_justifier(strbuf, STRLEN);

  putText(SPC_FPEAK_SIDE + 100
        , SPC_FPEAK_HEAD
        , String(strbuf), ILI9341_GREEN, 1);

  putText(SPC_FPEAK_SIDE + 140 
        , SPC_FPEAK_HEAD
        , "mV", ILI9341_GREEN, 1);
}

void putColorRange(int amp) {
  const static int STRLEN = 4;
  char strbuf[STRLEN] = {};
  float fvalmax = (WAV_MAX_VOL/amp);

  int ivalmax;
  String unit;
  if (fvalmax >= 1.0) {
    ivalmax = (int)(fvalmax);
    unit = "mV";
  } else if (fvalmax < 1.0) {
    ivalmax = (int)(fvalmax/1000);
    unit = "uV"; 
  }

  sprintf(strbuf, "%3d", ivalmax);
  sprintf_right_justifier(strbuf, STRLEN);
  tft.fillRect(SPC_FPEAK_SIDE + 270
        , SPC_FPEAK_HEAD 
        , 40, 10 , ILI9341_BLACK);
  putText(SPC_FPEAK_SIDE + 270
        , SPC_FPEAK_HEAD
        , String(strbuf), ILI9341_YELLOW, 1);

  putText(SPC_FPEAK_SIDE + 295
        , SPC_FPEAK_HEAD
        , unit, ILI9341_YELLOW, 1); 


  putText(SPC_FPEAK_SIDE + 285
        , SPC_COL_HEAD + SPC_COL_HEIGHT+8
        , String("0"), ILI9341_YELLOW, 1);

  putText(SPC_FPEAK_SIDE + 295 
        , SPC_COL_HEAD + SPC_COL_HEIGHT+8
        , unit, ILI9341_YELLOW, 1);   

        
}

void putPeakFrequencyInPower(float peakFs, float value) {
  // need to implement for dBV view
}

/* Draw a linear graph on FFT/WAV applications */
void putBufLinearGraph(uint16_t* frameBuf, float* graph
                     , float max_vol, int gskip, int side, int head, int width, int height
                     , uint16_t color, float df, int offset, bool clr, bool draw) {
                      
  if (clr) memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
    
  int m, n;
  for (m = 0, n = 0; m < FRAME_HEIGHT-gskip; m+=gskip, ++n) {
    if (n-1 >= FRAME_HEIGHT/gskip) break; // fail safe
    int val0 = (int)(graph[n]*(FRAME_WIDTH-offset)/max_vol) + offset;
    if (val0 > FRAME_WIDTH) val0 = FRAME_WIDTH;
    else if (val0 < 0)      val0 = 0;
    val0 = FRAME_WIDTH - val0 -1;
    int val1 = (int)(graph[n+1]*(FRAME_WIDTH-offset)/max_vol) + offset;
    if (val1 > FRAME_WIDTH) val1 = FRAME_WIDTH;
    else if (val1 < 0)      val1 = 0;
    val1 = FRAME_WIDTH - val1 -1;
    writeLineToBuf(frameBuf, val0, m, val1, m+gskip, color);
  }
  
  if (draw) tft.drawRGBBitmap(side, head, (uint16_t*)frameBuf, width, height);
}


/* Draw a log graph on FFT applications */
void putBufLogGraph(uint16_t* frameBuf, float* graph
                  , float max_vol, int len, int dskip, int side, int head, int width, int height
                  , uint16_t color, float df, int interval, double f_min_log
                  , bool clr, bool draw, int offset) {

  if (clr) memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
  len /= dskip;
  for (int n = 0; n < len; ++n) {
    int val0 = (int)(graph[n]*(FRAME_WIDTH-offset)/max_vol) + offset;
    if (val0 >= FRAME_WIDTH) val0 = FRAME_WIDTH-1;
    else if (val0 < 0)       val0 = 0;
    val0 = FRAME_WIDTH - val0;
    int val1 = (int)(graph[n+1]*(FRAME_WIDTH-offset)/max_vol) + offset;
    if (val1 >= FRAME_WIDTH) val1 = FRAME_WIDTH-1;
    else if (val1 < 0)       val1 = 0;
    val1 = FRAME_WIDTH - val1;
    
    /* calculate log cordination */
    int iy0 = log10f_fast(n*df*dskip)*interval - f_min_log;         
    if (iy0 < 0) iy0 = 0; if (iy0 >= FRAME_HEIGHT) iy0 = FRAME_HEIGHT-1;
    int iy1 = log10f_fast((n+1)*df*dskip)*interval - f_min_log; 
    if (iy1 < 0) iy1 = 0; if (iy1 >= FRAME_HEIGHT) iy1 = FRAME_HEIGHT-1;
    writeLineToBuf(frameBuf, val0, iy0, val1, iy1, color);
  }

  if (draw) tft.drawRGBBitmap(side, head, (uint16_t*)frameBuf, width, height);
}


/* Draw a log graph on FFT applications */
void putBufdBVGraph(uint16_t* frameBuf, float* graph
                  , int max_dbv, int min_dbv, int len, int dskip, int side, int head, int width, int height
                  , uint16_t color, float df, int interval, double f_min_log
                  , bool clr, bool draw, int offset) {
  if (clr) memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
  len /= dskip;
  int range = max_dbv - min_dbv;
  float value;
  for (int n = 1; n < len-1; ++n) {
    value = (graph[n] - min_dbv)/range;
    int val0 = (int)(value*(FRAME_WIDTH-offset)) + offset;
    if (val0 >= FRAME_WIDTH) val0 = FRAME_WIDTH-1;
    else if (val0 < 0)       val0 = 0;
    val0 = FRAME_WIDTH - val0;
    value = (graph[n+1] - min_dbv)/range;
    int val1 = (int)(value*(FRAME_WIDTH-offset)) + offset;
    if (val1 >= FRAME_WIDTH) val1 = FRAME_WIDTH-1;
    else if (val1 < 0)       val1 = 0;
    val1 = FRAME_WIDTH - val1;
    
    /* calculate log cordination */
    int iy0 = log10f_fast(n*df*dskip)*interval - f_min_log;         
    if (iy0 < 0) iy0 = 0; if (iy0 >= FRAME_HEIGHT) iy0 = FRAME_HEIGHT-1;
    int iy1 = log10f_fast((n+1)*df*dskip)*interval - f_min_log; 
    if (iy1 < 0) iy1 = 0; if (iy1 >= FRAME_HEIGHT) iy1 = FRAME_HEIGHT-1;
    writeLineToBuf(frameBuf, val0, iy0, val1, iy1, color);
  }

  if (draw) tft.drawRGBBitmap(side, head, (uint16_t*)frameBuf, width, height);
}


void putBufSpcGraph(uint16_t* spcBuf, float* spcDataBuf
                  , int len, int gskip, int dskip, float df
                  , int side, int head
                  , int width, int height) {

  int nlen = len*gskip/dskip;
  if (nlen > height) nlen = height; // sometimes, the max freq cannot adjust to fit display because of rounding error

#ifdef SCR_DEBUG
  MPLog("nlen: %d, len: %d, gskip: %d, dskip: %d\n",nlen, len, gskip, dskip);
#endif

  // need to think about memory layout to use DMA
  // current implementation is too slow!
  for (int j = 1; j < width; ++j) {
    for (int i = 0; i < height; ++i) {
      *(spcFrameBuf + i*width+j-1) = *(spcFrameBuf+i*width+j); 
    }
  }

  for (int i = 0; i < nlen; ++i) {
    int val = spcDataBuf[i]*255;
    if (val < 0) val = 0;  else if (val > 255) val = 255;
    int n = (height-1)-i; // inverse the data
    *(spcFrameBuf + n*width + (width-1)) = pseudoColors[val];
  }
  
  tft.drawRGBBitmap(side, head, (uint16_t*)spcFrameBuf, width, height);
}

/* plot virtical scale */
void plotvirticalscale(int head, int mag, bool ac) {

  /* omitted to avoid flikker of a peak value
  tft.fillRect(FFT_GRAPH_SIDE+FFT_GRAPH_WIDTH+3, head-4
             , SCREEN_WIDTH-(FFT_GRAPH_SIDE+FFT_GRAPH_WIDTH+3)
             , FFT_GRAPH_HEIGHT+7, ILI9341_BLACK);
  */

  tft.drawLine(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH
             , head
             , FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH+3
             , head
             , ILI9341_YELLOW);  
 
             
  tft.drawLine(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH
             , head + FFT_GRAPH_HEIGHT
             , FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH+3
             , head + FFT_GRAPH_HEIGHT
             , ILI9341_YELLOW);  

  if (ac == true) {
    /* top of the virtical line */
    tft.fillRect(FFT_GRAPH_SIDE+FFT_GRAPH_WIDTH+3
           , head-4
           , 40
           , 10
           , ILI9341_BLACK);

    float maxVoltage = WAV_MAX_VOL/mag;
    memset(strbuf, '0', STRLEN*sizeof(char));
    sprintf(strbuf, "%3.1f", maxVoltage);
    sprintf_right_justifier(strbuf, STRLEN);
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +8
          , head -4
          , String(strbuf), ILI9341_YELLOW, 1);

    putText(GRAPH_UNIT_SIDE
          , head -4
          , "mV", ILI9341_YELLOW, 1);

    /* middle of the vertical line */
    tft.drawLine(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH
          , head + FFT_GRAPH_HEIGHT/2
          , FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH+3
          , head + FFT_GRAPH_HEIGHT/2
          , ILI9341_YELLOW); 
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +8
          , head + FFT_GRAPH_HEIGHT/2 -4
          , " 0.0", ILI9341_YELLOW, 1);

    /* bottom of the vertical line */
    tft.fillRect(FFT_GRAPH_SIDE+FFT_GRAPH_WIDTH+3
          , head + FFT_GRAPH_HEIGHT -4
          , 40
          , 10
          , ILI9341_BLACK);
    
    float minVoltage = -WAV_MAX_VOL/mag;
    memset(strbuf, '0', STRLEN*sizeof(char));
    sprintf(strbuf, "%3.1f", minVoltage);
    sprintf_right_justifier(strbuf, STRLEN);
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +8
          , head + FFT_GRAPH_HEIGHT -4
          , String(strbuf), ILI9341_YELLOW, 1);

    putText(GRAPH_UNIT_SIDE
          , head + FFT_GRAPH_HEIGHT -4
          , "mV", ILI9341_YELLOW, 1);

  } else {
    /* top of the vertical line */
    tft.fillRect(FFT_GRAPH_SIDE+FFT_GRAPH_WIDTH+3
          , head-4
          , 40
          , 10
          , ILI9341_BLACK);

    float maxVoltage = WAV_MAX_VOL/mag;
    memset(strbuf, '0', STRLEN*sizeof(char));
    sprintf(strbuf, "%3.1f", maxVoltage);
    sprintf_right_justifier(strbuf, STRLEN);
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4
          , head -4
          , String(strbuf), ILI9341_YELLOW, 1);   

    putText(GRAPH_UNIT_SIDE
          , head -4
          , String("mV"), ILI9341_YELLOW, 1);   
           
    /* To make a space for statistic data on display, midLabel was omitted */ 
    /*     
    float midVoltage = 22.750/mag;
    String midLabel;
    if (midVoltage > 1.0) {
      midLabel = String(" ") + String(midVoltage, 1) + String(" mV");
    } else {
      midLabel = String(" ") + String(midVoltage*1000, 1) + String(" uV");      
    } 
 
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4, head + FFT_GRAPH_HEIGHT/2 -4
               , midLabel, ILI9341_YELLOW, 1);
    */
   
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +8
          , head + FFT_GRAPH_HEIGHT -4
          , "  0.0", ILI9341_YELLOW, 1);
    putText(GRAPH_UNIT_SIDE
          , head + FFT_GRAPH_HEIGHT -4
          , String("mV"), ILI9341_YELLOW, 1);  
  }
}


void plotverticalscale_spc(float df, int flen, int gskip, int dskip) {

  int fmaxdisp = (int)(df*flen);
  int memori[5];
  memset(memori, 0, sizeof(int)*5);

  int unit;
  if (fmaxdisp <= 5000)       unit = 1000;
  else if (fmaxdisp <= 10000) unit = 2000;
  else                        unit = 5000;

  int nlen = flen*gskip/dskip;
  int n;
  for (n = 0; n < 5; ++n) {
    int plotfreq = (n+1)*unit;
    int nheight = nlen*plotfreq/fmaxdisp;
#ifdef SCR_DEBUG
    MPLog("%d px, %d Hz\n", nheight, plotfreq);
#endif
    if (nheight < SPC_GRAPH_HEIGHT) {
      memori[n] = nheight;
    } else break;
  }

  /* draw memori */
  for (int i = 0; i < n; ++i) {
    tft.drawLine(SPC_GRAPH_SIDE+SPC_GRAPH_WIDTH
               , SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT-memori[i]
               , SPC_GRAPH_SIDE+SPC_GRAPH_WIDTH+3
               , SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT-memori[i]
               , ILI9341_YELLOW);
    int val = (i+1)*unit/1000;
    putText(SPC_GRAPH_SIDE+SPC_GRAPH_WIDTH +8
          , SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT-memori[i]-4
          , String(val) + String(" kHz"), ILI9341_YELLOW, 1);
  }
}


/* plot virtical scale in dBV */
void plotvirticalscale_dbv(int head, int maxdbv, int mindbv) {

  tft.drawLine(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH, head
             , FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH+3, head
             , ILI9341_YELLOW);  

  tft.drawLine(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH, head + FFT_GRAPH_HEIGHT/2
             , FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH+3, head + FFT_GRAPH_HEIGHT/2
             , ILI9341_YELLOW); 
  
  tft.drawLine(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH, head + FFT_GRAPH_HEIGHT
             , FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH+3, head + FFT_GRAPH_HEIGHT
             , ILI9341_YELLOW);  

  /* top of the virtical line */
  tft.fillRect(FFT_GRAPH_SIDE+FFT_GRAPH_WIDTH+3
             , head-4
             , 40
             , 10
             , ILI9341_BLACK);

  memset(strbuf, '0', STRLEN*sizeof(char));
  sprintf(strbuf, "%3.1f", (float)maxdbv);
  sprintf_right_justifier(strbuf, STRLEN);
  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4
        , head -4
        , String(strbuf), ILI9341_YELLOW, 1);   
  putText(GRAPH_UNIT_SIDE 
        , head -4
        , "dBV", ILI9341_YELLOW, 1); 


  /* bottom of the virtical line */
  tft.fillRect(FFT_GRAPH_SIDE+FFT_GRAPH_WIDTH+3
             , head + FFT_GRAPH_HEIGHT -4
             , 40
             , 10
             , ILI9341_BLACK);

  memset(strbuf, '0', STRLEN*sizeof(char));
  sprintf(strbuf, "%3.1f", (float)mindbv);
  sprintf_right_justifier(strbuf, STRLEN);
  putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4
        , head + FFT_GRAPH_HEIGHT -4
        , String(strbuf), ILI9341_YELLOW, 1);
  putText(GRAPH_UNIT_SIDE 
        , head + FFT_GRAPH_HEIGHT -4
        , "dBV", ILI9341_YELLOW, 1);
}


/* plot a time scale on a liner graph for WAV applications */
void plottimescale(float df, int len, int head) {

  float srate = df*len; // sampling rate
  float dt = 1/srate*1000;  // misec time for capturing 1 sample
  float max_time = dt*len;
  bool bMark = false;
  MPLog("max_time: %f df: %f srate: %f, len: %d\n", max_time, df, srate, len);
  
  float dtime;
  if (max_time < 2.0)        dtime = 0.10;   // 1.3 msec
  else if (max_time < 5.0)   dtime = 0.25;   // 2.7 msec
  else if (max_time < 10.0)  dtime = 0.50;   // 5.3 msec
  else if (max_time < 15.0)  dtime = 1.00;   // 10.7 msec
  else if (max_time < 30.0)  dtime = 2.00;   // 21.3 msec
  else if (max_time < 100.0) dtime = 5.00;   // 32, 42.6, 64, 85.3 msec
  else if (max_time < 200.0) dtime = 10.0;   // 125.9  msec
  else dtime = 20.0;                         // 256.0  msec
  
  for (float n = 0; n < max_time; n += dtime) {
    uint16_t line = (double)(n)/max_time*FFT_GRAPH_WIDTH;
    tft.drawLine(FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+1
               , FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+3
               , ILI9341_YELLOW);  
    if (n == 0) {
      bMark = true;
    } else if (max_time < 2.0) { /* 1.3 msec */
      if (n == 0.5 || n == 1.0) bMark = true;
    } else if (max_time < 5.0) { /* 2.7 msec */
      if (n == 1.0 || n == 2.0) bMark = true;      
    } else if (max_time < 10.0) { /* 5.3 msec */
      if (n == 2.5 || n == 5.0) bMark = true;
    } else if (max_time < 15.0) { /* 10.7 msec */
      if (n == 2.5 || n == 5.0 || n == 7.5 || n == 10.0) bMark = true;
    } else if (max_time < 30.0) { /* 21.3 msec */
      if (n == 10.0 || n == 20.0) bMark = true;
    } else if (max_time < 100.0) { /* 32., 42.6, 64.0, 85.3 msec */
      if (n == 20.0 || n == 40.0 || n == 60.0 || n == 80.0) bMark = true;
    } else if (max_time < 200.0) { /* 127.9 msec */
      if (n == 50.0  || n == 100.0 || n == max_time-dtime) bMark = true;
    } else { /* 256.0 msec */
      if (n == 100.0 || n == 200.0 || n == max_time-dtime) bMark = true;
    }
    if (bMark) {
      String sline = String(n,1) + String("msec");
      putText(FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+4
             , sline, ILI9341_YELLOW, 1);
      bMark = false;
    }
  } 
}


void plottimescale_for_spc(uint32_t duration) {

  float maxtime = duration*SPC_GRAPH_WIDTH;

  uint32_t m_sec_d = 0;
  for (int n = SPC_GRAPH_WIDTH-1; n >= 0; n -= SPC_GRAPH_WIDTH/5) {
    tft.drawLine(SPC_GRAPH_SIDE + n, SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT+1
               , SPC_GRAPH_SIDE + n, SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT+3
               , ILI9341_YELLOW);
    float sec_d;
    if (m_sec_d > 99) {
      sec_d = (float)(m_sec_d)/1000.;  
      putText(FFT_GRAPH_SIDE + n, SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT+4
            , String(sec_d, 0) + String(" s"), ILI9341_YELLOW, 1);
    } else {
      putText(FFT_GRAPH_SIDE + n, SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT+4
            , String(m_sec_d) + String(" ms"), ILI9341_YELLOW, 1);
    }
    m_sec_d += duration*(SPC_GRAPH_WIDTH/5);
  }
}


/* plot a liear scale on a liner graph for FFT applications */
void plotlinearscale(float df, int gskip, int dskip, int head) {

  uint32_t max_freq = df*dskip*FFT_GRAPH_WIDTH/gskip;
  bool bMark = false;
  for (int32_t n = 0; n < max_freq; n += 1000) {
    uint16_t line = (double)(n)/max_freq*FFT_GRAPH_WIDTH;
    tft.drawLine(FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+1
               , FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+3
               , ILI9341_YELLOW);  
    if (n == 0) {
      bMark = true;
    } else if (max_freq < 10000) { /* in case of 16kHz */
      if (n == 1000 || n == 3000 || n == 5000 || n == max_freq-1000) bMark = true;
    } else if (max_freq < 30000) { /* in case of 48kHz */
      if (n == 5000 || n == 10000 || n == 15000 || n == 20000) bMark = true;      
    } else if (max_freq < 100000) { /* in case of 192kHz */
      if (n == 5000 || n == 25000 || n == 50000 || n == 75000 || n == max_freq-1000) bMark = true;
    }
    if (bMark) {
      String sline = String(n/1000) + String("kHz");
      putText(FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+4
             , sline, ILI9341_YELLOW, 1);
      bMark = false;
    }
  } 
}


/* plot a log scale on a log graph for FFT applications */
void plotlogscale(int interval, float df, double f_min_log, int head) {
  /* graph scale */
  /* put text of minimum frequency */
  String smark = String(df, 0) + String("Hz");
  putText(FFT_GRAPH_SIDE, head+FFT_GRAPH_HEIGHT+4
         , smark, ILI9341_YELLOW, 1);  
         
  for (int32_t s = 1; s <= 100000 /* less than 100kHz */ ; s *= 10) {
    /* put scale number on a graph */
    float mark = (log10f_fast(s))*interval - f_min_log;
    if (mark > 0.0) {
      if (s >= 1000 /* kHz */) {
        smark = String(s/1000) + String("kHz");
      } else {
        smark = String(s) + String("Hz");
      }
      putText(FFT_GRAPH_SIDE + mark, head+FFT_GRAPH_HEIGHT+4, smark, ILI9341_YELLOW, 1);
    }
    
    /* put scale on a graph */
    for (int32_t n = 1*s; n < 10*s; n += s) {
      int32_t logn = log10f_fast(n)*interval - f_min_log;
      if (logn >= FFT_GRAPH_WIDTH) return;
      if (logn >= 0.0) {
        tft.drawLine(FFT_GRAPH_SIDE + logn, head+FFT_GRAPH_HEIGHT+1
                   , FFT_GRAPH_SIDE + logn, head+FFT_GRAPH_HEIGHT+3
                   , ILI9341_YELLOW);
      } 
    }
  }
  
}


/* plot orbit scale for orbit graph */
void plotorbitscale(int mag) {
  String unitText;
  MPLog("Orbit magnification: %d\n", mag);

  tft.fillRect(ORBIT_UNIT_R_SIDE, ORBIT_GRAPH_YCENTER, ORBIT_TEXT_BOXW, ORBIT_TEXT_BOXH, ILI9341_BLACK);
  tft.fillRect(ORBIT_UNIT_L_SIDE, ORBIT_GRAPH_YCENTER, ORBIT_TEXT_BOXW, ORBIT_TEXT_BOXH, ILI9341_BLACK);
  tft.fillRect(ORBIT_GRAPH_XCENTER, ORBIT_UNIT_U_HEAD, ORBIT_TEXT_BOXW, ORBIT_TEXT_BOXH, ILI9341_BLACK);
  tft.fillRect(ORBIT_GRAPH_XCENTER, ORBIT_UNIT_L_HEAD, ORBIT_TEXT_BOXW, ORBIT_TEXT_BOXH, ILI9341_BLACK);

  String maxValue = String(ORBIT_LIMIT_UM / mag) + String(" um");
  unitText = "+" + maxValue;
  putText(ORBIT_UNIT_R_SIDE, ORBIT_GRAPH_YCENTER, unitText, ILI9341_YELLOW, 1);
  
  unitText = "-" + maxValue;
  putText(ORBIT_UNIT_L_SIDE, ORBIT_GRAPH_YCENTER, unitText, ILI9341_YELLOW, 1);

  unitText = "+" + maxValue;
  putText(ORBIT_GRAPH_XCENTER, ORBIT_UNIT_U_HEAD, unitText, ILI9341_YELLOW, 1);

  unitText = "-" + maxValue;
  putText(ORBIT_GRAPH_XCENTER, ORBIT_UNIT_L_HEAD, unitText, ILI9341_YELLOW, 1);
  
}

/* Helper function for drawing lines on a graph based on Bresenham's line algorithm */
#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

void writeLineToBuf(uint16_t* fBuf
                  , int16_t x0, int16_t y0
                  , int16_t x1, int16_t y1, int16_t color) {

  /* limitter */
  if (x0 < 0) x0 = 0;
  if (x1 < 0) x1 = 0;
  if (x0 > FRAME_WIDTH-1) x0 = FRAME_WIDTH-1;
  if (x1 > FRAME_WIDTH-1) x1 = FRAME_WIDTH-1;
  if (y0 < 0) y0 = 0;
  if (y1 < 0) y1 = 0;
  if (y0 > FRAME_HEIGHT-1) y0 = FRAME_HEIGHT-1;
  if (y1 > FRAME_HEIGHT-1) y1 = FRAME_HEIGHT-1;

  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      //fBuf[y0][x0] = color;
      *(fBuf+y0*FRAME_HEIGHT+x0) = color;
    } else {
      //fBuf[x0][y0] = color;
      *(fBuf+x0*FRAME_HEIGHT+y0) = color;
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}


/* Helper function for drawing the circle for the orbit graph */
/* 
void writeOrBitGraphToBuf(uint16_t orbitBuf[ORBIT_SIZE][ORBIT_SIZE]
                    , int16_t x0, int16_t y0, int16_t r, uint16_t color) {
*/
void writeOrBitGraphToBuf(uint16_t* orbitBuf
                    , int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  //orbitBuf[x0][y0+r] = color;
  *(orbitBuf+x0*ORBIT_SIZE+(y0+r)) = color;
  //orbitBuf[x0][y0-r] = color;
  *(orbitBuf+x0*ORBIT_SIZE+(y0-r)) = color;
  //orbitBuf[x0+r][y0] = color;
  *(orbitBuf+(x0+r)*ORBIT_SIZE+y0) = color;
  //orbitBuf[x0-r][y0] = color;
  *(orbitBuf+(x0-r)*ORBIT_SIZE+y0) = color;

  while (x < y) {
    if (f >= 0) {
        y--;
        ddF_y += 2;
        f += ddF_y;
    }
    ++x;
    ddF_x += 2;
    f += ddF_x;
    //orbitBuf[x0+x][y0+y] = color;
    *(orbitBuf+(x0+x)*ORBIT_SIZE+(y0+y)) = color;
    //orbitBuf[x0-x][y0+y] = color;
    *(orbitBuf+(x0-x)*ORBIT_SIZE+(y0+y)) = color;
    //orbitBuf[x0+x][y0-y] = color;
    *(orbitBuf+(x0+x)*ORBIT_SIZE+(y0-y)) = color;
    //orbitBuf[x0-x][y0-y] = color;
    *(orbitBuf+(x0-x)*ORBIT_SIZE+(y0-y)) = color;
    //orbitBuf[x0+y][y0+x] = color;
    *(orbitBuf+(x0+y)*ORBIT_SIZE+(y0+x)) = color;
    //orbitBuf[x0-y][y0+x] = color;
    *(orbitBuf+(x0-y)*ORBIT_SIZE+(y0+x)) = color;
    //orbitBuf[x0+y][y0-x] = color;
    *(orbitBuf+(x0+y)*ORBIT_SIZE+(y0-x)) = color;
    //orbitBuf[x0-y][y0-x] = color;
    *(orbitBuf+(x0-y)*ORBIT_SIZE+(y0-x)) = color;
  }

  // x axis
  for (x = 0; x < r; ++x) {
    //orbitBuf[x0+x][y0] = color;
    *(orbitBuf+(x0+x)*ORBIT_SIZE+y0) = color;
    //orbitBuf[x0-x][y0] = color;
    *(orbitBuf+(x0-x)*ORBIT_SIZE+y0) = color;
  }

  // y axis
  for (y = 0; y < r; ++y) {
    //orbitBuf[x0][y0+y] = color;
    *(orbitBuf+x0*ORBIT_SIZE+(y0+y)) = color;
    //orbitBuf[x0][y0-y] = color;
    *(orbitBuf+x0*ORBIT_SIZE+(y0-y)) = color;
  }
}

void sprintf_right_justifier(char* strbuf, int len) {
  char* tmpbuf = (char*)malloc(sizeof(char)*len); 
  memset(tmpbuf, ' ', sizeof(char)*len);

  int cnt = 0;
  for (int i = 0; i < len; ++i) {
    if (strbuf[i] >= 0x2B && strbuf[i] <= 0x7E) {
      ++cnt;
    }  
  }
  tmpbuf[len-1]='\0';
  if (cnt > 0) {
    for (int i = 0; i < len-1; ++i) {
      if (strbuf[i] >= 0x2B && strbuf[i] <= 0x7E) {
        tmpbuf[len-cnt-1] = strbuf[i];
        --cnt;      
      } 
    }
  }  
  memcpy(strbuf, tmpbuf, sizeof(char)*len);
  free(tmpbuf);
}

/* ----------------------------------------------------------------------
** Fast approximation to the log2() function.  It uses a two step
** process.  First, it decomposes the floating-point number into
** a fractional component F and an exponent E.  The fraction component
** is used in a polynomial approximation and then the exponent added
** to the result.  A 3rd order polynomial is used and the result
** when computing db20() is accurate to 7.984884e-003 dB.
** ------------------------------------------------------------------- */

float log2f_approx_coeff[4] = {1.23149591368684f, -4.11852516267426f, 6.02197014179219f, -3.13396450166353f};
float log2f_approx(float X) {
  float *C = &log2f_approx_coeff[0];
  float Y;
  float F;
  int E;

  // This is the approximation to log2()
  F = frexpf(fabsf(X), &E);

  //  Y = C[0]*F*F*F + C[1]*F*F + C[2]*F + C[3] + E;
  Y = *C++;
  Y *= F;
  Y += (*C++);
  Y *= F;
  Y += (*C++);
  Y *= F;
  Y += (*C++);
  Y += E;

  return(Y);
}



// tentative implemntation. no test/no check have been made.
// if you want to use this, please do look into the code before using.
void putLcdLineGraph(float graph[], int gskip, int x, int y, int w, int h, uint16_t color, int y_axis) {
  tft.fillRect(x, y, w, h, ILI9341_BLACK);
  int i,j;
  for (i = x, j = 0; i < w-1; i+=gskip, ++j) {
    int y_0 = y + y_axis;
    int pixv0 = y_0 - graph[j];
    if (pixv0 < y) pixv0 = y;
    else if (pixv0 > y + h) pixv0 = y + h; 
    int pixv1 = y_0-graphDataBuf[j+1];
    if (pixv1 < y) pixv1 = y;
    else if (pixv1 > y + h) pixv1 = y + h;
    tft.drawLine(i, pixv0, i+gskip, pixv1, color);
  }  
}
