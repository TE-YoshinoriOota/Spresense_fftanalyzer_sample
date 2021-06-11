#include "AppSystem.h"
#include <RTC.h>
#define TIME_HEADER 'T' // Header tag for serial time sync message

#ifdef ENABLE_SAVE_DATA


File myStorage0;
File myStorage1;
static uint8_t storage_id = 0;

char filename0[16] = {};
char filename1[16] = {};

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

void openStorage(uint8_t appid) {
  if (Serial.available()) {
    if(Serial.find(TIME_HEADER)) {
      uint32_t pctime = Serial.parseInt();
      RtcTime rtc(pctime);
      RTC.setTime(rtc);
    }
  }
  
  RtcTime rtc = RTC.getTime();

  bool single = false;
  switch (appid) {
  case SID_REQ_MONDATA:
    sprintf(filename0, "m%02d%03d.csv", rtc.day(), storage_id);
    single = true;
    break;
  case SID_REQ_RAW_FIL:
    sprintf(filename0, "wi%02d%03d.csv", rtc.day(), storage_id);
    sprintf(filename1, "wo%02d%03d.csv", rtc.day(), storage_id);
    break;
  case SID_REQ_WAV_WAV:
    sprintf(filename0, "wa%02d%03d.csv", rtc.day(), storage_id);
    sprintf(filename1, "wb%02d%03d.csv", rtc.day(), storage_id);
    break;
  case SID_REQ_WAV_FFT:
  case SID_REQ_SPECTRO:
    sprintf(filename0, "wf%02d%03d.csv", rtc.day(), storage_id);
    sprintf(filename1, "ft%02d%03d.csv", rtc.day(), storage_id);
    break;
  case SID_REQ_FFT_FFT:
    sprintf(filename0, "fa%02d%03d.csv", rtc.day(), storage_id);
    sprintf(filename1, "fb%02d%03d.csv", rtc.day(), storage_id);
    break;
  case SID_REQ_ORBITDT:
    sprintf(filename0, "ob%02d%03d.csv", rtc.day(), storage_id);
    single = true;
    break;
  }

  if (theSD.exists(filename0)) theSD.remove(filename0);
  myStorage0 = theSD.open(filename0, FILE_WRITE);
  if (!myStorage0) {
    MPLog("Cannot open %s\n", filename0);
    while (true) {
      error_notifier(FILE_ERROR);
    }
  }
  
  if (single) return;
  
  if (theSD.exists(filename1)) theSD.remove(filename1);
  myStorage1 = theSD.open(filename1, FILE_WRITE);
  if (!myStorage1) {
    MPLog("Cannot open %s\n", filename1);
    while (true) {
      error_notifier(FILE_ERROR);
    }
  }
  ++storage_id;
  if (storage_id == 1000) storage_id = 0;
  return;
}


void saveData(struct SensorData* data) {
  if (data == '\0') {
    MPLog("<%s> Error: data is null!", __FUNCTION__);
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
}

void saveData(struct FftFftData* data) {
  if (data == '\0') {
    MPLog("<%s> Error: data is null!", __FUNCTION__);
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
}

void saveData(struct WavWavData* data) {
  if (data == '\0') {
    MPLog("<%s> Error: data is null!", __FUNCTION__);
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
}

void saveData(struct OrbitData* data) {
  if (data == '\0') {
    MPLog("<%s> Error: data is null!", __FUNCTION__);
    return;
  }
  if (!myStorage0 || !myStorage1) {
    MPLog("<%s> Error: storage not opened!", __FUNCTION__);
    return;
  }
}

void closeStorage() {
  if (myStorage0) myStorage0.close();
  if (myStorage1) myStorage1.close();
}


#endif
