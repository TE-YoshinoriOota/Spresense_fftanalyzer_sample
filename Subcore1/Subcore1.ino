#include "AppScreen.h"

DynamicJsonDocument* json;

void setup() {
  int ret;
  int8_t sid;
  hardwareSetup();
  ret = MP.begin();
  if (ret < 0) {
    MPLog("MP.begin error: %d\n", ret);
    while(true) { 
      error_notifier(MP_ERROR); 
    }    
  }
  
  MP.RecvTimeout(MP_RECV_POLLING);
}

void loop() {
  int ret;
  int8_t sid = 0x00;
  int appid = 0;
  struct Response *res = NULL;
  void *data = NULL;

  /* Send a request to MainCore */
  if (updateScreen()) {
    /* change page */
    //stopApplication();
    stopApplication();
    getResponse(res);
    sid = SID_REQ_JSONDOC;
    ret = MP.Send(sid, res);
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }   
  } 
  
  appid = isAppRunning();
  
  if ((appid > 0) && isDataReceived()) {
    switch(appid) {
    case APP_ID_MONDATA: sid = SID_REQ_MONDATA; break;
    case APP_ID_WAV_FFT: sid = SID_REQ_WAV_FFT; break;
    case APP_ID_FFT_FFT: sid = SID_REQ_FFT_FFT; break;
    case APP_ID_WAV_WAV: sid = SID_REQ_WAV_WAV; break;
    case APP_ID_RAW_FIL: sid = SID_REQ_RAW_FIL; break;
    case APP_ID_ORBITDT: sid = SID_REQ_ORBITDT; break;
    }

    MPLog("Request data to Maincore: 0x%02x\n", sid);
    
    ret = MP.Send(sid, data); /* data is dummy */
    if (ret < 0) {
      MPLog("MP.Send error: %d\n", ret);
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    requestedData();
  }
    

  /* data arrived from MainCore */
  float dummy = 0.0;
  if (MP.Recv(&sid, &data) < 0) {
    return;
  }
  MPLog("sid: 0x%02x\n", sid);

  /* check if the data is json documents */
  if (sid == SID_REQ_JSONDOC) {
    
    MPLog("Update screen\n");
    json = (DynamicJsonDocument*)data;
    clearScreen(json);
    char* title = (*json)["title"];
    MPLog("Build Screen %s\n", title);
    BuildScreen(json);
    
    return;  
  }
  
  if (sid == SID_REQ_MONDATA) {

    MPLog("Sensor Data arrived\n");
    
    /* check if the data is for sensor monitor application */
    struct SensorData *sd = (struct SensorData*)data;
    appSensorValue(sd->acc, sd->vel, sd->dis);
    receivedData();
    delay(100); /* delay for avoiding data flicker on LCD */
    
    return;
  } 

  if (sid == SID_REQ_WAV_FFT) {

    MPLog("FFT-WAV Data arrived\n");
    
    /* check if th data is for FFT monitor application */
    struct FftWavData *fft = (struct FftWavData*)data;
    if (fft->len == 0) { /* sometimes Maincore send a null data, sp check it */
      MPLog("fft->len(%d)\n", fft->len);
      receivedData();
      return;
    }
    
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
    appDraw2WayGraph(fft->pWav, fft->len, fft->pFft, fft->len/2, fft->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time);

    return;
  } 

  if (sid == SID_REQ_FFT_FFT) {

    MPLog("FFT-FFT Data arrived\n");
    
    /* check if th data is for FFT monitor application */
    struct FftFftData *fft = (struct FftFftData*)data;
    if (fft->len == 0) { /* sometimes MainCore send a null data */
      MPLog("fft->len(%d)\n", fft->len);
      receivedData();
      return;
    }
    
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
    appDraw2FftGraph(fft->pFft, fft->pSubFft, fft->len/2, fft->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time); // is this call really needed?

    return;
  } 

  if (sid == SID_REQ_WAV_WAV) {

    MPLog("WAV-WAV Data arrived\n");
    
    /* check if th data is for FFT monitor application */
    struct WavWavData *wav = (struct WavWavData*)data;
    if (wav->len == 0) { /* sometimes MainCore send a null data */
      MPLog("wav->len(%d)\n", wav->len);
      receivedData();
      return;
    }
    
    MPLog("wav->len(%d), wav->df(%f)\n", wav->len, wav->df);
    /* appDrawOrbitGraph(wav->pWav, wav->pSubWav, wav->len, wav->df); */
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / wav->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time); // is this call really needed?
    
    return;
  }
  
  if (sid == SID_REQ_RAW_FIL) {

    MPLog("RAW-FILL Data arrived\n");
    
    struct WavWavData *wav = (struct WavWavData*)data;
    if (wav->len == 0) { 
      MPLog("wav->len(%d)\n", wav->len);
      receivedData();
      return;
    }
    
    MPLog("wav->len(%d), wav->df(%f)\n", wav->len, wav->df);
    appDrawFilterGraph(wav->pWav, wav->pSubWav, wav->len, wav->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / wav->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time); // is this call really needed?

    return;
  }  

  if (sid == SID_REQ_ORBITDT) {

    MPLog("Orbit Data arrived\n");
    
    /* check if the data is for sensor monitor application */
    struct OrbitData *odata = (struct OrbitData*)data;
    appDrawOrbitGraph(odata);
    receivedData();
    delay(100); /* delay for avoiding data flicker on LCD */
    
    return;
  }   
  return;
}
