#include "AppSystem.h"

void setup() {
  int ret;
  
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  
  Serial.begin(115200);
  while (!Serial) {}

  while (!theSD.begin()) {
    Serial.println("Insert SD card");
    error_notifier(FILE_ERROR);
  }
  
  ret = MP.begin(SUBCORE);
  if (ret < 0) {
    Serial.println("MP.begin error: " + String(ret));
    while(true) { 
      error_notifier(MP_ERROR); 
    }
  }
  
  djson = updateJson(djson, NULL);
  int8_t sid = SID_DOC_MASK;
  ret = MP.Send(sid, djson, SUBCORE);
  if (ret < 0) {
    Serial.println("MP.Send error: " + String(ret));
    while(true) { 
      error_notifier(MP_ERROR); 
    }    
  }
  MP.RecvTimeout(MP_RECV_POLLING);
}

void loop() {
  int ret;
  int8_t sid = 0;
  if (MP.Recv(&sid, &res, SUBCORE) < 0) {
    usleep(1); // yield the process to SignalProcessing
    return;
  }
  printf("[Main] 0x%02x\n", sid);

  if (sid & SID_DOC_MASK) {
  /* request to change page from Subcore */

    /* stop the signal processing */
    bProcessing = false;
    Serial.println("Stop Thread");
    
    /* wait for the thread to stopp */
    while (!bThreadStopped) { 
      usleep(1); 
    } 
    
    if (bSignalProcessingStarted) {
      finish_processing();
      bSignalProcessingStarted = false;
    }
    
    /* update system properties */
    updateSysprop(sid, sjson, res);

    /* get the new page resources and update the json document */
    djson = updateJson(djson, res);
    int ch = (*sjson)["ch"];
    int ch0 = ch & 0x00ff;
    int ch1 = ch >> 8;
    (*djson)["ch0"] = ch0;
    (*djson)["ch1"] = ch1;

    /* send the json document to change the page on LCD */
    ret = MP.Send(sid, djson, SUBCORE);
    if (ret < 0) {
      Serial.println("MP.Send error: " + String(ret));
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }      

  } else if (sid > SID_DOC_MASK) {
    /* request to launch sensor applications */
    if (bProcessing == false) {
      Serial.println("init_processing");
      readSysprop(sid, sjson);     
      bProcessing = true;
      init_processing(sid, sjson);
      bSignalProcessingStarted = true;
    }

    if (sid & SID_DAT_MASK) {
    /* request data to monitor sensors */
      
      calc_sensor_data(&sData);

      struct SensorData sdata;
      memcpy(&sdata, &sData, sizeof(sData));

      /* send the data to update the number on LCD display */
      sid = SID_DAT_MASK;
      ret = MP.Send(sid, &sdata, SUBCORE);
      if (ret < 0) {
        Serial.println("MP.Send error: " + String(ret));
        while(true) {
          error_notifier(MP_ERROR);
        }    
      }      
    } else if ((sid & (SID_FFT_MASK | SID_WAV_MASK)) == (SID_FFT_MASK | SID_WAV_MASK)) {
    /* request calculate fft data */

      calc_fft_data(&fftWavData);

      Serial.println("Send fft-wav data to SUBCORE");

      /* send the wan and fft data to update the graph on LCD display */     
      sid = (SID_FFT_MASK | SID_WAV_MASK);
      ret = MP.Send(sid, &fftWavData, SUBCORE);
      if (ret < 0) {
        Serial.println("MP.Send error: " + String(ret));
        while (true) { 
          error_notifier(MP_ERROR); 
        }    
      }      
    } else if ((sid & SID_FFT_MASK) == (SID_FFT_MASK)) {
    /* request calculate double fft data */

      calc_fft2_data(&fft2Data);

      Serial.println("Send fft-fft data to SUBCORE");

      /* send the wan and fft data to update the graph on LCD display */     
      sid = SID_FFT_MASK;
      ret = MP.Send(sid, &fft2Data, SUBCORE);
      if (ret < 0) {
        Serial.println("MP.Send error: " + String(ret));
        while (true) {
          error_notifier(MP_ERROR); 
        }    
      }
            
    } else if ((sid & SID_WAV_MASK) == (SID_WAV_MASK)) {
    /* request calculate double wav data */   

      get_wav2_data(&wav2Data);

      Serial.println("Send wav-wav data to SUBCORE");
      
      /* send the wan and fft data to update the graph on LCD display */     
      sid = SID_WAV_MASK;
      ret = MP.Send(sid, &wav2Data, SUBCORE);
      if (ret < 0) {
        Serial.println("MP.Send error: " + String(ret));
        while(true) { 
          error_notifier(MP_ERROR); 
        }    
      }
      
    }
  }
  usleep(1); // yield 
}
