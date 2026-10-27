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


/* Text with a GFX font: y is the baseline. font NULL is the built-in font */
void putTextFont(int x, int y, const char* str, int color, const GFXfont* font) {
  tft.setFont(font);
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(x, y);
  tft.print(str);
  tft.setFont(NULL);
}

/* width of the text with a GFX font */
int textWidth(const char* str, const GFXfont* font) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setFont(font);
  tft.setTextSize(1);
  tft.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
  tft.setFont(NULL);
  return w;
}

/* the text without the spaces at the both ends (the page files align them with spaces) */
String trimText(const char* str) {
  String s = (str == NULL) ? String("") : String(str);
  s.trim();
  return s;
}

/* a label of the button i (0-3) at the bottom */
void putButtonLabel(int i, const char* label) {
  int cx = uiButtonCenter[i];
  int x  = cx - UI_BUTTON_WIDTH/2;
  tft.fillRect(x, BUTTON_DECO_LINE + 2, UI_BUTTON_WIDTH, SCREEN_HEIGHT - BUTTON_DECO_LINE - 2, ILI9341_BLACK);
  String s = trimText(label);
  if (s.length() == 0) return;
  tft.drawRoundRect(x, BUTTON_DECO_LINE + 5, UI_BUTTON_WIDTH, 24, 5, UI_FRAME_COLOR);
  /* "(-)" and "(+)" of AMP: draw large symbols instead of the small font */
  if (s == "(-)" || s == "(+)" || s == "-" || s == "+") {
    int cy = BUTTON_DECO_LINE + 5 + 12;
    int half = UI_SYMBOL_SIZE/2;
    tft.fillRect(cx - half, cy - 1, UI_SYMBOL_SIZE, 3, UI_BUTTON_COLOR);
    if (s.indexOf('+') >= 0) tft.fillRect(cx - 1, cy - half, 3, UI_SYMBOL_SIZE, UI_BUTTON_COLOR);
    return;
  }
  int w = textWidth(s.c_str(), &FreeSans9pt7b);
  putTextFont(cx - w/2, UI_BUTTON_BASE, s.c_str(), UI_BUTTON_COLOR, &FreeSans9pt7b);
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

  if (fvalmax < 10.0) snprintf(strbuf, STRLEN, "%3.1f", fvalmax);  /* 1.0 - 9.x mV */
  else                sprintf(strbuf, "%3d", ivalmax);
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
/* the style of the graphs (see GRAPH_xxx in AppScreen.h) */
/* frameBuf is a bitmap of FFT_GRAPH_WIDTH x FFT_GRAPH_HEIGHT: the pixel of the row y */
/* and the column x is frameBuf[y*FRAME_HEIGHT + x] (writeLineToBuf takes (y, x)) */
static uint16_t graphPalette[GRAPH_HUES][GRAPH_LEVELS];
static bool     graphPaletteReady = false;
static uint8_t  graphGrid[2][FFT_GRAPH_WIDTH/8];  /* the vertical grid of the upper and lower graphs */
static int8_t   graphTop[FFT_GRAPH_WIDTH];        /* the row of the trace farthest from 0 in each column */
static int      graphBase;                        /* the row of 0 */
static uint32_t graphTimeUs = 0;
static int      graphTimeCount = 0;

static uint16_t rgb565(int r, int g, int b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static void makeGraphPalette() {
  /* from the color near 0 to the color at the full scale */
  static const uint8_t ends[GRAPH_HUES][6] = {
    { 10, 35, 110,   70, 145, 240 },  /* blue */
    {  5, 60,  30,   70, 220, 100 },  /* green */
    { 70, 10,  15,  240,  90,  80 },  /* red */
    { 60, 10,  60,  240,  80, 240 },  /* magenta */
  };
  for (int h = 0; h < GRAPH_HUES; ++h) {
    for (int i = 0; i < GRAPH_LEVELS; ++i) {
      int r = ends[h][0] + (ends[h][3] - ends[h][0])*i/(GRAPH_LEVELS-1);
      int g = ends[h][1] + (ends[h][4] - ends[h][1])*i/(GRAPH_LEVELS-1);
      int b = ends[h][2] + (ends[h][5] - ends[h][2])*i/(GRAPH_LEVELS-1);
      graphPalette[h][i] = rgb565(r, g, b);
    }
  }
  graphPaletteReady = true;
}

static int graphHue(uint16_t color) {
  if (color == ILI9341_GREEN) return GRAPH_HUE_GREEN;
  if (color == ILI9341_RED)   return GRAPH_HUE_RED;
  if (color == ILI9341_MAGENTA) return GRAPH_HUE_MAGENTA;
  return GRAPH_HUE_BLUE;
}

static uint16_t graphLineColor(uint16_t color) {
  static const uint16_t trace[GRAPH_HUES] = { GRAPH_TRACE_BLUE, GRAPH_TRACE_GREEN, GRAPH_TRACE_RED, GRAPH_TRACE_MAGENTA };
  return trace[graphHue(color)];
}

static int graphIndex(int head) {
  return (head == FFT_GRAPH0_HEAD) ? 0 : 1;
}

/* the vertical grid: put on the labeled marks of the horizontal scale */
void clearGraphGrid(int head) {
  memset(graphGrid[graphIndex(head)], 0, sizeof(graphGrid[0]));
}

void addGraphGrid(int head, int col) {
  if (col <= 0 || col >= FFT_GRAPH_WIDTH) return;  /* the left edge is the frame */
  graphGrid[graphIndex(head)][col >> 3] |= 1 << (col & 7);
}

/* the background with the grid, and the line of 0 if axis >= 0 */
static void clearGraphBuf(uint16_t* buf, int head, int axis) {
  if (!graphPaletteReady) makeGraphPalette();
  const uint8_t* grid = graphGrid[graphIndex(head)];
  for (int y = 0; y < FRAME_WIDTH; ++y) {
    uint16_t* row = buf + y*FRAME_HEIGHT;
    bool hline = ((y + 1) % (FRAME_WIDTH/GRAPH_GRID_ROWS)) == 0 && y < FRAME_WIDTH-1;
    if (y == axis) {
      for (int x = 0; x < FRAME_HEIGHT; ++x) row[x] = GRAPH_AXIS_COLOR;
    } else if (hline) {
      for (int x = 0; x < FRAME_HEIGHT; ++x) row[x] = GRAPH_GRID_COLOR;
    } else {
      for (int x = 0; x < FRAME_HEIGHT; ++x) {
        row[x] = (grid[x >> 3] & (1 << (x & 7))) ? GRAPH_GRID_COLOR : GRAPH_BG_COLOR;
      }
    }
  }
}

/* the fill: the farthest row from 0 is kept for each column while the lines are drawn, */
/* then each column is filled once (to blend CH2 over CH1 evenly) */
static void graphFillBegin(int base) {
  if (base < 0) base = 0;
  if (base > FRAME_WIDTH-1) base = FRAME_WIDTH-1;
  graphBase = base;
  memset(graphTop, base, sizeof(graphTop));
}

static void graphFillSegment(int row0, int col0, int row1, int col1) {
  if (col0 > col1) {
    int t = col0; col0 = col1; col1 = t;
    t = row0; row0 = row1; row1 = t;
  }
  int n = col1 - col0;
  for (int c = col0; c <= col1; ++c) {
    if (c < 0 || c >= FRAME_HEIGHT) continue;
    int r = (n == 0) ? ((abs(row0 - graphBase) > abs(row1 - graphBase)) ? row0 : row1)
                     : row0 + (row1 - row0)*(c - col0)/n;
    if (r < 0) r = 0;
    if (r > FRAME_WIDTH-1) r = FRAME_WIDTH-1;
    if (abs(r - graphBase) > abs(graphTop[c] - graphBase)) graphTop[c] = r;
  }
}

static bool isGraphTrace(uint16_t p) {
  return p == GRAPH_TRACE_BLUE || p == GRAPH_TRACE_GREEN || p == GRAPH_TRACE_RED
      || p == GRAPH_TRACE_MAGENTA;
}

static bool isGraphBackground(uint16_t p) {
  return p == GRAPH_BG_COLOR || p == GRAPH_GRID_COLOR || p == GRAPH_AXIS_COLOR;
}

static void graphFillEnd(uint16_t* buf, uint16_t color) {
  const uint16_t* pal = graphPalette[graphHue(color)];
  int span = (graphBase > FRAME_WIDTH-1-graphBase) ? graphBase : FRAME_WIDTH-1-graphBase;
  if (span < 1) span = 1;
  for (int c = 0; c < FRAME_HEIGHT; ++c) {
    int top = graphTop[c];
    if (top == graphBase) continue;
    int y0 = (top < graphBase) ? top : graphBase + 1;
    int y1 = (top < graphBase) ? graphBase - 1 : top;
    for (int y = y0; y <= y1; ++y) {
      uint16_t* p = buf + y*FRAME_HEIGHT + c;
      if (isGraphTrace(*p)) continue;  /* keep the lines of the other channel */
      uint16_t col = pal[abs(y - graphBase)*(GRAPH_LEVELS-1)/span];
      if (isGraphBackground(*p)) *p = col;
      else *p = ((*p & 0xF7DE) >> 1) + ((col & 0xF7DE) >> 1);  /* CH2 over CH1: the average */
    }
  }
}

/* the time to make and send a graph (the average of GRAPH_TIME_FRAMES graphs) */
static void graphTimeLog(uint32_t t0, bool draw) {
  graphTimeUs += micros() - t0;
  if (!draw) return;
  if (++graphTimeCount >= GRAPH_TIME_FRAMES) {
    MPLog("graph draw %d us\n", (int)(graphTimeUs/graphTimeCount));
    graphTimeUs = 0;
    graphTimeCount = 0;
  }
}

void putBufLinearGraph(uint16_t* frameBuf, float* graph
                     , float max_vol, int gskip, int side, int head, int width, int height
                     , uint16_t color, float df, int offset, bool clr, bool draw) {
  uint32_t t0 = micros();
  int base = FRAME_WIDTH - offset - 1;   /* the row of 0 */
  if (clr) clearGraphBuf(frameBuf, head, (offset > 0) ? base : -1);
  uint16_t line = graphLineColor(color);
  graphFillBegin(base);
    
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
    writeLineToBuf(frameBuf, val0, m, val1, m+gskip, line);
    graphFillSegment(val0, m, val1, m+gskip);
  }
  graphFillEnd(frameBuf, color);
  
  if (draw) tft.drawRGBBitmap(side, head, (uint16_t*)frameBuf, width, height);
  graphTimeLog(t0, draw);
}


/* the vertical grid of a log graph: on the decades (10Hz, 100Hz, 1kHz, ...) */
static void putLogGraphGrid(int head, int interval, double f_min_log) {
  clearGraphGrid(head);
  for (int32_t s = 10; s <= 100000; s *= 10) {
    addGraphGrid(head, (int)(log10f_fast(s)*interval - f_min_log));
  }
}

/* Draw a log graph on FFT applications */
/* graph: the fft data of every bin (bin n is n*df Hz), scale: multiplied to the data */
/* the bin 1 (df) is on the left edge: every bin is drawn, not to leave a blank */
/* on the low frequency side, and not to miss a peak on the high frequency side */
void putBufLogGraph(uint16_t* frameBuf, float* graph, float scale
                  , float max_vol, int len, int dskip, int side, int head, int width, int height
                  , uint16_t color, float df, int interval, double f_min_log
                  , bool clr, bool draw, int offset) {
  uint32_t t0 = micros();
  int base = FRAME_WIDTH - offset;   /* the row of 0 */
  if (base > FRAME_WIDTH-1) base = FRAME_WIDTH-1;
  if (clr) {
    putLogGraphGrid(head, interval, f_min_log);
    clearGraphBuf(frameBuf, head, (offset > 0) ? base : -1);
  }
  uint16_t line = graphLineColor(color);
  graphFillBegin(base);
  len /= dskip;
  for (int n = 1; n < len; ++n) {
    int val0 = (int)(graph[n]*scale*(FRAME_WIDTH-offset)/max_vol) + offset;
    if (val0 >= FRAME_WIDTH) val0 = FRAME_WIDTH-1;
    else if (val0 < 0)       val0 = 0;
    val0 = FRAME_WIDTH - val0;
    int val1 = (int)(graph[n+1]*scale*(FRAME_WIDTH-offset)/max_vol) + offset;
    if (val1 >= FRAME_WIDTH) val1 = FRAME_WIDTH-1;
    else if (val1 < 0)       val1 = 0;
    val1 = FRAME_WIDTH - val1;
    
    /* calculate log cordination */
    int iy0 = log10f_fast(n*df*dskip)*interval - f_min_log;         
    if (iy0 < 0) iy0 = 0; if (iy0 >= FRAME_HEIGHT) iy0 = FRAME_HEIGHT-1;
    int iy1 = log10f_fast((n+1)*df*dskip)*interval - f_min_log; 
    if (iy1 < 0) iy1 = 0; if (iy1 >= FRAME_HEIGHT) iy1 = FRAME_HEIGHT-1;
    writeLineToBuf(frameBuf, val0, iy0, val1, iy1, line);
    graphFillSegment(val0, iy0, val1, iy1);
  }
  graphFillEnd(frameBuf, color);

  if (draw) tft.drawRGBBitmap(side, head, (uint16_t*)frameBuf, width, height);
  graphTimeLog(t0, draw);
}


/* Draw a log graph on FFT applications */
/* graph: the fft data of every bin, converted to dBV here (see putBufLogGraph) */
void putBufdBVGraph(uint16_t* frameBuf, float* graph
                  , int max_dbv, int min_dbv, int len, int dskip, int side, int head, int width, int height
                  , uint16_t color, float df, int interval, double f_min_log
                  , bool clr, bool draw, int offset) {
  uint32_t t0 = micros();
  int base = FRAME_WIDTH - offset;   /* the row of the minimum */
  if (base > FRAME_WIDTH-1) base = FRAME_WIDTH-1;
  if (clr) {
    putLogGraphGrid(head, interval, f_min_log);
    clearGraphBuf(frameBuf, head, (offset > 0) ? base : -1);
  }
  uint16_t line = graphLineColor(color);
  graphFillBegin(base);
  len /= dskip;
  int range = max_dbv - min_dbv;
  float value;
  for (int n = 1; n < len-1; ++n) {
    value = (20*log10f_fast(graph[n]*WAV_MAX_VOL/1000) - min_dbv)/range;
    int val0 = (int)(value*(FRAME_WIDTH-offset)) + offset;
    if (val0 >= FRAME_WIDTH) val0 = FRAME_WIDTH-1;
    else if (val0 < 0)       val0 = 0;
    val0 = FRAME_WIDTH - val0;
    value = (20*log10f_fast(graph[n+1]*WAV_MAX_VOL/1000) - min_dbv)/range;
    int val1 = (int)(value*(FRAME_WIDTH-offset)) + offset;
    if (val1 >= FRAME_WIDTH) val1 = FRAME_WIDTH-1;
    else if (val1 < 0)       val1 = 0;
    val1 = FRAME_WIDTH - val1;
    
    /* calculate log cordination */
    int iy0 = log10f_fast(n*df*dskip)*interval - f_min_log;         
    if (iy0 < 0) iy0 = 0; if (iy0 >= FRAME_HEIGHT) iy0 = FRAME_HEIGHT-1;
    int iy1 = log10f_fast((n+1)*df*dskip)*interval - f_min_log; 
    if (iy1 < 0) iy1 = 0; if (iy1 >= FRAME_HEIGHT) iy1 = FRAME_HEIGHT-1;
    writeLineToBuf(frameBuf, val0, iy0, val1, iy1, line);
    graphFillSegment(val0, iy0, val1, iy1);
  }
  graphFillEnd(frameBuf, color);

  if (draw) tft.drawRGBBitmap(side, head, (uint16_t*)frameBuf, width, height);
  graphTimeLog(t0, draw);
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

  /* from the start frequency (FFT START SETTING) */
  if (fstartshown > 0) {
    float fspan = df*flen;
    float step = freq_scale_step(fspan);
    int nlen = flen*gskip/dskip;
    for (float f = ceilf(fstartshown/step)*step; f < fstartshown + fspan; f += step) {
      int nheight = (f - fstartshown)/fspan*nlen;
      if (nheight >= SPC_GRAPH_HEIGHT) break;
      tft.drawLine(SPC_GRAPH_SIDE+SPC_GRAPH_WIDTH
                 , SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT-nheight
                 , SPC_GRAPH_SIDE+SPC_GRAPH_WIDTH+3
                 , SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT-nheight
                 , ILI9341_YELLOW);
      putText(SPC_GRAPH_SIDE+SPC_GRAPH_WIDTH +8
            , SPC_GRAPH_HEAD+SPC_GRAPH_HEIGHT-nheight-4
            , freq_scale_text(f, step), ILI9341_YELLOW, 1);
    }
    return;
  }

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
  clearGraphGrid(head);
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
      addGraphGrid(head, line);
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
  clearGraphGrid(head);
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
      addGraphGrid(head, line);
      String sline = String(n/1000) + String("kHz");
      putText(FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+4
             , sline, ILI9341_YELLOW, 1);
      bMark = false;
    }
  } 
}


/* the step of the scale from the start frequency: 1, 2, 5 x 10^n Hz for 5 marks at most */
static float freq_scale_step(float fspan) {
  for (float p = 1.0f; p < 1.0e6f; p *= 10.0f) {
    if (fspan/(1.0f*p) <= 5.0f) return 1.0f*p;
    if (fspan/(2.0f*p) <= 5.0f) return 2.0f*p;
    if (fspan/(5.0f*p) <= 5.0f) return 5.0f*p;
  }
  return 1.0e6f;
}

/* a mark of the scale: "500Hz", "12kHz", "12.5k", "12.25k" */
static String freq_scale_text(float f, float step) {
  char text[12];
  if (f < 1000.0f)          sprintf(text, "%dHz", (int)(f + 0.5f));
  else if (step >= 1000.0f) sprintf(text, "%dkHz", (int)(f/1000.0f + 0.5f));
  else if (step >= 100.0f)  sprintf(text, "%.1fk", f/1000.0f);
  else if (step >= 10.0f)   sprintf(text, "%.2fk", f/1000.0f);
  else                      sprintf(text, "%.3fk", f/1000.0f);
  return String(text);
}

/* plot a linear scale which starts from f0 Hz (FFT START SETTING) */
void plotlinearscale_from(float f0, float fspan, int head) {
  float step = freq_scale_step(fspan);
  clearGraphGrid(head);
  for (float f = ceilf(f0/step)*step; f < f0 + fspan; f += step) {
    uint16_t line = (f - f0)/fspan*FFT_GRAPH_WIDTH;
    tft.drawLine(FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+1
               , FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+3
               , ILI9341_YELLOW);
    addGraphGrid(head, line);
    putText(FFT_GRAPH_SIDE + line, head+FFT_GRAPH_HEIGHT+4
           , freq_scale_text(f, step), ILI9341_YELLOW, 1);
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
