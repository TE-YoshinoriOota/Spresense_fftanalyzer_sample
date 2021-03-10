//#define LCD_LINE_GRAPH
//#define BUF_FILL_GRAPH
//#define BUF_LINE_GRAPH
#define BUF_LOG_LINE_GRAPH


static int8_t scrType = 0;
static bool   scrChange = false;

static int nextScreen[5] = {-1,-1,-1,-1,-1};
static int backScreen = -1;

static uint8_t  item0_num = 0;
static uint8_t  item1_num = 0;
static uint8_t  inp_num = 0;
static int8_t   cur0 = 0;
static int8_t   cur1 = 0;
static uint16_t inpsel[100];
static int      graphDataBuf[FFT_GRAPH_WIDTH];
static bool     plotscale0_done = false;
static bool     plotscale1_done = false;
static int      fftamp = FFT_MIN_AMP;
static int      wavamp = WAV_MIN_AMP;

DynamicJsonDocument* doc;
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
  case SCR_TYPE_FFT2:
    if (wavamp == WAV_MAX_AMP) return;
    pthread_mutex_lock(&mtx);
      wavamp += WAV_AMP_STEP;
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
  case SCR_TYPE_FFT2:
    if (fftamp == FFT_MAX_AMP) return;
    pthread_mutex_lock(&mtx);
      fftamp += FFT_AMP_STEP;
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

void clearScreen(DynamicJsonDocument* jdoc) {
  scrType  = (*jdoc)["type"];
  scrChange = false;
  item0_num = 0;
  inp_num = 0;
  cur0 = 0;
  cur1 = 0;
  backScreen = -1;
  doc = jdoc;
  fftamp = FFT_MIN_AMP;
  wavamp = WAV_MIN_AMP;
  plotscale0_done = false;
  plotscale1_done = false;
  memset(inpsel, 0, sizeof(uint16_t)*100);
  memset(nextScreen, -1, sizeof(int)*5);
  memset(&response, 0, sizeof(struct Response));
  memcpy(response.label, "non", sizeof(char)*3);
  tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ILI9341_BLACK); 
}



/* UP/DOWN operation related functions */
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

void inpOperation(int cur) {
  int w = INPUT_BOX_WIDTH - INPUT_MARGIN -3;
  int h = INPUT_BOX_HEIGHT - INPUT_MARGIN -3;
  tft.fillRect(MENU_ITEM_SIDE, MENU_ITEM1_HEAD, w, h, ILI9341_BLACK); // clear box area
  putText(MENU_ITEM_SIDE, MENU_ITEM1_HEAD, String(inpsel[cur]), ILI9341_WHITE, 3); 
}

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
void buildTitle(DynamicJsonDocument* doc) {
  char* title = (*doc)["title"];
  putText(TITLE_SIDE, TITLE_HEAD, title, ILI9341_YELLOW, 2);
  putHorizonLine(TITLE_DECO_LINE, ILI9341_YELLOW);  
}

void buildAppSign(DynamicJsonDocument* doc) {
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

void buildInput(DynamicJsonDocument* doc) {
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

void buildMenu(DynamicJsonDocument* doc) {
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

void buildDMenu(DynamicJsonDocument* doc) {
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

void buildMonitor(DynamicJsonDocument* doc) {
  tft.drawRect(MON_BOX_SIDE, MON_BOX0_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  tft.drawRect(MON_BOX_SIDE, MON_BOX1_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  tft.drawRect(MON_BOX_SIDE, MON_BOX2_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  putSensorValue(0.0, 0.0, 0.0);
  putText(MON_UNIT_SIDE, MON_ELEM0_HEAD, String("m/s^2"), ILI9341_WHITE, 2);
  putText(MON_UNIT_SIDE, MON_ELEM1_HEAD, String("m/s"), ILI9341_WHITE, 2);
  putText(MON_UNIT_SIDE, MON_ELEM2_HEAD, String("m"), ILI9341_WHITE, 2);
}

void build2WayGraph(DynamicJsonDocument* doc) {
  tft.drawRect(FFT_BOX_SIDE, FFT_BOX0_HEAD, FFT_GRAPH_WIDTH+FFT_MARGIN+1, FFT_GRAPH_HEIGHT+FFT_MARGIN+1, ILI9341_YELLOW);  
  tft.drawRect(FFT_BOX_SIDE, FFT_BOX1_HEAD, FFT_GRAPH_WIDTH+FFT_MARGIN+1, FFT_GRAPH_HEIGHT+FFT_MARGIN+2, ILI9341_YELLOW); 
}


void buildChStatus(DynamicJsonDocument* doc) {
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

void buildButton(DynamicJsonDocument* doc) {
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

void buildNextBackConnection(DynamicJsonDocument* doc) {
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


/* Graphic related helper functions */
bool putText(int x, int y, String str, int color, int tsize) {
  if ((x >= 0 && x < 320) 
  &&  (y >= 0 && y < 240)
  &&  (color >= 0x0000 && color <= 0xFFFF)) 
  {
    tft.setCursor(x, y);
    tft.setTextColor(color);
    tft.setTextSize(tsize);
    tft.println(str);
    Serial.println("Draw: " + str);
    return true;
  }
  Serial.println("putText error");
  return false;
}

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

#define FRAME_WIDTH  FFT_GRAPH_HEIGHT
#define FRAME_HEIGHT FFT_GRAPH_WIDTH
static uint16_t frameBuf[FRAME_WIDTH][FRAME_HEIGHT];  
void putDraw2WayGraph(float* pWav, int len0, float* pFft, int len1, float df, int mode = FFT_MODE_WAV_FFT) {

  // static uint16_t frameBuf1[FFT_GRAPH_WIDTH][FFT_GRAPH_HIGHT];  
  memset(frameBuf, ILI9341_BLACK, sizeof(uint16_t)*FFT_GRAPH_WIDTH*FFT_GRAPH_HEIGHT);
  pthread_mutex_lock(&mtx);
  int f_amp = fftamp;
  int w_amp = wavamp;
  pthread_mutex_unlock(&mtx);

  MPLog("len0: %03d  len1: %03d  df: %1.4f\n", len0, len1, df); 
  
  int gskip, dskip;
  if (len0 < FFT_GRAPH_WIDTH) {
    gskip = FFT_GRAPH_WIDTH / len0; if (gskip == 0) gskip = 1;
    dskip = 1;
  } else {
    gskip = 1;
    dskip = len0/FFT_GRAPH_WIDTH; if (dskip == 0) dskip = 1;
  }
  int i,j;
  for (i = 0, j = 0; i < len0; i += dskip, ++j) {
    graphDataBuf[j] = pWav[i]*w_amp;
  }
  
  MPLog("len0: %03d  gskip: %03d  dskip: %03d\n", len0, gskip, dskip); 
  tft.fillRect(FFT_GRAPH_SIDE, FFT_GRAPH0_HEAD, FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, ILI9341_BLACK);
#if defined(LCD_LINE_GRAPH)
  for (i = FFT_GRAPH_SIDE, j = 0; i < FFT_GRAPH_WIDTH-1; i += gskip, ++j) {
    int center = FFT_GRAPH0_HEAD + FFT_GRAPH_HEIGHT/2;
    int pixv0 = center-graphDataBuf[j];
    if (pixv0 < FFT_GRAPH0_HEAD) pixv0 = FFT_GRAPH0_HEAD;
    else if (pixv0 > FFT_GRAPH0_HEAD + FFT_GRAPH_HEIGHT) pixv0 = FFT_GRAPH0_HEAD + FFT_GRAPH_HEIGHT; 
    int pixv1 = center-graphDataBuf[j+1];
    if (pixv1 < FFT_GRAPH0_HEAD) pixv1 = FFT_GRAPH0_HEAD;
    else if (pixv1 > FFT_GRAPH0_HEAD + FFT_GRAPH_HEIGHT) pixv1 = FFT_GRAPH0_HEAD + FFT_GRAPH_HEIGHT;
    tft.drawLine(i, pixv0, i+gskip, pixv1, ILI9341_CYAN);
  }
#elif defined(BUF_FILL_GRAPH)
  for (int y = 0; y < FRAME_HEIGHT; ++y) {
    int val = graphDataBuf[y] + FRAME_WIDTH/2;
    if (val > FRAME_WIDTH) val = FRAME_WIDTH;
    else if (val < 0)      val = 0;
    if (val <= FRAME_WIDTH/2) {
      for (int x = val; x <= FRAME_WIDTH/2; ++x) {
        frameBuf[x][y] = ILI9341_CYAN;
      }
    } else if (val >= FRAME_WIDTH/2) {
      for (int x = FRAME_WIDTH/2; x < val; ++x) {
        frameBuf[x][y] = ILI9341_CYAN;
      }
    }
  }
  tft.drawRGBBitmap(FFT_GRAPH_SIDE, FFT_GRAPH0_HEAD, (uint16_t*)frameBuf, FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT);
#elif defined(BUF_LINE_GRAPH) || defined(BUF_LOG_LINE_GRAPH)
  plotwavscale(df, len0);
  for (int y = 0; y < FRAME_HEIGHT-1; ++y) {
    int val0 = graphDataBuf[y] + FRAME_WIDTH/2;
    if (val0 > FRAME_WIDTH) val0 = FRAME_WIDTH;
    else if (val0 < 0)      val0 = 0;
    val0 = FRAME_WIDTH - val0;
    int val1 = graphDataBuf[y+1] + FRAME_WIDTH/2;
    if (val1 > FRAME_WIDTH) val1 = FRAME_WIDTH;
    else if (val1 < 0)      val1 = 0;
    val1 = FRAME_WIDTH - val1;
    //writeLineToBuf(val0, y, val1, y+1, ILI9341_CYAN);
    writeLineToBuf(frameBuf, val0, y, val1, y+1, ILI9341_CYAN);
  }
  tft.drawRGBBitmap(FFT_GRAPH_SIDE, FFT_GRAPH0_HEAD, (uint16_t*)frameBuf, FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT);
#else
  /* no graph */
  putText(FFT_GRAPH_SIDE+10, FFT_GRAPH0_HEAD/2-20, "not available now", ILI9341_RED, 3);
#endif

  if (len1 < FFT_GRAPH_WIDTH) {
    gskip = FFT_GRAPH_WIDTH / len1; if (gskip == 0) gskip = 1;
    dskip = 1;
  } else {
    gskip = 1;
    dskip = len1/FFT_GRAPH_WIDTH; if (dskip == 0) dskip = 1;
  }
   
  for (i = 0, j = 0; i < len1; i += dskip, ++j) {
    graphDataBuf[j] = pFft[i] * f_amp;
  }
  
  tft.fillRect(FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD, FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT, ILI9341_BLACK);
#if defined(LCD_LINE_GRAPH)
  MPLog("len1: %03d  gskip: %03d  dskip: %03d\n", len1, gskip, dskip); 
  for (int i = FFT_GRAPH_SIDE, j = 0; i < FFT_GRAPH_WIDTH-1; i += gskip, ++j) {
    int bottom = FFT_GRAPH1_HEAD + FFT_GRAPH_HEIGHT;
    int pixv0 = bottom-graphDataBuf[j];
    if (pixv0 < FFT_GRAPH1_HEAD) pixv0 = FFT_GRAPH1_HEAD;
    else if (pixv0 > FFT_GRAPH1_HEAD + FFT_GRAPH_HEIGHT) pixv0 = FFT_GRAPH1_HEAD + FFT_GRAPH_HEIGHT; 
    int pixv1 = bottom-graphDataBuf[j+1];
    if (pixv1 < FFT_GRAPH1_HEAD) pixv1 = FFT_GRAPH1_HEAD;
    else if (pixv1 > FFT_GRAPH1_HEAD + FFT_GRAPH_HEIGHT) pixv1 = FFT_GRAPH1_HEAD + FFT_GRAPH_HEIGHT; 
    tft.drawLine(i, pixv0, i+gskip, pixv1, ILI9341_MAGENTA);
  }
#elif defined(BUF_FILL_GRAPH)
  memset(frameBuf, ILI9341_BLACK, sizeof(uint16_t)*FRAME_HEIGHT*FRAME_WIDTH);
  for (int y = 0; y < FRAME_HEIGHT; ++y) {
    int val = graphDataBuf[y];
    if (val > FRAME_WIDTH) val = FRAME_WIDTH;
    else if (val < 0)      val = 0;
    val = FRAME_WIDTH - val;
    for (int x = FRAME_WIDTH-1; x >= val; --x) {
      frameBuf[x][y] = ILI9341_MAGENTA;
    }
  }
  tft.drawRGBBitmap(FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD, (uint16_t*)frameBuf, FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT);
#elif defined(BUF_LINE_GRAPH)
  memset(frameBuf, ILI9341_BLACK, sizeof(uint16_t)*FRAME_HEIGHT*FRAME_WIDTH);
  plotlinearscale(df, dskip);
  for (int y = 0; y < FRAME_HEIGHT-1; ++y) {
    int val0 = graphDataBuf[y];
    if (val0 > FRAME_WIDTH) val0 = FRAME_WIDTH;
    else if (val0 < 0)      val0 = 0;
    val0 = FRAME_WIDTH - val0;
    int val1 = graphDataBuf[y+1];
    if (val1 > FRAME_WIDTH) val1 = FRAME_WIDTH;
    else if (val1 < 0)      val1 = 0;
    val1 = FRAME_WIDTH - val1;
    writeLineToBuf(frameBuf, val0, y, val1, y+1, ILI9341_MAGENTA);
  }
  tft.drawRGBBitmap(FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD, (uint16_t*)frameBuf, FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT);
#elif defined(BUF_LOG_LINE_GRAPH) 
  memset(frameBuf, ILI9341_BLACK, sizeof(uint16_t)*FRAME_HEIGHT*FRAME_WIDTH);
  int interval;
  double f_max = df*len0/2.56;
  float log_f_max = log10(f_max)-1;
  if (log_f_max > 1.0) interval = (FRAME_HEIGHT-1)/(int16_t)(log_f_max);
  else interval = FRAME_HEIGHT;
  double f_min = log10(df)*interval;
  plotlogscale(interval, f_min);
  for (int y = 0; y < FRAME_HEIGHT-1; ++y) {
    int val0 = graphDataBuf[y];
    if (val0 >= FRAME_WIDTH) val0 = FRAME_WIDTH-1;
    else if (val0 < 0)      val0 = 0;
    val0 = FRAME_WIDTH - val0;
    int val1 = graphDataBuf[y+1];
    if (val1 >= FRAME_WIDTH) val1 = FRAME_WIDTH-1;
    else if (val1 < 0)      val1 = 0;
    val1 = FRAME_WIDTH - val1;
    
    /* calculate log cordination */
    int iy0 = log10(y*df)*interval - f_min;     if (iy0 < 0) iy0 = 0;
    int iy1 = log10((y+1)*df)*interval - f_min; if (iy1 < 0) iy1 = 0;
    writeLineToBuf(frameBuf, val0, iy0, val1, iy1, ILI9341_MAGENTA);
  }
  tft.drawRGBBitmap(FFT_GRAPH_SIDE, FFT_GRAPH1_HEAD, (uint16_t*)frameBuf, FFT_GRAPH_WIDTH, FFT_GRAPH_HEIGHT);
#else
  /* no graph */
  putText(FFT_GRAPH_SIDE+10, FFT_GRAPH1_HEAD/2-20, "not available now", ILI9341_RED, 3);
#endif
}

void putHorizonLine(int h, int color) {
  tft.drawLine(0, h, SCREEN_WIDTH-1, h, color);
}

void putItemCursor(int x, int y, int color) {
  tft.fillRect(x, y, MENU_CUR_SIZE, MENU_CUR_SIZE, color);
}

void plotwavscale(float df, int len) {
  if (plotscale1_done == true) return;
  float srate = df*len; // sampling rate
  float dt = 1/srate*1000;  // misec time for capturing 1 sample
  float max_time = dt*len;
  bool bMark = false;
  MPLog("max_time: %f df: %f srate: %f, len: %d\n", max_time, df, srate, len);
  
  /* 
   * minimum case: frame rate 192000Hz, sample  256, time  1.3 msec
   * maxmum  case: frame rate  16000Hz, sample 4096, time  256 msec
   */
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
    tft.drawLine(FFT_GRAPH_SIDE + line, FFT_GRAPH0_HEAD+FFT_GRAPH_HEIGHT+1
               , FFT_GRAPH_SIDE + line, FFT_GRAPH0_HEAD+FFT_GRAPH_HEIGHT+3
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
      putText(FFT_GRAPH_SIDE + line, FFT_GRAPH0_HEAD+FFT_GRAPH_HEIGHT+4
             , sline, ILI9341_YELLOW, 1);
      bMark = false;
    }
  } 
  plotscale0_done = true;  
  
  
}

void plotlinearscale(float df, int skip) {
  if (plotscale1_done == true) return;
  uint32_t max_freq = df*skip*FFT_GRAPH_WIDTH;
  bool bMark = false;
  for (int32_t n = 0; n < max_freq; n += 1000) {
    uint16_t line = (double)(n)/max_freq*FFT_GRAPH_WIDTH;
    tft.drawLine(FFT_GRAPH_SIDE + line, FFT_GRAPH1_HEAD+FFT_GRAPH_HEIGHT+1
               , FFT_GRAPH_SIDE + line, FFT_GRAPH1_HEAD+FFT_GRAPH_HEIGHT+3
               , ILI9341_YELLOW);  
    if (n == 0) continue;
    if (max_freq < 10000) { /* in case of 16kHz */
      if (n == 1000 || n == 3000 || n == 5000 || n == max_freq-1000) bMark = true;
    } else if (max_freq < 30000) { /* in case of 48kHz */
      if (n == 5000 || n == 10000 || n == 15000 || n == 20000) bMark = true;      
    } else if (max_freq < 100000) { /* in case of 192kHz */
      if (n == 5000 || n == 25000 || n == 50000 || n == 75000 || n == max_freq-1000) bMark = true;
    }
    if (bMark) {
      String sline = String(n/1000) + String("kHz");
      putText(FFT_GRAPH_SIDE + line, FFT_GRAPH1_HEAD+FFT_GRAPH_HEIGHT+4
             , sline, ILI9341_YELLOW, 1);
      bMark = false;
    }
  } 
  plotscale1_done = true;
}


void plotlogscale(int interval, double f_min_log) {
  if (plotscale1_done == true) return;
  /* graph scale */
  for (int32_t s = 1; s < 1000000; s *= 10) {
    float mark = (log10(s))*interval - f_min_log;
    if (mark > 0.0) {
      String smark;
      if (s > 100) {
        smark = String(s/1000) + String("kHz");
      } else {
        smark = String(s) + String("Hz");
      }
      putText(FFT_GRAPH_SIDE + mark, FFT_GRAPH1_HEAD+FFT_GRAPH_HEIGHT+4
             , smark, ILI9341_YELLOW, 1);
    }
    for (int32_t n = 1*s; n < 10*s; n += s) {
      int32_t logn = log10(n)*interval - f_min_log;
      if (logn >= FFT_GRAPH_WIDTH) return;
      if (logn >= 0.0 && logn >= 0.0) {
        tft.drawLine(FFT_GRAPH_SIDE + logn, FFT_GRAPH1_HEAD+FFT_GRAPH_HEIGHT+1
                   , FFT_GRAPH_SIDE + logn, FFT_GRAPH1_HEAD+FFT_GRAPH_HEIGHT+3
                   , ILI9341_YELLOW);
      } 
    }
  }
  plotscale1_done = true;
}


#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

/* Bresenham's line algorithm */
void writeLineToBuf(uint16_t fBuf[][FRAME_HEIGHT], int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t color) {

  if (x0 < 0 || x0 > FRAME_WIDTH-1  || x1 < 0 || x1 > FRAME_WIDTH-1) return;
  if (y0 < 0 || y0 > FRAME_HEIGHT-1 || y1 < 0 || y1 > FRAME_HEIGHT-1) return;

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
