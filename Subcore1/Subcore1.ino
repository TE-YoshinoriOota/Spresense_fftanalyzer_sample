#include "AppScreen.h"

DynamicJsonDocument* json;

void setup() {
  int ret;
  int8_t sid;
  hardwareSetup();
  ret = MP.begin();
  if (ret < 0) {
    Serial.println("MP.begin error: " + String(ret));
    while(true) { 
      error_notifier(MP_ERROR); 
    }    
  }
  
  MP.RecvTimeout(MP_RECV_POLLING);
}

void loop() {
  int ret;
  int8_t sid = 0x00;
  struct Response *res = NULL;
  void *data = NULL;

  /* Send a request to MainCore */
  if (updateScreen()) {
    /* change page */
    stopApplication();
    getResponse(res);
    sid = SID_DOC_MASK;
    ret = MP.Send(sid, res);
    if (ret < 0) {
      Serial.println("MP.Send error: " + String(ret));
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    
  } else if (isSensorAppRunning()) {
    
    /* sensor monitor application running */
    /* check whether the display process is running */
    if (isSensorDataReceived()) {
      sid = SID_DAT_MASK;
      ret = MP.Send(sid, data); /* data is dummy */
      if (ret < 0) {
        Serial.println("MP.Send error: " + String(ret));
        while(true) { 
          error_notifier(MP_ERROR); 
        }    
      }
      requestSensorData();
    }
    
  } else if (isFftWavAppRunning()) {

    /* fft monitor application running */
    /* check wheter the display process is running */
    if (isFftWavDataReceived()) {
      MPLog("Req FFT-WAV\n");  
      sid = (SID_FFT_MASK | SID_WAV_MASK);
      ret = MP.Send(sid, data);
      if (ret < 0) {
        Serial.println("MP.Send error: " + String(ret));
        while(true) { 
          error_notifier(MP_ERROR); 
        }    
      }
      requestFftWavData();
    }

  } else if (isFftFftAppRunning()) {

    /* fft monitor application running */
    /* check wheter the display process is running */
    if (isFftFftDataReceived()) {
      MPLog("Req FFT-FFT\n");  
      sid = SID_FFT_MASK;
      ret = MP.Send(sid, data);
      if (ret < 0) {
        Serial.println("MP.Send error: " + String(ret));
        while(true) { 
          error_notifier(MP_ERROR); 
        }    
      }
      requestFftFftData();
    }
      
  } else if (isOrbitAppRunning()) {

    /* fft monitor application running */
    /* check wheter the display process is running */
    if (isOrbitDataReceived()) {
      MPLog("Req WAV-WAV\n");  
      sid = SID_WAV_MASK;
      ret = MP.Send(sid, data);
      if (ret < 0) {
        Serial.println("MP.Send error: " + String(ret));
        while(true) { 
          error_notifier(MP_ERROR); 
        }    
      }
      requestOrbitData();  
    }
  } 

  /* data arrived from MainCore */
  float dummy = 0.0;
  if (MP.Recv(&sid, &data) < 0) {
    return;
  }
  MPLog("sid: 0x%02x\n", sid);

  /* check if the data is json documents */
  if (sid & SID_DOC_MASK) {
    
    MPLog("Update screen\n");
    json = (DynamicJsonDocument*)data;
    clearScreen(json);
    char* title = (*json)["title"];
    MPLog("Build Screen %s\n", title);
    BuildScreen(json);
    
  } else if (sid & SID_DAT_MASK) {

    MPLog("Sensor Data arrived\n");
    
    /* check if the data is for sensor monitor application */
    struct SensorData *sd = (struct SensorData*)data;
    putSensorValue(sd->acc, sd->vel, sd->dis);
    receivedSensorData();
    delay(100); /* delay for avoiding data flicker on LCD */
    
  } else if ((sid & (SID_FFT_MASK | SID_WAV_MASK)) == (SID_FFT_MASK | SID_WAV_MASK)) {

    MPLog("FFT-WAV Data arrived\n");
    
    /* check if th data is for FFT monitor application */
    struct FftWavData *fft = (struct FftWavData*)data;
    if (fft->len == 0) { /* sometimes Maincore send a null data, sp check it */
      MPLog("fft->len(%d)\n", fft->len);
      receivedFftWavData();
      return;
    }
    
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
    putDraw2WayGraph(fft->pWav, fft->len, fft->pFft, fft->len/2, fft->df);
    receivedFftWavData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time);
    
  } else if ((sid & SID_FFT_MASK) == (SID_FFT_MASK)) {

    MPLog("FFT-FFT Data arrived\n");
    
    /* check if th data is for FFT monitor application */
    struct FftFftData *fft = (struct FftFftData*)data;
    if (fft->len == 0) { /* sometimes MainCore send a null data */
      MPLog("fft->len(%d)\n", fft->len);
      receivedFftFftData();
      return;
    }
    
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
    putDraw2FftGraph(fft->pFft, fft->pSubFft, fft->len/2, fft->df);
    receivedFftFftData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time); // is this call really needed?
    
  } else if ((sid & SID_WAV_MASK) == (SID_WAV_MASK)) {

    MPLog("WAV-WAV Data arrived\n");
    
    /* check if th data is for FFT monitor application */
    struct WavWavData *wav = (struct WavWavData*)data;
    if (wav->len == 0) { /* sometimes MainCore send a null data */
      MPLog("wav->len(%d)\n", wav->len);
      receivedOrbitData();
      return;
    }
    
    MPLog("wav->len(%d), wav->df(%f)\n", wav->len, wav->df);
    putDrawOrbitGraph(wav->pWav, wav->pSubWav, wav->len, wav->df);
    receivedOrbitData();
    
    uint32_t delay_time = (uint32_t)(1000. / wav->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time); // is this call really needed?
  }
  
  return;
}
