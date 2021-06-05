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
#ifdef SCR_DEBUG
  MPLog("scr_type: %d\n", cur_scrType);
#endif
  clearScreen(doc);
  buildTitle(doc);
  buildAppSign(doc);
  switch (cur_scrType) {
  case SCR_TYPE_PAGE: 
  case SCR_TYPE_MENU:
#ifdef SCR_DEBUG
    MPLog("Building Page or Menu Screen\n");
#endif
    buildMenu(doc); 
    buildChStatus(doc);
    break;
  case SCR_TYPE_INPT: 
  case SCR_TYPE_SCLE:
#ifdef SCR_DEBUG
    MPLog("Building Input Screen\n");
#endif
    buildInput(doc); 
    buildChStatus(doc);
    break;
  case SCR_TYPE_DMNU:
#ifdef SCR_DEBUG
    MPLog("Building Double Menu Screen\n");
#endif
    buildDMenu(doc);
    buildChStatus(doc);
    break;  
  case SCR_TYPE_MNTR:
#ifdef SCR_DEBUG
    MPLog("Building Monitor Screen\n");
#endif
    buildMonitor(doc);
    startApplication(APP_ID_MONDATA);
    buildChStatus(doc);
    break;
  case SCR_TYPE_WVFT:
#ifdef SCR_DEBUG
    MPLog("Building FFT(Linear)-WAV Screen\n");
#endif
    bLogDisplay = false;
    build2WayGraph(doc);
    startApplication(APP_ID_WAV_FFT);
    break;
  case SCR_TYPE_WFLG:
#ifdef SCR_DEBUG
    MPLog("Building FFT(Log)-WAV Screen\n");
#endif
    bLogDisplay = true;
    build2WayGraph(doc);
    startApplication(APP_ID_WAV_FFT);
    break;
  case SCR_TYPE_FFT2:
#ifdef SCR_DEBUG
    MPLog("Building FFT-FFT(Linear) Screen\n");
#endif
    bLogDisplay = false;
    build2WayGraph(doc);
    startApplication(APP_ID_FFT_FFT);
    break;
  case SCR_TYPE_FTLG:
#ifdef SCR_DEBUG
    MPLog("Building FFT-FFT(Log) Screen\n");
#endif
    bLogDisplay = true;
    build2WayGraph(doc);
    startApplication(APP_ID_FFT_FFT);
    break;
  case SCR_TYPE_ORBT:
#ifdef SCR_DEBUG
    MPLog("Building Orbit Screen\n");
#endif
    buildSpace(doc);
    startApplication(APP_ID_ORBITDT);
    break;
  case SCR_TYPE_FLTR:
#ifdef SCR_DEBUG
    MPLog("Building RAW-FIL Screen\n");
#endif
    build2WayGraph(doc);
    startApplication(APP_ID_RAW_FIL);
    break;
  case SCR_TYPE_WVWV:
#ifdef SCR_DEBUG
    MPLog("Building WAV-WAV Screen\n");
#endif
    build2WayGraph(doc);
    startApplication(APP_ID_WAV_WAV);
    break;
  }
  buildButton(doc);
  buildNextBackConnection(doc);
}

/* appllication life cycle control */
static bool bAppRunning = false;
static bool bDataReceived = false;
static int  app_id = 0;

void startApplication(int appid) {
  bAppRunning = true;
  bDataReceived = true;
  app_id = appid;
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
