#include "AppSystem.h"


void update_system_properties(int8_t sid, DynamicJsonDocument* sys) {
  int ch, line;
  /* update System Properties */
  g_sid   = sid;              // Serial.println("[Main] sid : " + String(g_sid));
  g_sens  = (*sys)["sens"];   // Serial.println("[Main] sens: " + String(g_sens));
  g_gain  = (*sys)["gain"];   // Serial.println("[Main] gain: " + String(g_gain));
  g_chnm  = (*sys)["chnm"];   Serial.println("[Main] chnm: " + String(g_chnm));
  ch      = (*sys)["ch"];     // 
  g_ch0   = ch & 0x00ff;      Serial.println("[Main] ch0 : " + String(g_ch0));
  g_ch1   = ch >> 8;          // Serial.println("[Main] ch1 : " + String(g_ch1));
  g_rate  = (*sys)["rate"];   Serial.println("[Main] rate: " + String(g_rate));
  line    = (*sys)["line"];   //
  g_samp  = line*2.56;        Serial.println("[Main] samp: " + String(g_samp));
  g_lpf   = (*sys)["lpf"];    // Serial.println("[Main] lpf : " + String(g_lpf));
  g_hpf   = (*sys)["hpf"];    // Serial.println("[Main] hpf : " + String(g_hpf));
}

void readSysprop(int8_t sid, DynamicJsonDocument* doc) {
  
  File mySysprop;
  sfile = "sys.txt";
  Serial.println("Open system file : " + sfile);
  mySysprop = theSD.open(sfile, FILE_READ);
  if (!mySysprop) {
    Serial.println("Cannot open " + sfile);
    while (true) { 
      error_notifier(JSON_ERROR);
    }
  }
  String strSysprop;
  while (mySysprop.available()) strSysprop += char(mySysprop.read());
  DeserializationError error = deserializeJson(*doc, strSysprop);
  if (error) {
    Serial.println("deserializeJson() failed: " + String(error.f_str()));
    while (true) {
      error_notifier(JSON_ERROR);
    }
  }
  mySysprop.close();
  update_system_properties(sid, doc);
}

void updateSysprop(int8_t sid, DynamicJsonDocument* doc, struct Response* res) {
  
  if (res == NULL) return false;
  if (res->value < 0) return false;

  /* Read the current system property at first */
  File mySysprop;
  sfile = "sys.txt";
  Serial.println("[Main] Open system file : " + sfile);
  mySysprop = theSD.open(sfile, FILE_READ);
  if (!mySysprop) {
    Serial.println("[Main] Cannot open " + sfile);
    while (true) {
      error_notifier(FILE_ERROR);
    }
  }
  
  String strSysprop;
  while (mySysprop.available()) strSysprop += char(mySysprop.read());
  DeserializationError error = deserializeJson(*doc, strSysprop);
  if (error) {
    Serial.println("[Main] deserializeJson() failed: " + String(error.f_str()));
    while (true) error_notifier(JSON_ERROR);
  }
  mySysprop.close();

  /* update the system properties according to the response */
  char* label = res->label;
  int   value = res->value;
  if (theSD.exists(sfile)) theSD.remove(sfile);
  Serial.println("[Main] Update System Property File : " + sfile);
  Serial.println("[Main] " + String(label) + ":" + String(value));

  int8_t appid = (*doc)["id"];
  appid /= 100;
  (*doc)["app"] = appid;
  (*doc)[label] = value;
  mySysprop = theSD.open(sfile , FILE_WRITE);
  if (!mySysprop) {
    Serial.println("[Main] Cannot open " + sfile);
    while (true) {
      error_notifier(FILE_ERROR);
    }
  }

  size_t wsize = serializeJson((*doc), mySysprop);
  if (wsize == 0) {
    Serial.print("[Main] serializeJson() failed: ");
    while (true) {
      error_notifier(JSON_ERROR);
    }
  }
  mySysprop.close(); 
  
  update_system_properties(sid, doc);
}

DynamicJsonDocument* updateJson(DynamicJsonDocument* doc, struct Response* res) {
 
  File myJson;
  if (res == NULL) {
    // Serial.println("[Main] Response is null. Open the home menu");
    dfile = "AA000.txt";
  } else {
    int cur0 = res->cur0;
    int cur1 = res->cur1;
    int id = res->next_id;

    /* update the document properties according to the response */
    if (theSD.exists(dfile)) theSD.remove(dfile);
    Serial.println("[Main] Update json file : " + dfile);
    (*doc)["cur0"] = cur0;
    (*doc)["cur1"] = cur1;
    myJson = theSD.open(dfile , FILE_WRITE);
    size_t wsize = serializeJson((*doc), myJson);
    if (wsize == 0) {
      Serial.print("[Main] serializeJson() failed: ");
      while (true) {
        error_notifier(JSON_ERROR);
      }
    }
    myJson.close();    
    char fname[12] = {0};
    memset(fname, '\0', sizeof(char)*12);
    sprintf(fname, "AA%03d.txt", id);
    dfile = String(fname);
  }
    
  /* Open the new json file for the request to change page from Subcore */
  Serial.println("[Main] Open json file : " + dfile);
  myJson = theSD.open(dfile, FILE_READ);
  if (!myJson) {
    Serial.println("[Main] Cannot open " + dfile);
    while (true) {
      error_notifier(FILE_ERROR);
    }
  }
  
  String strJson;
  while (myJson.available()) strJson += char(myJson.read());
  DeserializationError error = deserializeJson(*doc, strJson);
  if (error) {
    Serial.println("[Main] deserializeJson() failed: " + String(error.f_str()));
    while (true) {
      error_notifier(JSON_ERROR);
    }
  }
  myJson.close();
  return doc;
}
