#include "AppScreen.h"


pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

struct Response response;
static int cur_scrType = -1;

void hardwareSetup() {
  pinMode(B0, INPUT_PULLUP);
  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(B0) ,updateB0 ,RISING);   
  attachInterrupt(digitalPinToInterrupt(B1) ,updateB1 ,RISING);
  attachInterrupt(digitalPinToInterrupt(B2) ,updateB2 ,RISING);   
  attachInterrupt(digitalPinToInterrupt(B3) ,updateB3 ,RISING);  
  tft.begin(40000000);
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  memset(&response, 0, sizeof(struct Response));
}

static void BuildScreen(DynamicJsonDocument *doc) {
  cur_scrType = (*doc)["type"];
  MPLog("scr_type: %d\n", cur_scrType);
  clearScreen(doc);
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
    startSensorApp();
    buildChStatus(doc);
    break;
  case SCR_TYPE_WVFT:
    MPLog("Building FFT-WAV Screen\n");
    build2WayGraph(doc);
    startFftWavApp();
    break;
  case SCR_TYPE_FFT2:
    MPLog("Building FFT-FFT Screen\n");
    build2WayGraph(doc);
    startFftFftApp();
    break;
  }
  buildButton(doc);
  buildNextBackConnection(doc);
}

/* run Sebsor application */
static bool bSensorAppRunning = false;
static bool bReceivedSensorData = false;
void startSensorApp() {
  bSensorAppRunning = true;
  bReceivedSensorData = true;
}

bool isSensorAppRunning() {
  return bSensorAppRunning;
}

void stopSensorApp() {
  bSensorAppRunning = false;
}

bool isSensorDataReceived() {
  return bReceivedSensorData;
}

bool receivedSensorData() {
  bReceivedSensorData = true;
}

void requestSensorData() {
  bReceivedSensorData = false;
}


/* run FFT-WAV application */
static bool bFftWavAppRunning = false;
static bool bReceivedFftWavData = false;
void startFftWavApp() {
  bFftWavAppRunning = true;
  bReceivedFftWavData = true;
}

bool isFftWavAppRunning() {
  return bFftWavAppRunning;
}

void stopFftWavApp() {
  bFftWavAppRunning = false;
}

bool isFftWavDataReceived() {
  return bReceivedFftWavData;
}

bool receivedFftWavData() {
  bReceivedFftWavData = true;
}

void requestFftWavData() {
  bReceivedFftWavData = false;
}


/* run FFT-FFT application */
static bool bFftFftAppRunning = false;
static bool bReceivedFftFftData = false;
void startFftFftApp() {
  bFftFftAppRunning = true;
  bReceivedFftFftData = true;
}

bool isFftFftAppRunning() {
  return bFftFftAppRunning;
}

void stopFftFftApp() {
  bFftFftAppRunning = false;
}

bool isFftFftDataReceived() {
  return bReceivedFftFftData;
}

bool receivedFftFftData() {
  bReceivedFftFftData = true;
}

void requestFftFftData() {
  bReceivedFftFftData = false;
}


/*** Force Halt ***/
static void stopApplication() {
  bSensorAppRunning = false;
  bFftWavAppRunning = false;
  bFftFftAppRunning = false;
  bReceivedSensorData = false;
  bReceivedFftWavData = false;
  bReceivedFftFftData = false;
}

/* Response related functions */
void getResponse(struct Response *res) {
  /* this function is called in the main loop, so it needs to avoid a conflict with the interrupt function */
  pthread_mutex_lock(&m);
  memcpy(res, &response, sizeof(response));
  pthread_mutex_unlock(&m);
}

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
  
  MPLog("Response.label %s\n", response.label);
  MPLog("Response.value %d\n", response.value);
  MPLog("Response.cur0  %d\n", response.cur0);
  MPLog("Response.cur1  %d\n", response.cur1);
  MPLog("Response.next_id  %d\n", response.next_id);
}
