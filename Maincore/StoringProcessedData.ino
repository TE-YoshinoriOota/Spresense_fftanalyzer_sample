#include "AppSystem.h"
#include <RTC.h>
#define TIME_HEADER 'T' // Header tag for serial time sync message

#ifdef ENABLE_SAVE_DATA


File myStorage0;
File myStorage1;
static uint16_t storage_id = 0;

char filename0[DATA_PATH_LEN] = {};
char filename1[DATA_PATH_LEN] = {};

void initStorage() {
  RTC.begin();  
  RtcTime compiledDateTime(__DATE__, __TIME__);
  RTC.setTime(compiledDateTime);

  MPLog("init RTC time!\n");
  if (Serial.available()) {
    if(Serial.find(TIME_HEADER)) {
      uint32_t pctime = Serial.parseInt();
      RtcTime rtc(pctime);
      RTC.setTime(rtc);
    }
  }
}

/*
 * The CSV files are made when the data is saved first on the screen (- and + pressed
 * together), not when the screen opens: no empty files for the screens without saving.
 * openStorage() keeps the application, storage_begin() makes the files.
 */
static uint8_t storage_appid = 0;
static bool    storage_opened = false;
static struct SaveStatus saveStatus;

void openStorage(uint8_t appid) {
  storage_appid = appid;
  storage_opened = false;
  memset(&saveStatus, 0, sizeof(saveStatus));
}

/* the result of the save requested by - and +: Subcore1 shows it on the screen */
/* send it before the data, or the screen capture may take this message as its reply */
void sendSaveStatus() {
  int ret = MP.Send(SID_REQ_SAVED, &saveStatus, SUBCORE);
  if (ret < 0) {
    MPLog("MP.Send error: %d\n", ret);
    while (true) {
      error_notifier(MP_ERROR);
    }
  }
}

/* called after a row is written: the state is ERROR if the files are not open */
static void storage_saved() {
  if (!storage_opened) {
    saveStatus.state = SAVE_STATE_ERROR;
    return;
  }
  saveStatus.state = SAVE_STATE_OK;
  ++saveStatus.rows;
  const char* base = strrchr(filename0, '/');
  base = (base == NULL) ? filename0 : base + 1;
  strncpy(saveStatus.filename, base, sizeof(saveStatus.filename)-1);
}

/* make the CSV files of the application: false if they can't be made */
static bool storage_begin() {
  if (storage_opened) return true;
  uint8_t appid = storage_appid;
  if (Serial.available()) {
    if(Serial.find(TIME_HEADER)) {
      uint32_t pctime = Serial.parseInt();
      RtcTime rtc(pctime);
      RTC.setTime(rtc);
    }
  }
  
  RtcTime rtc = RTC.getTime();

  /* find unused names: the number restarts at boot, and the old files must not be replaced */
  data_dir(CSV_DIR);
  bool single = false;
  for (int i = 0; i < 1000; ++i) {
    single = false;
    switch (appid) {
    case SID_REQ_MONDATA:
      sprintf(filename0, CSV_DIR "/m%02d%03d.csv", rtc.day(), storage_id);
      single = true;
      break;
    case SID_REQ_RAW_FIL:
      sprintf(filename0, CSV_DIR "/wi%02d%03d.csv", rtc.day(), storage_id);
      sprintf(filename1, CSV_DIR "/wo%02d%03d.csv", rtc.day(), storage_id);
      break;
    case SID_REQ_WAV_WAV:
      sprintf(filename0, CSV_DIR "/wa%02d%03d.csv", rtc.day(), storage_id);
      sprintf(filename1, CSV_DIR "/wb%02d%03d.csv", rtc.day(), storage_id);
      break;
    case SID_REQ_WAV_FFT:
    case SID_REQ_SPECTRO:
      sprintf(filename0, CSV_DIR "/wf%02d%03d.csv", rtc.day(), storage_id);
      sprintf(filename1, CSV_DIR "/ft%02d%03d.csv", rtc.day(), storage_id);
      break;
    case SID_REQ_FFT_FFT:
      sprintf(filename0, CSV_DIR "/fa%02d%03d.csv", rtc.day(), storage_id);
      sprintf(filename1, CSV_DIR "/fb%02d%03d.csv", rtc.day(), storage_id);
      break;
    }
    if (!theSD.exists(filename0) && (single || !theSD.exists(filename1))) break;
    if (++storage_id >= 1000) storage_id = 0;
  }

  if (theSD.exists(filename0)) theSD.remove(filename0);
  myStorage0 = theSD.open(filename0, FILE_WRITE);
  if (!myStorage0) {
    MPLog("Cannot open %s\n", filename0);
    return false;
  }
  MPLog("CSV: %s\n", filename0);
  storage_opened = true;
  
  if (single) return true;
  
  if (theSD.exists(filename1)) theSD.remove(filename1);
  myStorage1 = theSD.open(filename1, FILE_WRITE);
  if (!myStorage1) {
    MPLog("Cannot open %s\n", filename1);
    myStorage0.close();
    storage_opened = false;
    return false;
  }
  MPLog("CSV: %s\n", filename1);
  ++storage_id;
  if (storage_id == 1000) storage_id = 0;
  return true;
}


void saveData(struct SensorData* data) {
  if (data == '\0') {
    MPLog("<%s> Error: data is null!", __FUNCTION__);
    return;
  }
  if (!storage_begin()) {
    saveStatus.state = SAVE_STATE_ERROR;
    return;
  }
  if (!myStorage0) {
    MPLog("<%s> Error: storage not opened!", __FUNCTION__);
    return;
  }
}


void saveData(struct FftWavData* data) {
  if (data == '\0') {
    MPLog("<%s> Error: data is null!", __FUNCTION__);
    return;
  }
  if (!storage_begin()) {
    saveStatus.state = SAVE_STATE_ERROR;
    return;
  }
  if (!myStorage0 || !myStorage1) {
    MPLog("<%s> Error: storage not opened!", __FUNCTION__);
    return;
  }

  float* p_wav = data->pWav;
  float* p_fft = data->pFft;
  int len = data->len;
  float df = data->df;
  float dt = 1000/(df*len); /* 1000/sampling_rate */
  myStorage0.print(String(dt, 3) + ",");
  myStorage1.print(String(df, 3) + ",");
  for (int n = 0; n < len; ++n) {
    myStorage0.print(String(p_wav[n]*455.) + ",");
    myStorage1.print(String(p_fft[n]*455.) + ",");
  }
  myStorage0.println();
  myStorage1.println();
  myStorage0.flush();
  myStorage1.flush();
  storage_saved();
}

void saveData(struct FftFftData* data) {
  if (data == '\0') {
    MPLog("<%s> Error: data is null!", __FUNCTION__);
    return;
  }
  if (!storage_begin()) {
    saveStatus.state = SAVE_STATE_ERROR;
    return;
  }
  if (!myStorage0 || !myStorage1) {
    MPLog("<%s> Error: storage not opened!", __FUNCTION__);
    return;
  }
  
  float* p_fft = data->pFft;
  float* p_sfft = data->pSubFft;
  int len = data->len;
  float df = data->df;
  myStorage0.print(String(df, 3) + ",");
  myStorage1.print(String(df, 3) + ",");
  for (int n = 0; n < len; ++n) {
    myStorage0.print(String(p_fft[n]*455.) + ",");
    myStorage1.print(String(p_sfft[n]*455.) + ",");
  }
  myStorage0.println();
  myStorage1.println();
  myStorage0.flush();
  myStorage1.flush();
  storage_saved();
}

void saveData(struct WavWavData* data) {
  if (data == '\0') {
    MPLog("<%s> Error: data is null!", __FUNCTION__);
    return;
  }
  if (!storage_begin()) {
    saveStatus.state = SAVE_STATE_ERROR;
    return;
  }
  if (!myStorage0 || !myStorage1) {
    MPLog("<%s> Error: storage not opened!", __FUNCTION__);
    return;
  }
  float* p_wav = data->pWav;
  float* p_swav = data->pSubWav;
  int len = data->len;
  float df = data->df;
  float dt = 1000/(df*len); /* 1000/sampling_rate */
  myStorage0.print(String(dt, 3) + ",");
  myStorage1.print(String(dt, 3) + ",");
  for (int n = 0; n < len; ++n) {
    myStorage0.print(String(p_wav[n]*455.) + ",");
    myStorage1.print(String(p_swav[n]*455.) + ",");
  }
  myStorage0.println();
  myStorage1.println();
  myStorage0.flush();
  myStorage1.flush();
  storage_saved();
}

void closeStorage() {
  if (myStorage0) myStorage0.close();
  if (myStorage1) myStorage1.close();
  storage_opened = false;
}


#endif
