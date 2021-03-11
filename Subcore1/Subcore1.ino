#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ArduinoJson.h>
#include <MP.h>
#include "AppScreen.h"

#define TFT_DC 9
#define TFT_CS -1
#define TFT_RST 8

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

#define SID_DOC_MASK 0x01
#define SID_DAT_MASK 0x02
#define SID_WAV_MASK 0x04
#define SID_FFT_MASK 0x08


#define B3 (4)
#define B2 (5)
#define B1 (6)
#define B0 (7)

DynamicJsonDocument* json;

void setup() {
  int8_t sid;
  hardwareSetup();
  MP.begin();
  MP.RecvTimeout(MP_RECV_POLLING);
}

void loop() {
  int8_t sid = 0x00;
  struct Response *res;
  void *data;

  /* Send a request to MainCore */
  if (updateScreen()) {
    /* change page */
    stopApplication();
    getResponse(res);
    sid = SID_DOC_MASK;
    MP.Send(sid, res);
    
  } else if (isSensorAppRunning()) {
    
    /* sensor monitor application running */
    /* check whether the display process is running */
    if (isSensorDataReceived()) {
      sid = SID_DAT_MASK;
      MP.Send(sid, data); /* data is dummy */
      requestSensorData();
    }
    
  } else if (isFftWavAppRunning()) {

    /* fft monitor application running */
    /* check wheter the display process is running */
    if (isFftWavDataReceived()) {
      MPLog("Req FFT-WAV\n");  
      sid = (SID_FFT_MASK | SID_WAV_MASK);
      MP.Send(sid, data);
      requestFftWavData();
    }

  } else if (isFftFftAppRunning()) {

    /* fft monitor application running */
    /* check wheter the display process is running */
    if (isFftFftDataReceived()) {
      MPLog("Req FFT-FFT\n");  
      sid = SID_FFT_MASK;
      MP.Send(sid, data);
      requestFftFftData();
    }
  } 

  /* data arrived from MainCore */
  float dummy = 0.0;
  if (MP.Recv(&sid, &data) < 0) return;
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
    if (fft->len == 0) { /* sometimes MainCore send a null data */
      MPLog("fft->len(%d)\n", fft->len);
      receivedFftWavData();
      delay(500);
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
      delay(500);
      return;
    }
    
    MPLog("fft->len(%d), fft->df(%f)\n", fft->len, fft->df);
    putDraw2FftGraph(fft->pFft, fft->pSubFft, fft->len/2, fft->df);
    receivedFftFftData();
    
    uint32_t delay_time = (uint32_t)(1000. / fft->df);
    MPLog("dt=%d\n", delay_time);
    delay(delay_time);
  }
  //usleep(1);
}
