#include "AppScreen.h"

// #define DEMO_SETTING

DynamicJsonDocument* doc;


/* the magnification of the graphs:
 * steps by 'step' from 'min' up to 'lmax', then by highAmpSteps up to 1.0 mV full scale */
static int ampUp(int a, int step, int lmax) {
  if (a < lmax) return (a + step > lmax) ? lmax : a + step;
  const int steps = sizeof(highAmpSteps)/sizeof(highAmpSteps[0]);
  for (int i = 0; i < steps; ++i) {
    if (highAmpSteps[i] > a) return highAmpSteps[i];
  }
  return highAmpSteps[steps-1];
}

static int ampDown(int a, int step, int min, int lmax) {
  if (a <= lmax) return (a - step < min) ? min : a - step;
  const int steps = sizeof(highAmpSteps)/sizeof(highAmpSteps[0]);
  for (int i = steps-1; i >= 0; --i) {
    if (highAmpSteps[i] < a) return (highAmpSteps[i] > lmax) ? highAmpSteps[i] : lmax;
  }
  return lmax;
}


/* the cursor saved in the page file: 0 if it is out of the items
 * (a cursor out of the items selects a wrong next page, e.g. AA020.txt) */
static int validCursor(int cur, int num) {
  return (cur < 0 || cur >= num) ? 0 : cur;
}


/* graph screens where B0 and B1 are - and +: pressing them together saves the data */
static bool isSaveScreen() {
  switch (scrType) {
  case SCR_TYPE_WVFT:
  case SCR_TYPE_FFT2:
  case SCR_TYPE_FLTR:
  case SCR_TYPE_WVWV:
  case SCR_TYPE_WFLG:
  case SCR_TYPE_FTLG:
  case SCR_TYPE_WFDB:
  case SCR_TYPE_FFDB:
  case SCR_TYPE_SPCT:
  case SCR_TYPE_LSJU:
    return true;
  }
  return false;
}

/* - and + pressed together requests saving the data instead of changing the magnification.
 * - and + work on releasing, so the other button is checked at that time (as BACK + NEXT).
 * returns true if the release belongs to the combination (do not change the magnification) */
static bool releaseSaveCombo(int other) {
  if (!isSaveScreen()) return false;
  if (bSaveComboPressed) {
    if (digitalRead(B0) == HIGH && digitalRead(B1) == HIGH) {
      bSaveComboPressed = false;
      saveComboEndMs = millis();
    }
    return true;
  }
  if (millis() - saveComboEndMs < COMBO_GUARD_MS) return true;  /* chattering */
  if (digitalRead(other) == LOW) {
    bSaveComboPressed = true;
    save_request = SAVE_REQUEST;
    return true;
  }
  return false;
}

/* called in the main loop: both buttons held down */
void checkSaveCombo() {
  if (bSaveComboPressed || doc == NULL || !isSaveScreen()) return;
  if (digitalRead(B0) == LOW && digitalRead(B1) == LOW) {
    noInterrupts();
    bSaveComboPressed = true;
    save_request = SAVE_REQUEST;
    interrupts();
  }
}


/* "DOWN" Operation */
static void updateB0() {
  /* no page yet: the edge of the pull-up when attached comes before the first page */
  if (bCapturing || doc == NULL) return;
  if (releaseSaveCombo(B1)) return;
  noInterrupts();
  switch (scrType) {
  case SCR_TYPE_PAGE: 
  case SCR_TYPE_MENU:
    if (++cur0 >= item0_num) cur0 = (item0_num - 1);
    curOperation(cur0);
    break;
  case SCR_TYPE_INPT:  
  case SCR_TYPE_SCLE:
    if (--cur0 < 0) cur0 = 0;
    inpOperation(cur0);
    break;
  case SCR_TYPE_DMNU:
    if (++cur0 >= item0_num) cur0 = 0;
    curOperation(cur0, cur1);
    break; 
  case SCR_TYPE_WVFT:
  case SCR_TYPE_LSJU:
  case SCR_TYPE_FLTR:
  case SCR_TYPE_WVWV:
  case SCR_TYPE_WFLG:
  case SCR_TYPE_WFDB:
  case SCR_TYPE_FFT2:
  case SCR_TYPE_FTLG:
  case SCR_TYPE_SPCT:
    pthread_mutex_lock(&mtx);
    spcamp = ampDown(spcamp, SPC_AMP_STEP, SPC_MIN_AMP, SPC_MAX_AMP);
    amp = ampDown(amp, AMP_STEP, AMP_MIN, AMP_MAX);
    pthread_mutex_unlock(&mtx);
    break;
  case SCR_TYPE_FFDB:
    pthread_mutex_lock(&mtx);
    dbvoffset -= DBV_OFFSET_STEP;
    if (dbvoffset <= DBV_OFFSET_MIN) dbvoffset = DBV_OFFSET_MIN;
    pthread_mutex_unlock(&mtx);
    break;
  case SCR_TYPE_RECM:
    /* REC/STOP: the main loop handles this request */
    /* no mutex here: blocking on a mutex held by the main loop hangs the interrupt */
    rec_toggle_request = true;
    break;
  }
  interrupts();
}


/* "UP" Operation */
static void updateB1() {
  /* no page yet: the edge of the pull-up when attached comes before the first page */
  if (bCapturing || doc == NULL) return;
  if (releaseSaveCombo(B0)) return;
  noInterrupts();
  switch (scrType) {
  case SCR_TYPE_PAGE: 
  case SCR_TYPE_MENU:
    if (--cur0 < 0) cur0 = 0;
    curOperation(cur0);
    break;
  case SCR_TYPE_INPT:  
  case SCR_TYPE_SCLE:
    if (++cur0 >= inp_num) cur0 = (inp_num-1);
    inpOperation(cur0);  
    break;
  case SCR_TYPE_DMNU:
    if (++cur1 >= item1_num) cur1 = 0;
    curOperation(cur0, cur1);
    break;   
  case SCR_TYPE_LSJU:
  case SCR_TYPE_FLTR:
  case SCR_TYPE_WVWV:
  case SCR_TYPE_WFLG:
  case SCR_TYPE_WFDB:
  case SCR_TYPE_WVFT:
  case SCR_TYPE_FTLG:
  case SCR_TYPE_FFT2:
  case SCR_TYPE_SPCT:
    pthread_mutex_lock(&mtx);
    spcamp = ampUp(spcamp, SPC_AMP_STEP, SPC_MAX_AMP);
    amp = ampUp(amp, AMP_STEP, AMP_MAX);
    pthread_mutex_unlock(&mtx);
    break;
  case SCR_TYPE_FFDB:
    pthread_mutex_lock(&mtx);
    dbvoffset += DBV_OFFSET_STEP;
    if (dbvoffset <= DBV_OFFSET_INIT) dbvoffset = DBV_OFFSET_INIT;
    pthread_mutex_unlock(&mtx);
    break;
  case SCR_TYPE_RECM: {
    /* cycle the magnification: 1 -> 2 -> 5 -> ... -> 500 -> 1 */
    /* no mutex here for the same reason as B0 (a single int write is atomic) */
    const int steps = sizeof(recAmpSteps)/sizeof(recAmpSteps[0]);
    int next = recAmpSteps[0];
    for (int i = 0; i < steps; ++i) {
      if (recAmpSteps[i] > amp) { next = recAmpSteps[i]; break; }
    }
    amp = next;
    break;
  }
  }
  interrupts();
}


/* BACK + NEXT pressed together requests the screen capture instead of changing the page.
 * BACK and NEXT work on releasing, so the other button is checked at that time.
 * checkCaptureCombo() in the main loop also finds both buttons held down.
 * returns true if the release belongs to the combination (do not change the page) */
static bool releaseCaptureCombo(int other) {
  if (bComboPressed) {
    if (digitalRead(B2) == HIGH && digitalRead(B3) == HIGH) {
      bComboPressed = false;
      comboEndMs = millis();
    }
    return true;
  }
  if (millis() - comboEndMs < COMBO_GUARD_MS) return true;  /* chattering */
  if (digitalRead(other) == LOW) {
    bComboPressed = true;
    if (!bCapturing) capture_request = true;
    return true;
  }
  return false;
}

/* called in the main loop */
void checkCaptureCombo() {
  if (bComboPressed) return;
  if (digitalRead(B2) == LOW && digitalRead(B3) == LOW) {
    noInterrupts();
    bComboPressed = true;
    capture_request = true;
    interrupts();
  }
}


/* "BACK" Operation */
static void updateB2() {
  if (releaseCaptureCombo(B3)) return;
  /* no page yet: the edge of the pull-up when attached comes before the first page */
  if (bCapturing || doc == NULL) return;
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
  if (releaseCaptureCombo(B2)) return;
  /* no page yet: the edge of the pull-up when attached comes before the first page */
  if (bCapturing || doc == NULL) return;
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

#ifdef ENABLE_SAVE_BUTTON
/* "Save" Operation */
static void updateSave() {
  /* no page yet: the edge of the pull-up when attached comes before the first page */
  if (bCapturing || doc == NULL) return;
  /* no mutex: the interrupt must not wait for the main loop */
  noInterrupts();
  MPLog("Save button pressed!\n");
  save_request = SAVE_REQUEST;
  interrupts();
}
#endif


/* UP/DOWN operation related functions */
/* Item pitch for menu and page screens: narrow it to fit 5 items above the buttons */
static int menuTop = UI_MENU_TOP;     /* the top of the first row */
static int menuPitch = UI_MENU_PITCH;
static int menuRight = UI_ROW_RIGHT;
static int menuPrev0 = -1;               /* the selected rows drawn last time */
static int menuPrev1 = -1;

/* place the rows in the middle of the area between the title and the buttons */
static void layoutMenu(int num) {
  int area = UI_MENU_BOTTOM - UI_MENU_TOP;
  if (num < 1) num = 1;
  menuPitch = area / num;
  if (menuPitch > UI_MENU_PITCH) menuPitch = UI_MENU_PITCH;
  menuTop = UI_MENU_TOP + (area - menuPitch*num)/2;
  menuPrev0 = -1;
  menuPrev1 = -1;
}

/* a row of a menu: the selected row is filled with the amber bar */
static void drawMenuRow(int x0, int x1, int row, const char* text, bool selected) {
  int y = menuTop + menuPitch*row;
  int h = menuPitch - UI_ROW_MARGIN;
  int bg = selected ? UI_SEL_COLOR : ILI9341_BLACK;
  int fg = selected ? UI_SEL_TEXT  : UI_TEXT_COLOR;
  tft.fillRoundRect(x0, y, x1 - x0, h, 5, bg);
  String s = trimText(text);
  int base = y + h/2 + 6;   /* the capitals are 13 dots high */
  /* "name - unit": put the unit on a column so that the units line up */
  int sep = s.indexOf(" - ");
  if (sep > 0) {
    String unit = s.substring(sep + 3);
    unit.trim();
    s = s.substring(0, sep);
    s.trim();
    putTextFont(x0 + (x1 - x0)*UI_ROW_UNIT_POS/100, base, unit.c_str(), fg, &FreeSans9pt7b);
  }
  putTextFont(x0 + UI_ROW_TEXT_PAD, base, s.c_str(), fg, &FreeSans9pt7b);
}

/* Cursor operation for menu and page screens */
void curOperation(int cur) {
  if (cur < 0 || cur >= item0_num) return;
  if (menuPrev0 >= 0 && menuPrev0 != cur && menuPrev0 < item0_num) {
    String item = "item" + String(menuPrev0);
    drawMenuRow(UI_ROW_SIDE, menuRight, menuPrev0, (*doc)[item.c_str()], false);
  }
  String item = "item" + String(cur);
  drawMenuRow(UI_ROW_SIDE, menuRight, cur, (*doc)[item.c_str()], true);
  menuPrev0 = cur;
}


/* Input operation for input screens */
void inpOperation(int cur) {
  tft.fillRect(UI_INPUT_BOX_SIDE+2, UI_INPUT_BOX_HEAD+2, UI_INPUT_BOX_WIDTH-4, UI_INPUT_BOX_HEIGHT-4, ILI9341_BLACK);
  String v = String(inpsel[cur]);
  int w = textWidth(v.c_str(), &FreeSans12pt7b);
  putTextFont(UI_INPUT_BOX_SIDE + (UI_INPUT_BOX_WIDTH - w)/2, UI_INPUT_BOX_HEAD + UI_INPUT_BOX_HEIGHT/2 + 8
            , v.c_str(), UI_TEXT_COLOR, &FreeSans12pt7b);
}


/* Cursor operation for dual menu screens */
void curOperation(int cur0, int cur1) {
  if (menuPrev0 >= 0 && menuPrev0 != cur0 && menuPrev0 < item0_num) {
    String item = "item0_" + String(menuPrev0);
    drawMenuRow(UI_ROW_SIDE, UI_DMENU_SPLIT - UI_ROW_SIDE/2, menuPrev0, (*doc)[item.c_str()], false);
  }
  if (menuPrev1 >= 0 && menuPrev1 != cur1 && menuPrev1 < item1_num) {
    String item = "item1_" + String(menuPrev1);
    drawMenuRow(UI_DMENU_SPLIT + UI_ROW_SIDE/2, UI_ROW_RIGHT, menuPrev1, (*doc)[item.c_str()], false);
  }
  String item0 = "item0_" + String(cur0);
  String item1 = "item1_" + String(cur1);
  drawMenuRow(UI_ROW_SIDE, UI_DMENU_SPLIT - UI_ROW_SIDE/2, cur0, (*doc)[item0.c_str()], true);
  drawMenuRow(UI_DMENU_SPLIT + UI_ROW_SIDE/2, UI_ROW_RIGHT, cur1, (*doc)[item1.c_str()], true);
  menuPrev0 = cur0;
  menuPrev1 = cur1;
}


/* Building UI related functions */
/* Buidling Title on top of the screen */
void buildTitle(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  char* title = (*doc)["title"];
  tft.fillRect(0, 0, SCREEN_WIDTH, TITLE_DECO_LINE, UI_BAR_COLOR);
  putTextFont(TITLE_SIDE, UI_TITLE_BASE, trimText(title).c_str(), UI_TEXT_COLOR, &FreeSansBold9pt7b);
  putHorizonLine(TITLE_DECO_LINE, UI_ACCENT_COLOR);
}


/* Building a sign of application selected */
void buildAppSign(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  static char appChar[8][4] = {
    " ","SET","MNT","FFT","DUL","OBT","SPC","REC"
  };
  int16_t appid = (*doc)["id"];
  appid /= 100;
  if (appid != 0 && appid < 8) {
    /* green: microphones, magenta: the WAV file, red: the WAV file can't be played */
    int color = ILI9341_GREEN;
    const char* sign = appChar[appid];
    if (srcState == SRC_FILE || srcState == SRC_FILE_SOUND) {
      color = ILI9341_MAGENTA;
    } else if (srcState == SRC_FILE_ERROR) {
      color = ILI9341_RED;
      sign = "ERR";
    }
    tft.fillRoundRect(UI_BADGE_SIDE, UI_BADGE_HEAD, UI_BADGE_WIDTH, UI_BADGE_HEIGHT, 5, color);
    int w = textWidth(sign, &FreeSansBold9pt7b);
    putTextFont(UI_BADGE_SIDE + (UI_BADGE_WIDTH - w)/2, UI_BADGE_HEAD + UI_BADGE_HEIGHT/2 + 6
              , sign, ILI9341_BLACK, &FreeSansBold9pt7b);
  }
}


/* Building a input dialog box */
void buildInput(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  inp_num = (*doc)["inp_num"];
  for (int i = 0; i < inp_num; ++i) {
    inpsel[i] = (*doc)["input"][i];
  }
  String inp_title = (*doc)["inp_title"];
  String inp_unit = (*doc)["inp_unit"];
  cur0 = validCursor((*doc)["cur0"], inp_num);
  inp_title.trim();
  inp_unit.trim();
  putTextFont(UI_INPUT_BOX_SIDE, UI_INPUT_TITLE_BASE, inp_title.c_str(), UI_ACCENT_COLOR, &FreeSans12pt7b);
  tft.drawRoundRect(UI_INPUT_BOX_SIDE, UI_INPUT_BOX_HEAD, UI_INPUT_BOX_WIDTH, UI_INPUT_BOX_HEIGHT, 6, UI_ACCENT_COLOR);
  putTextFont(UI_INPUT_UNIT_SIDE, UI_INPUT_BOX_HEAD + UI_INPUT_BOX_HEIGHT/2 + 8
            , inp_unit.c_str(), UI_ACCENT_COLOR, &FreeSans12pt7b);
  inpOperation(cur0);
}


/* Building items on a menu screen */
void buildMenu(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  item0_num = (*doc)["item_num"];
  cur0 = validCursor((*doc)["cur0"], item0_num);
  int ch_disp = (*doc)["ch_disp"];
  menuRight = (ch_disp > 0) ? UI_ROW_RIGHT_CH : UI_ROW_RIGHT;
  layoutMenu(item0_num);
#ifdef SCR_DEBUG
  MPLog("SELECTION ITEM : %d\n", item0_num);
#endif
  for (int i = 0; i < item0_num; ++i) {
    if (i == cur0) continue;
    String item = "item" + String(i);
    drawMenuRow(UI_ROW_SIDE, menuRight, i, (*doc)[item.c_str()], false);
  }
  curOperation(cur0);
}


/* Building items on a double menu screen */
void buildDMenu(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  item0_num = (*doc)["item0_num"];
  item1_num = (*doc)["item1_num"];
  cur0 = validCursor((*doc)["cur0"], item0_num);
  cur1 = validCursor((*doc)["cur1"], item1_num);
  layoutMenu((item0_num > item1_num) ? item0_num : item1_num);
#ifdef SCR_DEBUG
  MPLog("SELECTION ITEM0 : %d\n", item0_num);
  MPLog("SELECTION ITEM1 : %d\n", item1_num);
#endif
  for (int i = 0; i < item0_num; ++i) {
    if (i == cur0) continue;
    String item = "item0_" + String(i);
    drawMenuRow(UI_ROW_SIDE, UI_DMENU_SPLIT - UI_ROW_SIDE/2, i, (*doc)[item.c_str()], false);
  }
  for (int i = 0; i < item1_num; ++i) {
    if (i == cur1) continue;
    String item = "item1_" + String(i);
    drawMenuRow(UI_DMENU_SPLIT + UI_ROW_SIDE/2, UI_ROW_RIGHT, i, (*doc)[item.c_str()], false);
  }
  curOperation(cur0, cur1);
}


/* Building the screen for the sensor monitor application */
void buildMonitor(DynamicJsonDocument* jdoc) {
  jdoc = doc;
  tft.drawRect(MON_BOX_SIDE, MON_BOX0_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  tft.drawRect(MON_BOX_SIDE, MON_BOX1_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  tft.drawRect(MON_BOX_SIDE, MON_BOX2_HEAD, MON_BOX_WIDTH, MON_BOX_HEIGHT, ILI9341_YELLOW);  
  putText(MON_UNIT_SIDE, MON_ELEM0_HEAD, String("m/s2"), ILI9341_WHITE, 2);
  putText(MON_UNIT_SIDE, MON_ELEM1_HEAD, String("mm/s"), ILI9341_WHITE, 2);
  putText(MON_UNIT_SIDE, MON_ELEM2_HEAD, String("um"), ILI9341_WHITE, 2);
}


/* Building the screen for 2 graph applications */
void build2WayGraph(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  tft.drawRect(FFT_BOX_SIDE, FFT_BOX0_HEAD, FFT_GRAPH_WIDTH+FFT_MARGIN+1, FFT_GRAPH_HEIGHT+FFT_MARGIN+1, ILI9341_YELLOW);  
  tft.drawRect(FFT_BOX_SIDE, FFT_BOX1_HEAD, FFT_GRAPH_WIDTH+FFT_MARGIN+1, FFT_GRAPH_HEIGHT+FFT_MARGIN+2, ILI9341_YELLOW); 
  /* the lower graph of CH2 is not drawn */
  if (bMonoFile) {
    putText(FFT_GRAPH_SIDE + (FFT_GRAPH_WIDTH - 9*12)/2, FFT_GRAPH1_HEAD + FFT_GRAPH_HEIGHT/2 - 8
          , "MONO FILE", ILI9341_RED, 2);
  }
}

/* Building the screen for 2 graph applications */
void buildSpectroGraph(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  tft.drawRect(SPC_BOX_SIDE, SPC_BOX_HEAD, SPC_GRAPH_WIDTH+SPC_MARGIN+1, SPC_GRAPH_HEIGHT+SPC_MARGIN+1, ILI9341_YELLOW);
  tft.drawRect(SPC_COL_SIDE, SPC_COL_HEAD, SPC_COL_WIDTH+SPC_MARGIN+1, SPC_COL_HEIGHT+SPC_MARGIN+2, ILI9341_YELLOW);
  int n = 0;
  for (int i = 0; i < 256; i+=2, ++n) {
    tft.drawLine(SPC_COL_SIDE+1,             SPC_COL_HEAD+1+SPC_COL_HEIGHT-n
               , SPC_COL_SIDE+SPC_COL_WIDTH, SPC_COL_HEAD+SPC_COL_HEIGHT-n
               , pseudoColors[i]);
  }
}

/* Building the screen for the recorder application */
/* the upper graph shows WAV and the lower graph shows FFT of the first channel */
void buildRecMonitor(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  build2WayGraph(doc);
  int ch_disp = (*doc)["ch_disp"];
  int ch0 = (*doc)["ch0"];
  int ch1 = (*doc)["ch1"];
  if (ch_disp > 1) sprintf(recChText, "CH%d+%d", ch0, ch1);
  else             sprintf(recChText, "CH%d", ch0);
  appRecStatus(true);
}


/* Building a sign of selected channel */
void buildChStatus(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  int ch_disp = (*doc)["ch_disp"];
#ifdef SCR_DEBUG
  MPLog("CHANNEL DISPLAY : %d\n", ch_disp);
#endif
  for (int i = 0; i < ch_disp; ++i) {
    String ch = "ch" + String(i);
    int8_t ich = (*doc)[ch.c_str()];
    String sch = "CH" + String(ich);
    int y = UI_MENU_TOP + 40*i;
    tft.drawRoundRect(UI_BADGE_SIDE, y, UI_BADGE_WIDTH, UI_BADGE_HEIGHT + 4, 5, ILI9341_GREEN);
    int w = textWidth(sch.c_str(), &FreeSansBold9pt7b);
    putTextFont(UI_BADGE_SIDE + (UI_BADGE_WIDTH - w)/2, y + (UI_BADGE_HEIGHT + 4)/2 + 6
              , sch.c_str(), ILI9341_GREEN, &FreeSansBold9pt7b);
  }
}


/* Building buttons */
void buildButton(DynamicJsonDocument* jdoc) {
  doc = jdoc;
  char* b0 = (*doc)["B0"];
  char* b1 = (*doc)["B1"];
  char* b2 = (*doc)["B2"];
  char* b3 = (*doc)["B3"];
#ifdef SCR_DEBUG
  MPLog("Build Buttons\n");
#endif 
  putHorizonLine(BUTTON_DECO_LINE, UI_FRAME_COLOR);
  putButtonLabel(0, b0);
  putButtonLabel(1, b1);
  putButtonLabel(2, b2);
  putButtonLabel(3, b3);
}


/* Building a connection to a next page and a back page */
void buildNextBackConnection(DynamicJsonDocument* jdoc) {
  doc = jdoc;
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


/* Building SD card error page */
void buildSDCardErrorPage() {
  /* tentative implementation */
  putText(80, 100, "Insert SD Card", ILI9341_RED, 2);
}
