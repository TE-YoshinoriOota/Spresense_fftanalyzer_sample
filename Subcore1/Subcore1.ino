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
    // erase save_request for safety 
    pthread_mutex_lock(&mtx);
    save_request = SAVE_INIT;
    pthread_mutex_unlock(&mtx);  
  } 
  
  appid = isAppRunning();
  
  if ((appid > 0) && isDataReceived()) {
    switch(appid) {
    case APP_ID_MONDATA: sid = SID_REQ_MONDATA | save_request; break;
    case APP_ID_WAV_FFT: sid = SID_REQ_WAV_FFT | save_request; break;
    case APP_ID_FFT_FFT: sid = SID_REQ_FFT_FFT | save_request; break;
    case APP_ID_WAV_WAV: sid = SID_REQ_WAV_WAV | save_request; break;
    case APP_ID_RAW_FIL: sid = SID_REQ_RAW_FIL | save_request; break;
    case APP_ID_ORBITDT: sid = SID_REQ_ORBITDT | save_request; break;
    case APP_ID_SPECTRO: sid = SID_REQ_SPECTRO | save_request; break;
  }
  pthread_mutex_lock(&mtx);
#ifdef SCR_DEBUG
  MPLog("save_request:%d\n", save_request);
#endif
  save_request = SAVE_INIT;
  pthread_mutex_unlock(&mtx);
      
#ifdef MP_DEBUG
    MPLog("Request data to Maincore: 0x%02x\n", sid);
#endif

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

#ifdef MP_DEBUG
  MPLog("sid: 0x%02x\n", sid);
#endif

  /* check whether sid is sdcard error */
  /*     tentative implementation      */    
  if (sid == SID_REQ_ESDCARD) {
    buildSDCardErrorPage();
    return;
  }

  /* check if the data is json documents */
  if (sid == SID_REQ_JSONDOC) {
#ifdef MP_DEBUG
    MPLog("Update screen\n");
#endif
    json = (DynamicJsonDocument*)data;
    ClearScreen(json);
    char* title = (*json)["title"];
#ifdef MP_DEBUG
    MPLog("Build Screen %s\n", title);
#endif
    BuildScreen(json);
    return;  
  }
  
  if (sid == SID_REQ_MONDATA) {
#ifdef MP_DEBUG
    MPLog("Sensor Data arrived\n");
#endif
    /* check if the data is for sensor monitor application */
    struct SensorData *sd = (struct SensorData*)data;
    appSensorValue(sd->acc, sd->vel, sd->dis);
    receivedData();
    delay(100); /* delay for avoiding data flicker on LCD */
    
    return;
  } 

  if (sid == SID_REQ_WAV_FFT) {
#ifdef MP_DEBUG
    MPLog("FFT-WAV Data arrived\n");
#endif  
    /* check if th data is for FFT monitor application */
    struct FftWavData *fft = (struct FftWavData*)data;
    if (fft->len == 0) { /* sometimes Maincore send a null data, sp check it */
      MPLog("fft->len(%d)\n", fft->len);
      receivedData();
      return;
    }
#ifdef MP_DEBUG
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
#endif  
    appDraw2WayGraph(fft->pWav, fft->len, fft->pFft, fft->len/2, fft->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
#ifdef MP_DEBUG
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time);
    return;
  } 

  if (sid == SID_REQ_FFT_FFT) {
#ifdef MP_DEBUG
    MPLog("FFT-FFT Data arrived\n");
#endif
    /* check if th data is for FFT monitor application */
    struct FftFftData *fft = (struct FftFftData*)data;
    if (fft->len == 0) { /* sometimes MainCore send a null data */
#ifdef MP_DEBUG
      MPLog("fft->len(%d)\n", fft->len);
#endif
      receivedData();
      return;
    }
#ifdef MP_DEBUG    
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
#endif
    appDraw2FftGraph(fft->pFft, fft->pSubFft, fft->len/2, fft->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
#ifdef MP_DEBUG    
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time); // is this call really needed?
    return;
  } 

  if (sid == SID_REQ_WAV_WAV) {
#ifdef MP_DEBUG
    MPLog("WAV-WAV Data arrived\n");
#endif
    /* check if th data is for FFT monitor application */
    struct WavWavData *wav = (struct WavWavData*)data;
    if (wav->len == 0) { /* sometimes MainCore send a null data */
      MPLog("wav->len(%d)\n", wav->len);
      receivedData();
      return;
    }
#ifdef MP_DEBUG
    MPLog("wav->len(%d), wav->df(%f)\n", wav->len, wav->df);
#endif
    appDraw2WavGraph(wav->pWav, wav->pSubWav, wav->len, wav->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / wav->df);
#ifdef MP_DEBUG
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time); // is this call really needed?
    return;
  }
  
  if (sid == SID_REQ_RAW_FIL) {
#ifdef MP_DEBUG
    MPLog("RAW-FILL Data arrived\n");
#endif
    struct WavWavData *wav = (struct WavWavData*)data;
    if (wav->len == 0) { 
#ifdef MP_DEBUG
      MPLog("wav->len(%d)\n", wav->len);
#endif
      receivedData();
      return;
    }
#ifdef MP_DEBUG
    MPLog("wav->len(%d), wav->df(%f)\n", wav->len, wav->df);
#endif
    appDraw2WavGraph(wav->pWav, wav->pSubWav, wav->len, wav->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / wav->df);
#ifdef MP_DEBUG
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time); // is this call really needed?
    return;
  }  


  if (sid == SID_REQ_ORBITDT) {
#ifdef MP_DEBUG
    MPLog("Orbit Data arrived\n");
#endif
    /* check if the data is for sensor monitor application */
    struct OrbitData *odata = (struct OrbitData*)data;
    appDrawOrbitGraph(odata);
    receivedData();
    delay(100); /* delay for avoiding data flicker on LCD */
    return;
  }   

  if (sid == SID_REQ_SPECTRO) {
#ifdef MP_DEBUG
    MPLog("Spectrogram Data arrived\n");
#endif  
    /* check if th data is for FFT monitor application */
    struct FftWavData *fft = (struct FftWavData*)data;
    if (fft->len == 0) { /* sometimes Maincore send a null data, sp check it */
      MPLog("fft->len(%d)\n", fft->len);
      receivedData();
      return;
    }
#ifdef MP_DEBUG
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
#endif  
    appDrawSpectroGraph(fft->pFft, fft->len/2, fft->df);
    receivedData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
#ifdef MP_DEBUG
    MPLog("dt=%d\n", delay_time);
#endif
    delay(delay_time);
    return;
  }   
  return;
}
