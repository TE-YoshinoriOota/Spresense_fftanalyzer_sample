#include "AppScreen.h"



DynamicJsonDocument* doc;

/* to avoid conflict between the main loop and interrupt function */
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; 


/* "DOWN" Operation */
static void updateB0() {
  switch (scrType) {
  case SCR_TYPE_PAGE: 
  case SCR_TYPE_MENU:
    if (++cur0 == item0_num) cur0 = (item0_num - 1);
    curOperation(cur0);
    break;
  case SCR_TYPE_INPT:  
    if (--cur0 < 0) cur0 = 0;
    inpOperation(cur0);
    break;
  case SCR_TYPE_DMNU:
    if (++cur0 == item0_num) cur0 = 0;
    curOperation(cur0, cur1);
    break; 
  case SCR_TYPE_WVFT:
  case SCR_TYPE_FFT2:
    if (wavamp0 == WAV_MAX_AMP) return;
    if (fftamp0 == FFT_MAX_AMP) return;
    pthread_mutex_lock(&mtx);
      wavamp0 += WAV_AMP_STEP;
      fftamp0 += FFT_AMP_STEP;
    pthread_mutex_unlock(&mtx);
    break;
  }
}


/* "UP" Operation */
static void updateB1() {
  switch (scrType) {
  case SCR_TYPE_PAGE: 
  case SCR_TYPE_MENU:
    if (--cur0 < 0) cur0 = 0;
    curOperation(cur0);
    break;
  case SCR_TYPE_INPT:  
    if (++cur0 == inp_num) cur0 = (inp_num-1);
    inpOperation(cur0);  
    break;
  case SCR_TYPE_DMNU:
    if (++cur1 == item1_num) cur1 = 0;
    curOperation(cur0, cur1);
    break;   
  case SCR_TYPE_WVFT:
  case SCR_TYPE_FFT2:
    if (wavamp1 == WAV_MAX_AMP) return;
    if (fftamp1 == FFT_MAX_AMP) return;
    pthread_mutex_lock(&mtx);
      wavamp1 += WAV_AMP_STEP;
      fftamp1 += FFT_AMP_STEP;
    pthread_mutex_unlock(&mtx);
    break;

  }
}


/* "BACK" Operation */
static void updateB2() {
  if (backScreen < 0) return;
  scrChange = true;

  char *label = (*doc)["res_label"];
  int value;
  int next_id;
  if (scrType == SCR_TYPE_PAGE || scrType == SCR_TYPE_MENU) {
    String item = "item" + String(cur0) + "_v";
    value = (*doc)[item.c_str()];
  } else if (scrType == SCR_TYPE_INPT) {
    value = inpsel[cur0];
  } else if (scrType == SCR_TYPE_DMNU) {
    String item0 = "item0_v" + String(cur0);
    String item1 = "item1_v" + String(cur1);
    int ch_0 = (*doc)[item0.c_str()];
    int ch_1 = (*doc)[item1.c_str()];
    value = ch_1 << 8 | ch_0;
  }
  next_id = backScreen;
  updateResponse(label, value, cur0, cur1, next_id);
}


/* "NEXT" Operation */
static void updateB3() {
  if (nextScreen[0] < 0) return;
  scrChange = true;

  char *label = (*doc)["res_label"];
  int value;
  int next_id;
  
  if (scrType == SCR_TYPE_PAGE || scrType == SCR_TYPE_MENU) {
    String item = "item" + String(cur0) + "_v";
    value = (*doc)[item.c_str()];
  } else if (scrType == SCR_TYPE_INPT) {
    value = inpsel[cur0];
  } else if (scrType == SCR_TYPE_DMNU) {
    String item0 = "item0_v" + String(cur0);
    String item1 = "item1_v" + String(cur1);
    int ch_0 = (*doc)[item0.c_str()];
    int ch_1 = (*doc)[item1.c_str()];
    value = ch_1 << 8 | ch_0;
  }

  next_id = nextScreen[0];
  if (scrType == SCR_TYPE_PAGE) // overwrite
    next_id = nextScreen[cur0];

  updateResponse(label, value, cur0, cur1, next_id);
}


/* Screen operation related functions */
static bool updateScreen() {
  bool tmp = scrChange;
  scrChange = false;
  return tmp;
}


/* this function is called when the page changes */
void clearScreen(DynamicJsonDocument *jdoc) {
  scrType  = (*jdoc)["type"];
  scrChange = false;
  item0_num = 0;
  inp_num = 0;
  cur0 = 0;
  cur1 = 0;
  backScreen = -1;
  doc = jdoc;
  fftamp0 = FFT_MIN_AMP;
  wavamp0 = WAV_MIN_AMP;
  fftamp1 = FFT_MIN_AMP;
  wavamp1 = WAV_MIN_AMP;
  plotscale0_done = false;
  plotscale1_done = false;
  memset(inpsel, 0, sizeof(uint16_t)*100);
  memset(nextScreen, -1, sizeof(int)*5);
  memset(&response, 0, sizeof(struct Response));
  memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
  memset(orbitBuf, 0, sizeof(uint16_t)*ORBIT_SIZE*ORBIT_SIZE);
  memcpy(response.label, "non", sizeof(char)*3);
  tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ILI9341_BLACK); 
}


/* UP/DOWN operation related functions */
/* Cursor operation for menu and page screens */
void curOperation(int cur) {
  uint16_t celDim[4] = {
     MENU_ITEM0_HEAD
    ,MENU_ITEM1_HEAD
    ,MENU_ITEM2_HEAD
    ,MENU_ITEM3_HEAD
  };
  int w = MENU_ITEM_SIDE - TITLE_SIDE;
  int h = (BUTTON_HEAD-10) - MENU_ITEM0_HEAD;
  tft.fillRect(TITLE_SIDE, MENU_ITEM0_HEAD, w, h, ILI9341_BLACK); // clear box area
  putItemCursor(MENU_CUR_SIDE, celDim[cur], ILI9341_YELLOW);  
}


/* Input operation for input screens */
void inpOperation(int cur) {
  int w = INPUT_BOX_WIDTH - INPUT_MARGIN -3;
  int h = INPUT_BOX_HEIGHT - INPUT_MARGIN -3;
  tft.fillRect(MENU_ITEM_SIDE, MENU_ITEM1_HEAD, w, h, ILI9341_BLACK); // clear box area
  putText(MENU_ITEM_SIDE, MENU_ITEM1_HEAD, String(inpsel[cur]), ILI9341_WHITE, 3); 
}


/* Cursor operation for dual menu screens */
void curOperation(int cur0, int cur1) {
  uint16_t celDim[4] = {
     DMENU_ITEM0_HEAD
    ,DMENU_ITEM1_HEAD
    ,DMENU_ITEM2_HEAD
    ,DMENU_ITEM3_HEAD
  };
  int w0 = DMENU_ITEM_SIDE0 - DMENU_CUR_SIDE0;
  int h0 = (BUTTON_HEAD-10) - DMENU_ITEM0_HEAD;
  int w1 = DMENU_ITEM_SIDE1 - DMENU_CUR_SIDE1;
  int h1 = (BUTTON_HEAD-10) - DMENU_ITEM0_HEAD;
  tft.fillRect(DMENU_CUR_SIDE0, MENU_ITEM0_HEAD, w0, h0, ILI9341_BLACK); // clear box area
  tft.fillRect(DMENU_CUR_SIDE1, MENU_ITEM0_HEAD, w1, h1, ILI9341_BLACK); // clear box area
  putItemCursor(DMENU_CUR_SIDE0, celDim[cur0], ILI9341_YELLOW);  
  putItemCursor(DMENU_CUR_SIDE1, celDim[cur1], ILI9341_YELLOW);  
}


/* Building UI related functions */
/* Buidling Title on top of the screen */
void buildTitle(DynamicJsonDocument *doc) {
  char* title = (*doc)["title"];
  putText(TITLE_SIDE, TITLE_HEAD, title, ILI9341_YELLOW, 2);
  putHorizonLine(TITLE_DECO_LINE, ILI9341_YELLOW);  
}


/* Building a sign of application selected */
void buildAppSign(DynamicJsonDocument *doc) {
  static char appChar[6][4] = {
    " ","SET","MNT","FFT","DIF","OBT"
  }; 
  int16_t appid = (*doc)["id"];
  appid /= 100;
  if (appid != 0) {
    tft.fillRect(DISP_CH_SIDE-10, (TITLE_HEAD-10), 50, 30, ILI9341_GREEN);
    putText(DISP_CH_SIDE, TITLE_HEAD, appChar[appid], ILI9341_BLACK, 2);
  }
}


/* Building a input dialog box */
void buildInput(DynamicJsonDocument *doc) {
  inp_num = (*doc)["inp_num"];
  for (int i = 0; i < inp_num; ++i) {
    inpsel[i] = (*doc)["input"][i];
  }
  String inp_title = (*doc)["inp_title"];
  String inp_unit = (*doc)["inp_unit"];
  cur0 = (*doc)["cur0"];
  tft.drawRect(INPUT_BOX_SIDE, INPUT_BOX_HEAD, INPUT_BOX_WIDTH, INPUT_BOX_HEIGHT, ILI9341_YELLOW);  
  putText(INPUT_LABEL_SIDE, INPUT_LABEL_HEAD, inp_title, ILI9341_YELLOW, 2);
  putText(INPUT_UNIT_SIDE, INPUT_UNIT_HEAD, inp_unit, ILI9341_YELLOW, 3);
  putText(INPUT_ELEM_SIDE, INPUT_ELEM_HEAD, String(inpsel[0]), ILI9341_WHITE, 3);
  inpOperation(cur0);
}


/* Building items on a menu screen */
void buildMenu(DynamicJsonDocument *doc) {
  item0_num = (*doc)["item_num"];
  cur0 = (*doc)["cur0"];
  Serial.println("SELECTION ITEM :" + String(item0_num));
  for (int i = 0; i < item0_num; ++i) {
    String item = "item" + String(i);
    char* item_c = (*doc)[item.c_str()];
    putText(MENU_ITEM_SIDE, MENU_ITEM0_HEAD+40*i, item_c, ILI9341_WHITE, 2);
  }
  curOperation(cur0);
}


/* Building items on a double menu screen */
void buildDMenu(DynamicJsonDocument *doc) {
  item0_num = (*doc)["item0_num"];
  item1_num = (*doc)["item1_num"];
  cur0 = (*doc)["cur0"];
  cur1 = (*doc)["cur1"];
  Serial.println("SELECTION ITEM0 :" + String(item0_num));
  Serial.println("SELECTION ITEM1 :" + String(item0_num));
  for (int i = 0; i < item0_num; ++i) {
    String item = "item0_" + String(i);
    char* item_c = (*doc)[item.c_str()];
    putText(DMENU_ITEM_SIDE0, MENU_ITEM0_HEAD+40*i, item_c, ILI9341_WHITE, 2);
  }
  for (int i = 0; i < item1_num; ++i) {
    String item = "item1_" + String(i);
    char* item_c = (*doc)[item.c_str()];
    putText(DMENU_ITEM_SIDE1, MENU_ITEM0_HEAD+40*i, item_c, ILI9341_WHITE, 2);
  }
  curOperation(cur0, cur1);
}


/* Building the screen for the sensor monitor application */
void buildMonitor(DynamicJsonDocument *doc) {
  tft.drawRect(MON_BOX_SIDE, MON_BOX0_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  tft.drawRect(MON_BOX_SIDE, MON_BOX1_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  tft.drawRect(MON_BOX_SIDE, MON_BOX2_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  putSensorValue(0.0, 0.0, 0.0);
  putText(MON_UNIT_SIDE, MON_ELEM0_HEAD, String("m/s^2"), ILI9341_WHITE, 2);
  putText(MON_UNIT_SIDE, MON_ELEM1_HEAD, String("m/s"), ILI9341_WHITE, 2);
  putText(MON_UNIT_SIDE, MON_ELEM2_HEAD, String("m"), ILI9341_WHITE, 2);
}


/* Building the screen for 2 graph applications */
void build2WayGraph(DynamicJsonDocument *doc) {
  tft.drawRect(FFT_BOX_SIDE, FFT_BOX0_HEAD, FFT_GRAPH_WIDTH+FFT_MARGIN+1, FFT_GRAPH_HEIGHT+FFT_MARGIN+1, ILI9341_YELLOW);  
  tft.drawRect(FFT_BOX_SIDE, FFT_BOX1_HEAD, FFT_GRAPH_WIDTH+FFT_MARGIN+1, FFT_GRAPH_HEIGHT+FFT_MARGIN+2, ILI9341_YELLOW); 
}


/* Building the screen for the orbit graph application */
void buildOrbitGraph(DynamicJsonDocument *doc) {
  // currently no operation so far...??
  // the graph is drawn by putFunction operating graphic buffer
}

/* Building a sign of selected channel */
void buildChStatus(DynamicJsonDocument *doc) {
  int ch_disp = (*doc)["ch_disp"];
  Serial.println("CHANNEL DISPLAY :" + String(ch_disp));
  for (int i = 0; i < ch_disp; ++i) {
    String ch = "ch" + String(i);
    int8_t ich = (*doc)[ch.c_str()];
    String sch = "CH" + String(ich);
    tft.drawRect(DISP_CH_SIDE-10, (DISP_CH0_HEAD + 40*i)-10, 50, 30, ILI9341_GREEN);
    putText(DISP_CH_SIDE, DISP_CH0_HEAD+40*i, sch, ILI9341_GREEN, 2);
  }
}


/* Building buttons */
void buildButton(DynamicJsonDocument *doc) {
  char* b0 = (*doc)["B0"];
  char* b1 = (*doc)["B1"];
  char* b2 = (*doc)["B2"];
  char* b3 = (*doc)["B3"];
  Serial.println("Build Buttons"); 
  putHorizonLine(BUTTON_DECO_LINE, ILI9341_YELLOW);  
  putText(BUTTON0_SIDE, BUTTON_HEAD, b0, ILI9341_CYAN, 2);
  putText(BUTTON1_SIDE, BUTTON_HEAD, b1, ILI9341_CYAN, 2);
  putText(BUTTON2_SIDE, BUTTON_HEAD, b2, ILI9341_CYAN, 2);
  putText(BUTTON3_SIDE, BUTTON_HEAD, b3, ILI9341_CYAN, 2);  
}


/* Building a connection to a next page and a back page */
void buildNextBackConnection(DynamicJsonDocument *doc) {
  if (scrType == SCR_TYPE_PAGE) {
    for (int i = 0; i < item0_num; ++i) {
      String next = String("next") + String(i);
      nextScreen[i] = (*doc)[next.c_str()];
      MPLog("buildConn: %d\n", nextScreen[i]);
    }
  } else {
    nextScreen[0] = (*doc)["next0"];
    MPLog("buildConn: %d\n", nextScreen[0]);
  }
  backScreen = (*doc)["back"];
}


/* put sensor values to the sensor monitor screen */
void putSensorValue(float acc, float vel, float dis) {
  tft.fillRect(MON_ELEM_SIDE, MON_ELEM0_HEAD, MON_BOX_WIDTH-MON_MARGIN*2, MON_BOX_HEIGHT-MON_MARGIN*2+1, ILI9341_BLACK);
  tft.fillRect(MON_ELEM_SIDE, MON_ELEM1_HEAD, MON_BOX_WIDTH-MON_MARGIN*2, MON_BOX_HEIGHT-MON_MARGIN*2+1, ILI9341_BLACK);
  tft.fillRect(MON_ELEM_SIDE, MON_ELEM2_HEAD, MON_BOX_WIDTH-MON_MARGIN*2, MON_BOX_HEIGHT-MON_MARGIN*2+1, ILI9341_BLACK);
  char dataText[10];
  memset(dataText, 0, sizeof(char)*10);
  sprintf(dataText, "%+02.2f", acc);
  putText(MON_ELEM_SIDE, MON_ELEM0_HEAD, dataText, ILI9341_WHITE, 3);
  memset(dataText, 0, sizeof(char)*10);
  sprintf(dataText, "%+02.2f", vel);
  putText(MON_ELEM_SIDE, MON_ELEM1_HEAD, dataText, ILI9341_WHITE, 3);
  memset(dataText, 0, sizeof(char)*10);
  sprintf(dataText, "%+02.2f", dis);
  putText(MON_ELEM_SIDE, MON_ELEM2_HEAD, dataText, ILI9341_WHITE, 3);
}


/* put WAV and FFT data on graphs on the WAV-FFT application */
void putDraw2WayGraph(float *pWav, int len0, float *pFft, int len1, float df) {
  int i,j;
  int interval;
  double f_max;
  float log_f_max;
  double f_min_log;
  
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


/* put FFT and FFT data on graphs on the FFT-FFT application */
void putDraw2FftGraph(float *pFft, float *pSubFft, int len, float df) {
  int i, j;
  int interval;
  double f_max;
  float log_f_max;
  double f_min_log;
  
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
}


/* put orbit trace on the graph on the orbit application */
void putDrawOrbitGraph(float *pWav, float *pSubWav, int len, float df) {

  int val0, val1;
  int skip = len/ORBIT_SIZE;
  static int last_w_amp0 = 0;
  static int last_w_amp1 = 0;

  pthread_mutex_lock(&mtx);
  int w_amp0 = wavamp0;
  int w_amp1 = wavamp1;
  pthread_mutex_unlock(&mtx);

  if (w_amp0 != last_w_amp0 || w_amp1 != last_w_amp1) {
    // The magnification of the screen is changed. clear the screen.
    memset(orbitBuf, 0, sizeof(uint16_t)*ORBIT_SIZE*ORBIT_SIZE);
  }

  MPLog("Plot wav data to buffer\n");
  /* no need to care about graph direction with this orbit case */
  for (int i = 0; i < len; i+=skip) {
    if (i >= ORBIT_GRAPH_SIZE) break; // fail safe.
    
    val0 = (int)(w_amp0*pWav[i]);
    val1 = (int)(w_amp1*pSubWav[i]);

    if (val0 >= ORBIT_GRAPH_RADIUS)       val0 = ORBIT_GRAPH_RADIUS-1;
    else if (val0 <= -ORBIT_GRAPH_RADIUS) val0 = -(ORBIT_GRAPH_RADIUS-1);
  
    if (val1 >= ORBIT_GRAPH_RADIUS)       val1 = ORBIT_GRAPH_RADIUS-1;
    else if (val1 <= -ORBIT_GRAPH_RADIUS) val1 = -(ORBIT_GRAPH_RADIUS-1);

    val0 += ORBIT_GRAPH_RADIUS;
    val1 += ORBIT_GRAPH_RADIUS;
  
    orbitBuf[val0][val1] = ILI9341_CYAN;
  }

  MPLog("Write OrBitGraph\n");
  writeOrBitGraphToBuf(orbitBuf, ORBIT_SIZE/2, ORBIT_SIZE/2
                     , ORBIT_GRAPH_RADIUS, ILI9341_YELLOW);
                     
  int x = ORBIT_GRAPH_XCENTER-ORBIT_SIZE/2;
  int y = ORBIT_GRAPH_YCENTER-ORBIT_SIZE/2;
  tft.drawRGBBitmap(x, y, (uint16_t*)orbitBuf, ORBIT_SIZE, ORBIT_SIZE); 
  
  last_w_amp0 = w_amp0;
  last_w_amp1 = w_amp1;
}
