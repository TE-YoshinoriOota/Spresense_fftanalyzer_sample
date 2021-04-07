#include "AppScreen.h"


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
    MPLog("Draw: %s\n", str.c_str());
    return true;
  }
  MPLog("putText error\n");
  return false;
}


/* Draw a linear graph on FFT/WAV applications */
void putBufLinearGraph(uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT], int graph[]
                     , int gskip, int side, int head, int width, int height
                     , uint16_t color, float df, int offset, bool clr, bool draw) {
                      
  if (clr) memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
    
  int m, n;
  for (m = 0, n = 0; m < FRAME_HEIGHT-gskip; m+=gskip, ++n) {
    if (n-1 >= FRAME_HEIGHT/gskip) break; // fail safe
    int val0 = graph[n] + offset;
    if (val0 > FRAME_WIDTH) val0 = FRAME_WIDTH;
    else if (val0 < 0)      val0 = 0;
    val0 = FRAME_WIDTH - val0;
    int val1 = graph[n+1] + offset;
    if (val1 > FRAME_WIDTH) val1 = FRAME_WIDTH;
    else if (val1 < 0)      val1 = 0;
    val1 = FRAME_WIDTH - val1;
    writeLineToBuf(frameBuf, val0, m, val1, m+gskip, color);
  }
  
  if (draw) tft.drawRGBBitmap(side, head, (uint16_t*)frameBuf, width, height);
}


/* Draw a log graph on FFT applications */
void putBufLogGraph(uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT], int graph[]
                  , int len, int dskip, int side, int head, int width, int height
                  , uint16_t color, float df, int interval, double f_min_log
                  , bool clr, bool draw, int offset) {

  if (clr) memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);

  int n;
  for (int n = 0; n < len; ++n) {
    int val0 = graph[n] + offset;
    if (val0 >= FRAME_WIDTH) val0 = FRAME_WIDTH-1;
    else if (val0 < 0)       val0 = 0;
    val0 = FRAME_WIDTH - val0;
    int val1 = graph[n+1] + offset;
    if (val1 >= FRAME_WIDTH) val1 = FRAME_WIDTH-1;
    else if (val1 < 0)       val1 = 0;
    val1 = FRAME_WIDTH - val1;
    
    /* calculate log cordination */
    int iy0 = log10(n*df*dskip)*interval - f_min_log;         
    if (iy0 < 0) iy0 = 0; if (iy0 >= FRAME_HEIGHT) iy0 = FRAME_HEIGHT-1;
    int iy1 = log10((n+1)*df*dskip)*interval - f_min_log; 
    if (iy1 < 0) iy1 = 0; if (iy1 >= FRAME_HEIGHT) iy1 = FRAME_HEIGHT-1;
    writeLineToBuf(frameBuf, val0, iy0, val1, iy1, color);
  }

  if (draw) tft.drawRGBBitmap(side, head, (uint16_t*)frameBuf, width, height);
}

/* plot virtical scale */
void plotvirticalscale(int head, int mag, bool ac) {

  tft.fillRect(FFT_GRAPH_SIDE+FFT_GRAPH_WIDTH+3, head-4
             , SCREEN_WIDTH-(FFT_GRAPH_SIDE+FFT_GRAPH_WIDTH+3)
             , FFT_GRAPH_HEIGHT+7, ILI9341_BLACK);

  tft.drawLine(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH, head
             , FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH+3, head
             , ILI9341_YELLOW);  

  tft.drawLine(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH, head + FFT_GRAPH_HEIGHT/2
             , FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH+3, head + FFT_GRAPH_HEIGHT/2
             , ILI9341_YELLOW); 
  
             
  tft.drawLine(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH, head + FFT_GRAPH_HEIGHT
             , FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH+3, head + FFT_GRAPH_HEIGHT
             , ILI9341_YELLOW);  

  if (ac == true) {
    float maxVoltage = 45.5/mag;
    String maxLabel;
    if (maxVoltage > 1.0) {
      maxLabel = String(" ") + String(maxVoltage, 1) + String(" mV");
    } else {
      maxLabel = String(" ") + String(maxVoltage*1000, 1) + String(" uV");      
    }
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +8, head -4
               , maxLabel, ILI9341_YELLOW, 1);
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +8, head + FFT_GRAPH_HEIGHT/2 -4
               , " 0.0", ILI9341_YELLOW, 1);   
    float minVoltage = -45.5/mag;
    String minLabel;
    if (minVoltage < -1.0) {
      minLabel = String(minVoltage, 1) + String(" mV");
    } else {
      minLabel = String(minVoltage*1000, 1) + String(" uV");      
    }
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +8, head + FFT_GRAPH_HEIGHT -4
               , minLabel, ILI9341_YELLOW, 1);
  } else {
    float maxVoltage = 45.5/mag;
    String maxLabel;
    if (maxVoltage > 1.0) {
      maxLabel = String(" ") + String(maxVoltage, 1) + String(" mV");
    } else {
      maxLabel = String(" ") + String(maxVoltage*1000, 1) + String(" uV");      
    }
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4, head -4
               , maxLabel, ILI9341_YELLOW, 1);         
    float midVoltage = 22.750/mag;
    String midLabel;
    if (midVoltage > 1.0) {
      midLabel = String(" ") + String(midVoltage, 1) + String(" mV");
    } else {
      midLabel = String(" ") + String(midVoltage*1000, 1) + String(" uV");      
    }   
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4, head + FFT_GRAPH_HEIGHT/2 -4
               , midLabel, ILI9341_YELLOW, 1);         
    putText(FFT_GRAPH_SIDE + FFT_GRAPH_WIDTH +4, head + FFT_GRAPH_HEIGHT -4
               , " 0.0", ILI9341_YELLOW, 1);
  }
}


/* plot a time scale on a liner graph for WAV applications */
void plottimescale(float df, int len, int head, bool redraw) {
  if (plotscale0_done == true) return;
  float srate = df*len; // sampling rate
  float dt = 1/srate*1000;  // misec time for capturing 1 sample
  float max_time = dt*len;
  bool bMark = false;
  MPLog("max_time: %f df: %f srate: %f, len: %d\n", max_time, df, srate, len);
  
  float dtime;
  if (max_time < 2.0) dtime        = 0.10;   // 1.3 msec
  else if (max_time < 5.0) dtime   = 0.25;   // 2.7 msec
  else if (max_time < 10.0) dtime  = 0.50;   // 5.3 msec
  else if (max_time < 15.0) dtime  = 1.00;   // 10.7 msec
  else if (max_time < 30.0) dtime  = 2.00;   // 21.3 msec
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
  if (redraw == false)
    plotscale0_done = true;   
}


/* plot a liear scale on a liner graph for FFT applications */
void plotlinearscale(float df, int gskip, int dskip, int head, bool redraw) {
  if (plotscale1_done == true) return;
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
  if (redraw == false)
    plotscale1_done = true;
}


/* plot a log scale on a log graph for FFT applications */
void plotlogscale(int interval, float df, double f_min_log, int head, bool redraw) {
  if (plotscale1_done == true) return;
  /* graph scale */
  /* put text of minimum frequency */
  String smark = String(df, 0) + String("Hz");
  putText(FFT_GRAPH_SIDE, head+FFT_GRAPH_HEIGHT+4
         , smark, ILI9341_YELLOW, 1);  
         
  for (int32_t s = 1; s < 1000000; s *= 10) {
    /* put scale number on a graph */
    float mark = (log10(s))*interval - f_min_log;
    if (mark > 0.0) {
      if (s > 100) {
        smark = String(s/1000) + String("kHz");
      } else {
        smark = String(s) + String("Hz");
      }
      putText(FFT_GRAPH_SIDE + mark, head+FFT_GRAPH_HEIGHT+4, smark, ILI9341_YELLOW, 1);
    }
    
    /* put scale on a graph */
    for (int32_t n = 1*s; n < 10*s; n += s) {
      int32_t logn = log10(n)*interval - f_min_log;
      if (logn >= FFT_GRAPH_WIDTH) return;
      if (logn >= 0.0) {
        tft.drawLine(FFT_GRAPH_SIDE + logn, head+FFT_GRAPH_HEIGHT+1
                   , FFT_GRAPH_SIDE + logn, head+FFT_GRAPH_HEIGHT+3
                   , ILI9341_YELLOW);
      } 
    }
  }
  
  if (redraw == false)
    plotscale1_done = true;
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
void writeLineToBuf(uint16_t fBuf[][FRAME_HEIGHT]
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
  if (y1 > FRAME_HEIGHT-1) y0 = FRAME_HEIGHT-1;

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
      frameBuf[y0][x0] = color;
      fBuf[y0][x0] = color;
    } else {
      frameBuf[x0][y0] = color;
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}


/* Helper function for drawing the circle for the orbit graph */ 
void writeOrBitGraphToBuf(uint16_t orbitBuf[ORBIT_SIZE][ORBIT_SIZE]
                    , int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  orbitBuf[x0][y0+r] = color;
  orbitBuf[x0][y0-r] = color;
  orbitBuf[x0+r][y0] = color;
  orbitBuf[x0-r][y0] = color;

  while (x < y) {
    if (f >= 0) {
        y--;
        ddF_y += 2;
        f += ddF_y;
    }
    ++x;
    ddF_x += 2;
    f += ddF_x;
    orbitBuf[x0+x][y0+y] = color;
    orbitBuf[x0-x][y0+y] = color;
    orbitBuf[x0+x][y0-y] = color;
    orbitBuf[x0-x][y0-y] = color;
    orbitBuf[x0+y][y0+x] = color;
    orbitBuf[x0-y][y0+x] = color;
    orbitBuf[x0+y][y0-x] = color;
    orbitBuf[x0-y][y0-x] = color;
  }

  // x axis
  for (x = 0; x < r; ++x) {
    orbitBuf[x0+x][y0] = color;
    orbitBuf[x0-x][y0] = color;
  }

  // y axis
  for (y = 0; y < r; ++y) {
    orbitBuf[x0][y0+y] = color;
    orbitBuf[x0][y0-y] = color;
  }
}



// tentative implemntation. no test/no check have been made.
// if you want to use this, please do look into the code before using.
void putLcdLineGraph(int graph[], int gskip, int x, int y, int w, int h, uint16_t color, int y_axis) {
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

// tentative implemntation. no test/no check have been made.
// if you want to use this, please do look into the code before using.
void putBufFillGraph(uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT], int graph[]
                   , int gskip, int x, int y, int w, int h, uint16_t color, int y_axis) {
  memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
  int m, n;
  for (int m = 0, n = 0; m < FRAME_HEIGHT; m+=gskip, ++n) {
    int val = graph[n] + y_axis;
    if (val > FRAME_WIDTH) val = FRAME_WIDTH;
    else if (val < 0)      val = 0;
    val = FRAME_WIDTH - val;
    if (val <= y_axis) {
      for (int o = val; o <= FRAME_WIDTH/2; ++o) {
        frameBuf[o][m] = color;
      }
    } else if (val >= FRAME_WIDTH/2) {
      for (int o = y_axis; o < val; ++o) {
        frameBuf[o][m] = color;
      }
    }
  }
  tft.drawRGBBitmap(x, y, (uint16_t*)frameBuf, w, h);  
}
