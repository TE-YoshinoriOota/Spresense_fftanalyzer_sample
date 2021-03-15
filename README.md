# Spresense FFT Analyzer sample

# Software Structure of this sample
This sample code uses 2 core. Maincore takes in charge of Signal Processing, Subcore-1 takes in charge of Visualization. To make the software understand easily and simply, these programs are made by Spresense Arduino Library. So you can remake them very easily. Here are the major specifications of this sample.


## Major functions of this sample
|Functions|Parameters|note|
|---:|-------|------|
|Input Channel|up to 4 channel|Voltage level of michrophone interface is +/-0.45V|
|Sampling Rate|16kHz, 48kHz, 192kHz|This sample supports only 16bit depth|
|Lines for Singal Processing |100,200,400,800,1600|256,512,1024,2046,4096 samples|
|Low pass filter|1k,2k,5k,10k,15k,20k (Hz)|if "0" sets to cutoff, the filter will not be applied|
|High pass filter|5,10,15,20,100,200,500,1k,5k,10k,15k,20k (Hz)|if "0" sets to cutoff, the filter will not be applied|
|Fast Fourier Transform|Rectangular, Hanning, Hamming window|Flat-top will support soon|
|Dual signal analysis|line and filter can be set|Dual FFT graph to compare 2 signals|
|Orbit analysis|Currenly, line can only be set|Plot 2 signals on X-Y coordinates to analyze the correlation|

## Screen Shorts of this examples (still under development)
This sample is implemented on this package. If you are interested in, please contact Sony Semiconductor Solutions Corp. [Japanese](https://www.sony-semicon.co.jp/contact/) / [English](https://www.sony-semicon.co.jp/e/contact/)

![Spresense FFT sample](https://github.com/TE-YoshinoriOota/Spresense_fftanalyzer_sample/blob/main/Documents/screenshots/Package.jpg)

Here are the screenshots of this sample application.
### Home Menu
![Home Menu](https://github.com/TE-YoshinoriOota/Spresense_fftanalyzer_sample/blob/main/Documents/screenshots/0_menu.jpg)

### Digital Filter Application
![Digital Filter](https://github.com/TE-YoshinoriOota/Spresense_fftanalyzer_sample/blob/main/Documents/screenshots/1_digialF_filter.jpg)

### FFT Application
![Single FFT](https://github.com/TE-YoshinoriOota/Spresense_fftanalyzer_sample/blob/main/Documents/screenshots/3_fft.jpg)

### Dual FFT Application
![Dual FFT](https://github.com/TE-YoshinoriOota/Spresense_fftanalyzer_sample/blob/main/Documents/screenshots/4_fft_dual.jpg)

### Orbit Application
![Orbit](https://github.com/TE-YoshinoriOota/Spresense_fftanalyzer_sample/blob/main/Documents/screenshots/5_orbit.jpg)


# Singnal Processing on Maincore
Signal Processing contains signal capturing part and signal processing part. Sensor signal comes from Spresense microphone interface supporting sampling rate of 16kHz, 48kHz, and 192 kHz. This sample includes digital filters like a high pass and a low pass filter and the fast Fourier transformation and so on. 


## Ssignal capturing part on Maincore
Captured signals via Spresense microphone interface are stored in Ringbuffer prepared for each channel with no conditions. This routine should not be obstructed by any tasks. Imagine that capturing signals of 192kHz in 256 samples, the allowed time for capturing is only 1.3 msec (1/192000 x 256 = 1.3msec). So this routine is implemented on the independent thread with high priority. And both the buffer size of the readframe and RingBuffer is very important. If you are not familiar with Spresense Audio system, you should not change the buffer size. If you change without knowledge, the hardware FIFO buffer will be overflow frequently and it makes you bother to make your application. 

```
while(bProcessing) { 

    err = theAudio->readFrames(pRaw, buffer_size, &read_size);
    if (err != AUDIOLIB_ECODE_OK && err != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA) {
      Serial.println("[Main] Error err = " + String(err));
      bProcessing = false;
      break;
    }
    
    /* check if the read data is zero, just rest and continue */ 
    if (read_size == 0) {
      usleep(1); // yield the process to the main loop
      continue;
    }

    // printf("read_size: %d\n", read_size);
     
    /* push the captured data to the ringbuffers */
    int captured_sample = read_size/(g_chnm*sizeof(int16_t));
    pthread_mutex_lock(&m);
    for (int i = 0; i < g_chnm; i++) {
      ringbuff[i].put((q15_t*)pRaw, captured_sample, g_chnm, i);
    }
    pthread_mutex_unlock(&m);  
    usleep(1); // yield the process to main loop
  }
  bThreadStopped = true;  
}
```

## Signal Processing part on Maincore
Signal processing is done as per a request from Subcore-1, and the communication between Maincore and Subcore-1 is done on the main loop. It means that signal processing is worked on the different thread from the signal capturing thread explained above. The main loop on Maincore is very simple. Checking requests from Subcore-1 constantly, when a request arrived, looking into the sid to detect what kind of process Subcore-1 needs, and do that.


```
void loop() {
  int ret;
  int8_t sid = 0;
  if (MP.Recv(&sid, &res, SUBCORE) < 0) {
    usleep(1); // yield the process to SignalProcessing
    return;
  }
  
  Serial.println("[Main] sid = " + String(sid));

  if (sid == SID_REQ_JSONDOC) {
  /* request to change page from Subcore */

    /* stop the signal processing */
    bProcessing = false;
    Serial.println("[Main] Stop Thread");
    
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
      Serial.println("[Main] MP.Send error: " + String(ret));
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1); // yield 
    return;
  } 
  
  /* need signal processing, starts the sub process */
  /* request to launch the process, if it not started */
  if (bProcessing == false) {
    Serial.println("[Main] init_processing");
    readSysprop(sid, sjson);     
    bProcessing = true;
    init_processing(sid, sjson);
    bSignalProcessingStarted = true;
    usleep(MAX_USTIME_FOR_CAPTURING); // yield the process to SignalProcessing
  }
    
  /* request data to monitor sensors */
  if (sid == SID_REQ_MONDATA) {

    Serial.println("[Main] Calculating sensor data");
    calc_sensor_data(&sData);

    struct SensorData sdata;
    memcpy(&sdata, &sData, sizeof(sData));

    Serial.println("[Main] Send monitor data to Subcore");

    /* send the data to update the number on LCD display */
    ret = MP.Send(sid, &sdata, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while(true) {
        error_notifier(MP_ERROR);
      }    
    }
    usleep(1);
    return;      
  } 
  
  /* request calculate fft data */  
  if (sid == SID_REQ_WAV_FFT) {

    Serial.println("[Main] Calculating fft-wav data");
    calc_fft_data(&fftWavData);

    Serial.println("[Main] Send fft-wav data to SUBCORE");

    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &fftWavData, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while (true) { 
        error_notifier(MP_ERROR); 
      }    
    } 
    usleep(1);
    return; 
  } 
  
  /* request calculate double fft data */
  if (sid == SID_REQ_FFT_FFT) {

    Serial.println("[Main] Calculating fft-fft data");
    calc_fft2_data(&fft2Data);

    Serial.println("[Main] Send fft-fft data to SUBCORE");

    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &fft2Data, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while (true) {
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1);
    return;        
  } 
  
  /* request calculate double wav data */   
  if (sid == SID_REQ_WAV_WAV) {

    Serial.println("[Main] Collecting wav-wav data");
    get_wav2_data(&wav2Data);

    Serial.println("[Main] Send wav-wav data to SUBCORE");
    
    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &wav2Data, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1);
    return;        
  } 
  
  /* request calculate double wav data */   
  if (sid == SID_REQ_RAW_FIL) {

    Serial.println("[Main] Collecting filtered wav data");
    get_rawfil_data(&wav2Data);

    Serial.println("[Main] Send raw-filtered data to SUBCORE");
    
    /* send the wan and fft data to update the graph on LCD display */     
    ret = MP.Send(sid, &wav2Data, SUBCORE);
    if (ret < 0) {
      Serial.println("[Main] MP.Send error: " + String(ret));
      while(true) { 
        error_notifier(MP_ERROR); 
      }    
    }
    usleep(1);
    return;      
  }
  
  usleep(1); // yield the process to SignalProcessing
}
```

Signal processing is different implemented manner as per each application on Subcore-1. Generally, each Subcore applications request different data, so the signal processing should be implemented differently. However, there is one important thing you should mind when implements an application. That is the RingBuffer protection. When taking the signal from RingBuffer, you should avoid conflict between the threads of Singal Capturing and Signal Processing. Remember that the Signal Capturing is running on a different high priority thread. On the other hand, Signal Processing is running on the main loop. So when your Signal Processing application picks up signal data from the Ring Buffer managed by the Signal Capturing thread, you should implement to protect the Ring Buffer by a mutex to pick up safely. Needless to say, the picking up process should be very short not to obstacle the Signal Capturing process.

```
void calc_sensor_data(struct SensorData* sdata) {

  /* get the data from the ringbuffer */
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);   
  pthread_mutex_unlock(&m);  
  
  uint32_t cur_time = millis();
  uint32_t dt = cur_time - last_time;
  calcSensorData(&sData, pWav, g_samp, dt);
  last_time = cur_time;
}

void calc_fft_data(struct FftWavData* fdata) {
  
  /* Copy data for FFT calculation */
  //uint32_t mTime = millis();
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);       
  pthread_mutex_unlock(&m); 


  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);
  }
  if (hpf != NULL) {
    hpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);    
  }
  
  /* ch0 fft transform */
  memcpy(pTmp, pWav, sizeof(float)*g_samp); 
  fft->clear();
  fft->fft_amp(pFft, pTmp);
  //mTime = millis() - mTime;
  //Serial.println("[Main] " + String(mTime));
   
  fdata->pWav = pWav;
  fdata->pFft = pFft;
  fdata->len  = g_samp;
  fdata->df   = (float)(g_rate)/g_samp;
}

void calc_fft2_data(struct FftFftData* fdata) {
  
  /* Copy data for FFT calculation */
  uint32_t mTime = millis();
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);       
  ringbuff[(g_ch1-1)].get(pSubWav, g_samp);       
  pthread_mutex_unlock(&m); 

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);
  }
  if (hpf != NULL) {
    hpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);    
  }
  
  /* ch0 fft transform */
  memcpy(pTmp, pWav, sizeof(float)*g_samp);
  fft->clear();
  fft->fft_amp(pFft, pTmp);

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);
  }
  if (hpf != NULL) {
    hpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);    
  }

  /* ch1 fft transform */
  memcpy(pTmp, pSubWav, sizeof(float)*g_samp);
  fft->clear();
  fft->fft_amp(pSubFft, pTmp);
  //mTime = millis() - mTime;
  //Serial.println("[Main] " + String(mTime));

  fdata->pFft = pFft;
  fdata->pSubFft = pSubFft;
  fdata->len  = g_samp;
  fdata->df   = (float)(g_rate)/g_samp;
 
}


void get_wav2_data(struct WavWavData* wdata) {
  
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);       
  ringbuff[(g_ch1-1)].get(pSubWav, g_samp);       
  pthread_mutex_unlock(&m); 

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);
  } 
  if (hpf != NULL) {
    hpf->get(pTmp, pWav, g_samp);
    memcpy(pWav, pTmp, sizeof(float)*g_samp);    
  }

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  if (lpf != NULL) {
    lpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);
  } 
  if (hpf != NULL) {
    hpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);    
  }
  
  wdata->pWav = pWav;
  wdata->pSubWav = pSubWav;
  wdata->len  = g_samp;
  wdata->df   = (float)(g_rate)/g_samp;
  
}

void get_rawfil_data(struct WavWavData* wdata) {
  
  pthread_mutex_lock(&m);
  ringbuff[(g_ch0-1)].get(pWav, g_samp);   
  pthread_mutex_unlock(&m); 

  /* digital fitering */
  /* which is better? lpf first? hpf first? */
  /* note that this process is slightly different from above for keeping the original raw data*/
  if (lpf != NULL) {
    Serial.println("[Main] applying low pass filter");
    lpf->get(pTmp, pWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);
  } else { 
    // in case of no lpf applied.
    memcpy(pSubWav, pWav, sizeof(float)*g_samp);    
  }
  
  if (hpf != NULL) {
    Serial.println("[Main] applying high pass filter");
    hpf->get(pTmp, pSubWav, g_samp);
    memcpy(pSubWav, pTmp, sizeof(float)*g_samp);    
  }
  
  wdata->pWav = pWav;
  wdata->pSubWav = pSubWav;
  wdata->len  = g_samp;
  wdata->df   = (float)(g_rate)/g_samp;  
}
```

# Visualization application on Subcore-1
Subcore-1 takes charge of the visualization of signal processed data. This application contains 3 parts. Application Life Cycle Management related to the communication part with Maincore.  Visualization part that plots signal processed data. Layouting part that managing information display and parameter inputs, etc... This chapter describes each part.

## Commnication between Maincore and Subcore-1
Communication between Maincore and Subcore-1 is based on a master-slave model. The master is Subcore-1, the slave is Maincore. The main loop of the Subcore-1 takes in charge of managing communication to the Maincore and reflects the receiving data to the LCD display. The application data is identified by SID that there is a rule to assign. The SIDs of applications requiring FFT should be numbered between 0x10 to 0x70. The reason is that Maincore prepares the resource for FFT by looking at this id. 

The process of the main loop on Subcore-1 is also simple. Firstly, checking the page change request. Secondly, if an application requiring signal processing is running, send the data request to Maincore. Thirdly, checking the data arrival, and reflecting it to the application. 

To say it simply, sending data requests to Maincore and receiving the requested data is the main loop task.

```
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
      Serial.println("MP.Send error: " + String(ret));
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
    }

    MPLog("Request data to Maincore: 0x%02x\n", sid);
    
    ret = MP.Send(sid, data); /* data is dummy */
    if (ret < 0) {
      Serial.println("MP.Send error: " + String(ret));
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
    appDrawOrbitGraph(wav->pWav, wav->pSubWav, wav->len, wav->df);
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
  
  return;
}
```

## Visalization process on Subcore-1
The visualization process of this sample is relatively complicated. The application model is based on Model-View-Controller. The main loop that manages Application Life Cycle is the controller needless to say. The model in charge of visualizing signal processed data is appDrawxxx functions implemented in ScreenApps file. The view functions that manage the layout of the application are defined in ScreenElements file. (The navigation buttons are handled by ScreenElements) The views are made by the builder functions in BuildScreen file. The construction process of an application is as follows. 

1. A user changes an application page by pressing the button "NEXT" or "BACK".
2. The main loop detects the change request and sends the request to get JSON data for the next page.
3. Maincore gets the request, sends the JSON data back to Subcore-1
4. Subcore-1 gets the data in the main loop, then starts to build the view by using JSON doc. During the building the view, if the application needs the signal processing data, builder sets a data-request by calling startApplication(). 
5. The main loop detects the data request by calling isRunningApplication(), and sends the request to Maincore.
6. Maincore gets the request, sends the signal processed data back to Subcore-1.
7. The main loop on Subcore-1 receives the data and plot the data on the LCD display by appDrawxxxx functions
8. Loop from (5) to (7) until the end of the application by User instruction pressing "NEXT" or "BACK"

When a user presses the "NEXT" or "BACK" button, the framework makes the application-loop go back to (1) and stops the application at the (2) step.

## Layouting process on Subcore-1
Comming soon...



