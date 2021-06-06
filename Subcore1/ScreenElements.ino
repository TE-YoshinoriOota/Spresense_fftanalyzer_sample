#include "AppScreen.h"

// #define DEMO_SETTING

DynamicJsonDocument* doc;


/* "DOWN" Operation */
static void updateB0() {
  noInterrupts();
  switch (scrType) {
  case SCR_TYPE_PAGE: 
  case SCR_TYPE_MENU:
    if (++cur0 == item0_num) cur0 = (item0_num - 1);
    curOperation(cur0);
    break;
  case SCR_TYPE_INPT:  
  case SCR_TYPE_SCLE:
    if (--cur0 < 0) cur0 = 0;
    inpOperation(cur0);
    break;
  case SCR_TYPE_DMNU:
    if (++cur0 == item0_num) cur0 = 0;
    curOperation(cur0, cur1);
    break; 
  case SCR_TYPE_WVFT:
  case SCR_TYPE_ORBT:
  case SCR_TYPE_FLTR:
  case SCR_TYPE_WVWV:
  case SCR_TYPE_WFLG:
  case SCR_TYPE_WFDB:
  case SCR_TYPE_FFT2:
  case SCR_TYPE_FTLG:
  case SCR_TYPE_FFDB:
    pthread_mutex_lock(&mtx);
    wavamp0 += WAV_AMP_STEP;
    if (wavamp0 == WAV_MAX_AMP) wavamp0 = WAV_MIN_AMP;
    fftamp0 += FFT_AMP_STEP;
    if (fftamp0 == FFT_MAX_AMP) fftamp0 = FFT_MIN_AMP;
    ++dbvdisp0;
    if (dbvdisp0 == FFT_DBV_RANGE_MAX) dbvdisp0 = 0;
    pthread_mutex_unlock(&mtx);
    break;
  }
  interrupts();
}


/* "UP" Operation */
static void updateB1() {
  noInterrupts();
  switch (scrType) {
  case SCR_TYPE_PAGE: 
  case SCR_TYPE_MENU:
    if (--cur0 < 0) cur0 = 0;
    curOperation(cur0);
    break;
  case SCR_TYPE_INPT:  
  case SCR_TYPE_SCLE:
    if (++cur0 == inp_num) cur0 = (inp_num-1);
    inpOperation(cur0);  
    break;
  case SCR_TYPE_DMNU:
    if (++cur1 == item1_num) cur1 = 0;
    curOperation(cur0, cur1);
    break;   
  case SCR_TYPE_ORBT:
    pthread_mutex_lock(&mtx);
    if (orbitamp == ORBIT_MAX_AMP) orbitamp = ORBIT_MIN_AMP;
    orbitamp += ORBIT_AMP_STEP;
    pthread_mutex_unlock(&mtx);
    break;

  case SCR_TYPE_FLTR:
  case SCR_TYPE_WVWV:
  case SCR_TYPE_WFLG:
  case SCR_TYPE_WFDB:
  case SCR_TYPE_WVFT:
  case SCR_TYPE_FTLG:
  case SCR_TYPE_FFT2:
  case SCR_TYPE_FFDB:
    pthread_mutex_lock(&mtx);
    wavamp1 += WAV_AMP_STEP;
    if (wavamp1 == WAV_MAX_AMP) wavamp1 = WAV_MIN_AMP;
    fftamp1 += FFT_AMP_STEP;
    if (fftamp1 == FFT_MAX_AMP) fftamp1 = FFT_MIN_AMP;
    ++dbvdisp1;
    if (dbvdisp1 == FFT_DBV_RANGE_MAX) dbvdisp1 = 0;
    pthread_mutex_unlock(&mtx);
    break;
  }
  interrupts();
}


/* "BACK" Operation */
static void updateB2() {
  if (backScreen < 0) return;
  noInterrupts();
  scrChange = true;

  char *label = (*doc)["res_label"];
  int value;
  int next_id;
  if (scrType == SCR_TYPE_PAGE || scrType == SCR_TYPE_MENU) {
    String item = "item" + String(cur0) + "_v";
    value = (*doc)[item.c_str()];
  } else if (scrType == SCR_TYPE_INPT) {
    value = inpsel[cur0];
  } else if (scrType == SCR_TYPE_SCLE) {
    fmaxdisp = inpsel[cur0];
  } else if (scrType == SCR_TYPE_DMNU) {
    String item0 = "item0_v" + String(cur0);
    String item1 = "item1_v" + String(cur1);
    int ch_0 = (*doc)[item0.c_str()];
    int ch_1 = (*doc)[item1.c_str()];
    value = ch_1 << 8 | ch_0;
  }
  next_id = backScreen;
  updateResponse(label, value, cur0, cur1, next_id);
  interrupts();
}


/* "NEXT" Operation */
static void updateB3() {
  if (nextScreen[0] < 0) return;
  noInterrupts();
  scrChange = true;

  char *label = (*doc)["res_label"];
  int value;
  int next_id;
  
  if (scrType == SCR_TYPE_PAGE || scrType == SCR_TYPE_MENU) {
    String item = "item" + String(cur0) + "_v";
    value = (*doc)[item.c_str()];
  } else if (scrType == SCR_TYPE_INPT) {
    value = inpsel[cur0];
  } else if (scrType == SCR_TYPE_SCLE) {
    fmaxdisp = inpsel[cur0];
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
  interrupts();
}


/* Screen operation related functions */
static bool updateScreen() {
  bool tmp = scrChange;
  scrChange = false;
  return tmp;
}


/* this function is called when the page changes */
void clearScreen(DynamicJsonDocument* jdoc) {
  scrType  = (*jdoc)["type"];
  scrChange = false;
  item0_num = 0;
  inp_num = 0;
  cur0 = 0;
  cur1 = 0;
  backScreen = -1;
  doc = jdoc;
#ifdef DEMO_SETTING
  fftamp0  = FFT_MAX_AMP-85*FFT_AMP_STEP;
  wavamp0  = WAV_MAX_AMP;
  fftamp1  = FFT_MAX_AMP-85*FFT_AMP_STEP;
  wavamp1  = WAV_MAX_AMP;
  orbitamp = ORBIT_MIN_AMP;
#else
  fftamp0  = FFT_MIN_AMP;
  wavamp0  = WAV_MIN_AMP;
  fftamp1  = FFT_MIN_AMP;
  wavamp1  = WAV_MIN_AMP;
  orbitamp = ORBIT_MIN_AMP;
  dbvdisp0 = FFT_DBV_INIT;
  bdBVDisplay = false;
  bLogDisplay = false;
#endif
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
void buildTitle(DynamicJsonDocument* doc) {
  char* title = (*doc)["title"];
  putText(TITLE_SIDE, TITLE_HEAD, title, ILI9341_YELLOW, 2);
  putHorizonLine(TITLE_DECO_LINE, ILI9341_YELLOW);  
}


/* Building a sign of application selected */
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


/* Building a input dialog box */
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


/* Building items on a menu screen */
void buildMenu(DynamicJsonDocument* doc) {
  item0_num = (*doc)["item_num"];
  cur0 = (*doc)["cur0"];
#ifdef SCR_DEBUG
  MPLog("SELECTION ITEM : %d\n", item0_num);
#endif
  for (int i = 0; i < item0_num; ++i) {
    String item = "item" + String(i);
    char* item_c = (*doc)[item.c_str()];
    putText(MENU_ITEM_SIDE, MENU_ITEM0_HEAD+40*i, item_c, ILI9341_WHITE, 2);
  }
  curOperation(cur0);
}


/* Building items on a double menu screen */
void buildDMenu(DynamicJsonDocument* doc) {
  item0_num = (*doc)["item0_num"];
  item1_num = (*doc)["item1_num"];
  cur0 = (*doc)["cur0"];
  cur1 = (*doc)["cur1"];
#ifdef SCR_DEBUG
  MPLog("SELECTION ITEM0 : %d\n", item0_num);
  MPLog("SELECTION ITEM1 : %d\n", item1_num);
#endif
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
void buildMonitor(DynamicJsonDocument* doc) {
  tft.drawRect(MON_BOX_SIDE, MON_BOX0_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  tft.drawRect(MON_BOX_SIDE, MON_BOX1_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  tft.drawRect(MON_BOX_SIDE, MON_BOX2_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  putText(MON_UNIT_SIDE, MON_ELEM0_HEAD, String("m/s2"), ILI9341_WHITE, 2);
  putText(MON_UNIT_SIDE, MON_ELEM1_HEAD, String("mm/s"), ILI9341_WHITE, 2);
  putText(MON_UNIT_SIDE, MON_ELEM2_HEAD, String("um"), ILI9341_WHITE, 2);
}


/* Building the screen for 2 graph applications */
void build2WayGraph(DynamicJsonDocument* doc) {
  tft.drawRect(FFT_BOX_SIDE, FFT_BOX0_HEAD, FFT_GRAPH_WIDTH+FFT_MARGIN+1, FFT_GRAPH_HEIGHT+FFT_MARGIN+1, ILI9341_YELLOW);  
  tft.drawRect(FFT_BOX_SIDE, FFT_BOX1_HEAD, FFT_GRAPH_WIDTH+FFT_MARGIN+1, FFT_GRAPH_HEIGHT+FFT_MARGIN+2, ILI9341_YELLOW); 
}


/* Building the screen for the orbit graph application */
void buildSpace(DynamicJsonDocument* doc) {
  // the graph is drawn by applications
}

/* Building a sign of selected channel */
void buildChStatus(DynamicJsonDocument* doc) {
  int ch_disp = (*doc)["ch_disp"];
#ifdef SCR_DEBUG
  MPLog("CHANNEL DISPLAY : %d\n", ch_disp);
#endif
  for (int i = 0; i < ch_disp; ++i) {
    String ch = "ch" + String(i);
    int8_t ich = (*doc)[ch.c_str()];
    String sch = "CH" + String(ich);
    tft.drawRect(DISP_CH_SIDE-10, (DISP_CH0_HEAD + 40*i)-10, 50, 30, ILI9341_GREEN);
    putText(DISP_CH_SIDE, DISP_CH0_HEAD+40*i, sch, ILI9341_GREEN, 2);
  }
}


/* Building buttons */
void buildButton(DynamicJsonDocument* doc) {
  char* b0 = (*doc)["B0"];
  char* b1 = (*doc)["B1"];
  char* b2 = (*doc)["B2"];
  char* b3 = (*doc)["B3"];
#ifdef SCR_DEBUG
  MPLog("Build Buttons\n");
#endif 
  putHorizonLine(BUTTON_DECO_LINE, ILI9341_YELLOW);  
  putText(BUTTON0_SIDE, BUTTON_HEAD, b0, ILI9341_CYAN, 2);
  putText(BUTTON1_SIDE, BUTTON_HEAD, b1, ILI9341_CYAN, 2);
  putText(BUTTON2_SIDE, BUTTON_HEAD, b2, ILI9341_CYAN, 2);
  putText(BUTTON3_SIDE, BUTTON_HEAD, b3, ILI9341_CYAN, 2);  
}


/* Building a connection to a next page and a back page */
void buildNextBackConnection(DynamicJsonDocument* doc) {
  if (scrType == SCR_TYPE_PAGE) {
    for (int i = 0; i < item0_num; ++i) {
      String next = String("next") + String(i);
      nextScreen[i] = (*doc)[next.c_str()];
#ifdef SCR_DEBUG
      MPLog("buildConn: %d\n", nextScreen[i]);
#endif
    }
  } else {
    nextScreen[0] = (*doc)["next0"];
#ifdef SCR_DEBUG
    MPLog("buildConn: %d\n", nextScreen[0]);
#endif
  }
  backScreen = (*doc)["back"];
}
