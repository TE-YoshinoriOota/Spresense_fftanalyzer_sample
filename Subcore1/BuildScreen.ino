#include "AppScreen.h"

#define SCR_TYPE_PAGE  (0)
#define SCR_TYPE_MENU  (1)
#define SCR_TYPE_INPT  (2)
#define SCR_TYPE_DMNU  (3)
#define SCR_TYPE_MNTR  (4)
#define SCR_TYPE_FFT2  (5)
#define SCR_TYPE_FFT4  (6)
#define SCR_TYPE_ORBT  (7)

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
    buildMenu(doc); break;
  case SCR_TYPE_INPT: 
    buildInput(doc); break;
  case SCR_TYPE_DMNU:
    buildDMenu(doc); break;  
  case SCR_TYPE_MNTR:
    buildMonitor(doc);
    startSensorApp();
    break;
  case SCR_TYPE_FFT2:
    build2WayGraph(doc);
    startFftApp();
  }
  buildChStatus(doc);
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



/* run FFT application */
static bool bFftAppRunning = false;
static bool bReceivedFftData = false;
void startFftApp() {
  bFftAppRunning = true;
  bReceivedFftData = true;
}

bool isFftAppRunning() {
  return bFftAppRunning;
}

void stopFftApp() {
  bFftAppRunning = false;
}

bool isFftDataReceived() {
  return bReceivedFftData;
}

bool receivedFftData() {
  bReceivedFftData = true;
}

void requestFftData() {
  bReceivedFftData = false;
}


/*** Force Halt ***/
static void stopApplication() {
  bSensorAppRunning = false;
  bFftAppRunning = false;
}

/* Response related functions */
void getResponse(struct Response *res) {
  pthread_mutex_lock(&m);
  memcpy(res, &response, sizeof(response));
  pthread_mutex_unlock(&m);
}

void updateResponse(char* label, int value, int cur0, int cur1, int next_id) {
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
