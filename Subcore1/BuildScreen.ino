#include "AppScreen.h"

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

struct Response response;
static int cur_scrType = -1;


/* LCD and Buttons setup */
void hardwareSetup() {
  /* Button settings */
  pinMode(B0, INPUT_PULLUP);
  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(B0) ,updateB0 ,RISING);   
  attachInterrupt(digitalPinToInterrupt(B1) ,updateB1 ,RISING);
  attachInterrupt(digitalPinToInterrupt(B2) ,updateB2 ,RISING);   
  attachInterrupt(digitalPinToInterrupt(B3) ,updateB3 ,RISING); 
   
  /* LCD settings */
  tft.begin(40000000);
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  memset(&response, 0, sizeof(struct Response));
}


/* Building LCD Screen */
void BuildScreen(DynamicJsonDocument *doc) {
  cur_scrType = (*doc)["type"];
  MPLog("scr_type: %d\n", cur_scrType);
  ClearScreen(doc);
  buildTitle(doc);
  buildAppSign(doc);
  switch (cur_scrType) {
  case SCR_TYPE_PAGE: 
  case SCR_TYPE_MENU:
    MPLog("Building Page or Menu Screen\n");
    buildMenu(doc); 
    buildChStatus(doc);
    break;
  case SCR_TYPE_INPT: 
  case SCR_TYPE_SCLE:
    MPLog("Building Input Screen\n");
    buildInput(doc); 
    buildChStatus(doc);
    break;
  case SCR_TYPE_DMNU:
    MPLog("Building Double Menu Screen\n");
    buildDMenu(doc);
    buildChStatus(doc);
    break;  
  case SCR_TYPE_MNTR:
    MPLog("Building Monitor Screen\n");
    buildMonitor(doc);
    startApplication(APP_ID_MONDATA);
    buildChStatus(doc);
    break;
  case SCR_TYPE_WVFT:
    MPLog("Building FFT(Linear)-WAV Screen\n");
    bLogDisplay = false;
    build2WayGraph(doc);
    startApplication(APP_ID_WAV_FFT);
    break;
  case SCR_TYPE_WFLG:
    MPLog("Building FFT(Log)-WAV Screen\n");
    bLogDisplay = true;
    build2WayGraph(doc);
    startApplication(APP_ID_WAV_FFT);
    break;
  case SCR_TYPE_FFT2:
    MPLog("Building FFT-FFT(Linear) Screen\n");
    bLogDisplay = false;
    build2WayGraph(doc);
    startApplication(APP_ID_FFT_FFT);
    break;
  case SCR_TYPE_FTLG:
    MPLog("Building FFT-FFT(Log) Screen\n");
    bLogDisplay = true;
    build2WayGraph(doc);
    startApplication(APP_ID_FFT_FFT);
    break;
  case SCR_TYPE_ORBT:
    MPLog("Building Orbit Screen\n");
    buildSpace(doc);
    startApplication(APP_ID_ORBITDT);
    break;
  case SCR_TYPE_FLTR:
    MPLog("Building RAW-FIL Screen\n");
    build2WayGraph(doc);
    startApplication(APP_ID_RAW_FIL);
    break;
  case SCR_TYPE_WVWV:
    MPLog("Building WAV-WAV Screen\n");
    build2WayGraph(doc);
    startApplication(APP_ID_WAV_WAV);
    break;
  case SCR_TYPE_WFDB:
    MPLog("Building FFT(dBV)-WAV Screen\n");
    bdBVDisplay = true;
    bLogDisplay = true;
    build2WayGraph(doc);
    startApplication(APP_ID_WAV_FFT);
    break;
  case SCR_TYPE_FFDB:
    MPLog("Building FFT(dBV)-WAV Screen\n");
    bdBVDisplay = true;
    bLogDisplay = true;
    build2WayGraph(doc);
    startApplication(APP_ID_FFT_FFT);
    break;
  case SCR_TYPE_SPCT:
    MPLog("Building Spectrogram Screen\n");
    buildSpectroGraph(doc);
    startApplication(APP_ID_SPECTRO);
    break;
    
  }
  buildButton(doc);
  buildNextBackConnection(doc);
}



/* Screen operation related functions */
static bool updateScreen() {
  bool tmp = scrChange;
  scrChange = false;
  return tmp;
}


/* this function is called when the page changes */
void ClearScreen(DynamicJsonDocument* jdoc) {
  scrType  = (*jdoc)["type"];
  scrChange = false;
  item0_num = 0;
  inp_num = 0;
  cur0 = 0;
  cur1 = 0;
  backScreen = -1;
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
  dbvdisp1 = FFT_DBV_INIT;
  bdBVDisplay = false;
  bLogDisplay = false;
#endif
  plotscale0_done = false;
  plotscale1_done = false;
  plotscalespc_done = false;
  memset(inpsel, 0, sizeof(uint16_t)*100);
  memset(nextScreen, -1, sizeof(int)*5);
  memset(&response, 0, sizeof(struct Response));
  memcpy(response.label, "non", sizeof(char)*3);
  
  freeScreenMemories();

  tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ILI9341_BLACK); 
}




/* appllication life cycle control */
static bool bAppRunning = false;
static bool bDataReceived = false;
static int  app_id = 0;

void startApplication(int appid) {
  bAppRunning = true;
  bDataReceived = true;
  app_id = appid;
  switch (appid) {
  case APP_ID_MONDATA:
    break;
  case APP_ID_WAV_FFT:
  case APP_ID_FFT_FFT:
  case APP_ID_WAV_WAV:
  case APP_ID_RAW_FIL:
    signalScreenMemoryAllocation(); 
    break;
  case APP_ID_ORBITDT:
    orbitScreenMemoryAllocation();
    break;
  case APP_ID_SPECTRO:
    spectroScreenMemoryAllocation();
    break;
  }
}

int isAppRunning() {
  return app_id;
}

void stopApplication() {
  bAppRunning = false;
  bDataReceived = false;
  app_id = 0;
}

bool isDataReceived() {
  return bDataReceived;
}

void requestedData() {
  bDataReceived = false;
}

bool receivedData() {
  bDataReceived = true;
}


/* Response related functions */
/* get a response handled by the interrupt functions on buttons */
void getResponse(struct Response *res) {
  /* this function is called in the main loop, so it needs to avoid a conflict with the interrupt function */
  pthread_mutex_lock(&m);
  memcpy(res, &response, sizeof(response));
  pthread_mutex_unlock(&m);
}


/* update response processed in the interrupt function on buttons */
void updateResponse(char* label, int value, int cur0, int cur1, int next_id) {

  /* this function is called in a button interrupt function, so block it */
  pthread_mutex_lock(&m);
  if (sizeof(response.label) >= sizeof(label)) 
    memcpy(response.label, label, sizeof(label));
  else
    memcpy(response.label, "non", sizeof("non"));

  response.value   = value;
  response.cur0    = cur0;
  response.cur1    = cur1;
  response.next_id = next_id;
  pthread_mutex_unlock(&m);
  
#ifdef MP_DEBUG
  MPLog("Response.label %s\n", response.label);
  MPLog("Response.value %d\n", response.value);
  MPLog("Response.cur0  %d\n", response.cur0);
  MPLog("Response.cur1  %d\n", response.cur1);
  MPLog("Response.next_id  %d\n", response.next_id);
#endif
}



/* helper function to avoid retype */
void signalScreenMemoryAllocation() {
  frameBuf = (uint16_t*)malloc(sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
  graphDataBuf = (float*)malloc(sizeof(float)*FFT_GRAPH_WIDTH);
  memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
  memset(graphDataBuf, 0, sizeof(float)*FFT_GRAPH_WIDTH);  
}

void orbitScreenMemoryAllocation() {
  orbitBuf = (uint16_t*)malloc(sizeof(uint16_t)*ORBIT_SIZE*ORBIT_SIZE);
  memset(frameBuf, 0, sizeof(uint16_t)*ORBIT_SIZE*ORBIT_SIZE);
}

void spectroScreenMemoryAllocation() {
  spcFrameBuf = (uint16_t*)malloc(sizeof(uint16_t)*SPC_GRAPH_HEIGHT*SPC_GRAPH_WIDTH);
  spcDataBuf = (float*)malloc(sizeof(float)*SPC_GRAPH_HEIGHT);
  memset(spcFrameBuf, 0, sizeof(uint16_t)*SPC_GRAPH_HEIGHT*SPC_GRAPH_WIDTH);
  memset(spcDataBuf, 0, sizeof(float)*SPC_GRAPH_HEIGHT);
}

void freeScreenMemories() {
  if (frameBuf != '\0') {
    memset(frameBuf, 0, sizeof(uint16_t)*FRAME_WIDTH*FRAME_HEIGHT);
    free(frameBuf); frameBuf = '\0';
  }
  if (spcFrameBuf != '\0') {
    memset(spcFrameBuf, 0, sizeof(uint16_t)*SPC_GRAPH_WIDTH*SPC_GRAPH_HEIGHT);
    free(spcFrameBuf); spcFrameBuf = '\0';    
  }
  if (graphDataBuf != '\0') {
    memset(graphDataBuf, 0, sizeof(float)*FFT_GRAPH_WIDTH);
    free(graphDataBuf); graphDataBuf = '\0';
  }
  if (spcDataBuf != '\0') {
    memset(spcDataBuf, 0, sizeof(float)*SPC_GRAPH_HEIGHT);
    free(spcDataBuf); spcDataBuf = '\0';
  }
  if (orbitBuf != '\0') {
    memset(orbitBuf, 0, sizeof(uint16_t)*ORBIT_SIZE*ORBIT_SIZE);
    orbitBuf = (uint16_t*)malloc(sizeof(uint16_t)*ORBIT_SIZE*ORBIT_SIZE);
    free(orbitBuf); orbitBuf = '\0';
  }
}
