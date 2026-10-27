#include "AppSystem.h"
#include <errno.h>
#include <sys/boardctl.h>

/*
 * sys.txt is the master of the settings.
 * normalize_sysprop() keeps the settings consistent with each other,
 * and patchPage() fits the menu pages of the settings to sys.txt when they are shown.
 */
static bool is_hirate(int rate) {
  return rate > 48000;
}

/* the biquad filter diverges at the cutoff of rate/2 or more */
static bool filter_ok(int cutoff, int rate) {
  return cutoff < rate/2;
}

static int max_rech(int rate, int chnm) {
  int n = is_hirate(rate) ? HIRATE_MAX_RECH : 2;
  if (n > chnm) n = chnm;
  return (n < 1) ? 1 : n;
}

static void normalize_sysprop(DynamicJsonDocument* sys) {
  int rate = (*sys)["rate"];
  int chnm = (*sys)["chnm"];
  if (is_hirate(rate) && chnm > HIRATE_MAX_CHNM) {
    MPLog("chnm %d -> %d for %d Hz\n", chnm, HIRATE_MAX_CHNM, rate);
    chnm = HIRATE_MAX_CHNM;
    (*sys)["chnm"] = chnm;
  }
  int ch  = (*sys)["ch"];
  int ch0 = ch & 0x00ff;
  int ch1 = ch >> 8;
  if (ch0 > chnm || ch1 > chnm) {
    if (ch0 > chnm) ch0 = 1;
    if (ch1 > chnm) ch1 = (chnm >= 2 && ch0 != 2) ? 2 : 1;
    MPLog("ch -> CH%d, CH%d\n", ch0, ch1);
    (*sys)["ch"] = (ch1 << 8) | ch0;
  }
  int rech = (*sys)["rech"] | 1;
  if (rech > max_rech(rate, chnm)) {
    MPLog("rech %d -> %d\n", rech, max_rech(rate, chnm));
    (*sys)["rech"] = max_rech(rate, chnm);
  }
  int src  = (*sys)["src"] | SRC_MIC;
  int line = (*sys)["line"];
  if (src != SRC_FILE && line > LINE_MAX) {
    MPLog("line %d -> %d for the source %d\n", line, LINE_MAX, src);
    (*sys)["line"] = LINE_MAX;
  }
  const char* fkey[2] = { "lpf", "hpf" };
  for (int i = 0; i < 2; ++i) {
    int cutoff = (*sys)[fkey[i]];
    if (cutoff != 0 && !filter_ok(cutoff, rate)) {
      MPLog("%s %d -> 0 for %d Hz\n", fkey[i], cutoff, rate);
      (*sys)[fkey[i]] = 0;
    }
  }
}

/* the original number of items of the patched page, to write back the page file as it was */
static int patched_id = -1;
static int patched_num[2];

/* index of the item which has the value, or the last item */
static int find_item(DynamicJsonDocument* doc, String prefix, String suffix, int num, int value) {
  for (int i = 0; i < num; ++i) {
    int v = (*doc)[prefix + String(i) + suffix];
    if (v == value) return i;
  }
  return num - 1;
}

static void patch_menu(DynamicJsonDocument* doc, int limit, int value) {
  int num = (*doc)["item_num"];
  patched_id = (*doc)["id"];
  patched_num[0] = num;
  if (limit > 0 && limit < num) num = limit;
  (*doc)["item_num"] = num;
  (*doc)["cur0"] = find_item(doc, "item", "_v", num, value);
}

static void patch_dmenu(DynamicJsonDocument* doc, int limit, int value0, int value1) {
  int num0 = (*doc)["item0_num"];
  int num1 = (*doc)["item1_num"];
  patched_id = (*doc)["id"];
  patched_num[0] = num0;
  patched_num[1] = num1;
  if (limit > 0 && limit < num0) num0 = limit;
  if (limit > 0 && limit < num1) num1 = limit;
  (*doc)["item0_num"] = num0;
  (*doc)["item1_num"] = num1;
  (*doc)["cur0"] = find_item(doc, "item0_v", "", num0, value0);
  (*doc)["cur1"] = find_item(doc, "item1_v", "", num1, value1);
}

/* the input page of a filter: hide the cutoffs the rate can't use (the list is in ascending order) */
static void patch_filter(DynamicJsonDocument* doc, int rate, int value) {
  int num = (*doc)["inp_num"];
  int n = 0;
  while (n < num && ((int)(*doc)["input"][n] == 0 || filter_ok((*doc)["input"][n], rate))) ++n;
  if (n < 1) n = 1;
  patched_id = (*doc)["id"];
  patched_num[0] = num;
  (*doc)["inp_num"] = n;
  int cur = 0;  /* no filter, if the value is hidden */
  for (int i = 0; i < n; ++i) {
    if ((int)(*doc)["input"][i] == value) cur = i;
  }
  (*doc)["cur0"] = cur;
}

/* the input page of the FFT lines: 3200 and 6400 lines are only for the WAV file without the sound */
static void patch_line(DynamicJsonDocument* doc, int src, int value) {
  int num = (*doc)["inp_num"];
  int max = (src == SRC_FILE) ? LINE_MAX_FILE : LINE_MAX;
  int n = 0;
  while (n < num && (int)(*doc)["input"][n] <= max) ++n;
  if (n < 1) n = 1;
  patched_id = (*doc)["id"];
  patched_num[0] = num;
  (*doc)["inp_num"] = n;
  int cur = n - 1;
  for (int i = 0; i < n; ++i) {
    if ((int)(*doc)["input"][i] == value) cur = i;
  }
  (*doc)["cur0"] = cur;
}

/* the input page of the volume: put the cursor on the volume of sys.txt */
static void patch_volume(DynamicJsonDocument* doc, int value) {
  int num = (*doc)["inp_num"];
  for (int i = 0; i < num; ++i) {
    if ((int)(*doc)["input"][i] == value) (*doc)["cur0"] = i;
  }
}

/* call it after updateJson(), before sending the page to Subcore */
void patchPage(DynamicJsonDocument* doc, DynamicJsonDocument* sys) {
  patched_id = -1;
  int id = (*doc)["id"];
  const char* label = (*doc)["res_label"];
  if (label == NULL) return;
  bool chnm_page = (strcmp(label, "chnm") == 0);
  bool rech_page = (strcmp(label, "rech") == 0);
  bool filt_page = (strcmp(label, "lpf") == 0 || strcmp(label, "hpf") == 0);
  bool pvol_page = (strcmp(label, "pvol") == 0);
  bool line_page = (strcmp(label, "line") == 0);
  if (!chnm_page && !rech_page && !filt_page && !pvol_page && !line_page
      && id != REC_CH_PAGE && id != REC_DCH_PAGE) return;

  if (!sys->containsKey("rate")) readSysprop(g_sid, sys);
  int rate = (*sys)["rate"];
  int chnm = (*sys)["chnm"];
  int ch   = (*sys)["ch"];
  int rech = (*sys)["rech"] | 1;
  if (filt_page) {
    patch_filter(doc, rate, (*sys)[label]);
  } else if (line_page) {
    patch_line(doc, (*sys)["src"] | SRC_MIC, (*sys)["line"]);
  } else if (pvol_page) {
    patch_volume(doc, (*sys)["pvol"] | PLAY_VOL_INIT);
  } else if (chnm_page) {
    patch_menu(doc, is_hirate(rate) ? HIRATE_MAX_CHNM : 0, chnm);
  } else if (rech_page) {
    patch_menu(doc, max_rech(rate, chnm), rech);
  } else if (id == REC_CH_PAGE) {
    patch_menu(doc, chnm, ch & 0x00ff);
  } else {
    patch_dmenu(doc, chnm, ch & 0x00ff, ch >> 8);
  }
}

/* tell Subcore the input source and if CH2 is missing: call it after patchPage() */
/* src: SRC_MIC, SRC_FILE, SRC_FILE_SOUND or SRC_FILE_ERROR, mono: 1 if the page shows CH2 of a mono file */
void patchSource(DynamicJsonDocument* doc, DynamicJsonDocument* sys) {
  if (!sys->containsKey("rate")) readSysprop(g_sid, sys);
  int src   = (*sys)["src"] | SRC_MIC;
  int pfile = (*sys)["pfil"] | 0;
  int id    = (*doc)["id"];
  int type  = (*doc)["type"];
  int pcmp  = (*sys)["pcmp"] | 0;
  if (id == PLAY_FILE_PAGE) play_list_files(doc, pfile);
  if (id == PLAY_CMP_PAGE)  play_list_compare(doc, pfile, pcmp);

  int mono = 0;
  if (src != SRC_MIC) {
    struct PlayInfo info;
    if (!play_probe(pfile, &info)) {
      src = SRC_FILE_ERROR;
    } else if (info.chnm == 1 && !(src == SRC_FILE && play_pair_ok(pfile, pcmp))) {
      switch (type) {
      case PAGE_TYPE_FFT2: case PAGE_TYPE_LSJU: case PAGE_TYPE_WVWV:
      case PAGE_TYPE_FTLG: case PAGE_TYPE_FFDB:
        mono = 1;
        break;
      }
    }
  }
  (*doc)["src"]  = src;
  (*doc)["mono"] = mono;
}

/* put back the number of items before writing the page file */
static void unpatchPage(DynamicJsonDocument* doc) {
  int id = (*doc)["id"];
  if (id != patched_id) return;
  if (doc->containsKey("inp_num")) {
    (*doc)["inp_num"] = patched_num[0];
  } else if (doc->containsKey("item_num")) {
    (*doc)["item_num"] = patched_num[0];
  } else {
    (*doc)["item0_num"] = patched_num[0];
    (*doc)["item1_num"] = patched_num[1];
  }
  patched_id = -1;
}


void update_system_properties(int8_t sid, DynamicJsonDocument* sys) {
  int ch, line;
  /* update System Properties */
  g_sid   = sid;              // MPLog("sid : %d\n", g_sid);
  g_sens  = (*sys)["sens"];   // MPLog("sens: %d\n", g_sens);
  g_gain  = (*sys)["gain"];   // MPLog("gain: %d\n", g_gain);
  g_chnm  = (*sys)["chnm"];   // MPLog("chnm: %d\n", g_chnm);
  ch      = (*sys)["ch"];     // 
  g_ch0   = ch & 0x00ff;      // MPLog("ch0 : %d\n", g_ch0);
  g_ch1   = ch >> 8;          // MPLog("ch1 : %d\n", g_ch1);
  g_rate  = (*sys)["rate"];   // MPLog("rate: %ld\n", g_rate);
  line    = (*sys)["line"];   //
  g_samp  = line*2.56;        // MPLog("samp: %d\n", g_samp);
  g_lpf   = (*sys)["lpf"];    // MPLog("lpf : %d\n", g_lpf);
  g_hpf   = (*sys)["hpf"];    // MPLog("hpf : %d\n", g_hpf);
  /* sys.txt of the older version may have a cutoff the rate can't use */
  if (!filter_ok(g_lpf, g_rate)) g_lpf = 0;
  if (!filter_ok(g_hpf, g_rate)) g_hpf = 0;
  g_win   = (*sys)["win"];    MPLog("win : %d\n", g_win);
  g_rech  = (*sys)["rech"] | 1; // 1 if not set yet
  g_src   = (*sys)["src"] | SRC_MIC;
  g_pfile = (*sys)["pfil"] | 0;
  g_pcmp  = (*sys)["pcmp"] | 0;
  /* pvol is the volume in dB: vol of the older page was the attenuation */
  if (sys->containsKey("pvol")) g_vol = (*sys)["pvol"];
  else                          g_vol = -((*sys)["vol"] | -PLAY_VOL_INIT);
  if (g_vol < PLAY_VOL_MIN) g_vol = PLAY_VOL_MIN;
  if (g_vol > PLAY_VOL_MAX) g_vol = PLAY_VOL_MAX;
  /* the long FFT (more than 1600 lines) is only for the WAV file without the sound */
  if (g_src != SRC_FILE && g_samp > MAX_FIFO_BUFF) g_samp = MAX_FIFO_BUFF;
}

/*
 * The setting files (sys.txt and the pages AAnnn.txt) are in CFG_DIR.
 * A file is written to name.tmp first and then renamed to name, so that a reset
 * while writing leaves the old or the new file: if name is missing or broken,
 * name.tmp is read and renamed (the write was cut between remove and rename).
 */
#ifdef USE_SD_CARD
#define CFG_FS     theSD
#define CFG_MOUNT  "/mnt/sd0/"
#else
#define CFG_FS     Flash
#define CFG_MOUNT  "/mnt/spif/"
#endif
#define CFG_DIR    "config"

static String cfg_path(const String& name) {
  return String(CFG_DIR "/") + name;
}

static String cfg_tmp(const String& name) {
  String tmp = cfg_path(name);
  int dot = tmp.lastIndexOf('.');
  if (dot > 0) tmp = tmp.substring(0, dot);
  return tmp + ".tmp";
}

/* replace the file path with the file tmp */
static bool cfg_replace(const String& tmp, const String& path) {
  if (CFG_FS.exists(path)) CFG_FS.remove(path);
  String from = String(CFG_MOUNT) + tmp;
  String to   = String(CFG_MOUNT) + path;
  if (rename(from.c_str(), to.c_str()) != 0) {
    MPLog("Cannot rename %s to %s (errno %d)\n", tmp.c_str(), path.c_str(), errno);
    return false;
  }
  return true;
}

static bool cfg_load(const String& path, DynamicJsonDocument* doc) {
  if (!CFG_FS.exists(path)) {
    MPLog("%s is not found (errno %d)\n", path.c_str(), errno);
    return false;
  }
  File f = CFG_FS.open(path, FILE_READ);
  if (!f) {
    MPLog("Cannot open %s (errno %d)\n", path.c_str(), errno);
    return false;
  }
  String str;
  while (f.available()) str += char(f.read());
  f.close();
  if (str.length() == 0) {
    MPLog("%s is empty\n", path.c_str());
    return false;
  }
  DeserializationError error = deserializeJson(*doc, str);
  if (error) {
    MPLog("deserializeJson() failed: %s (%s)\n", error.c_str(), path.c_str());
    return false;
  }
  return true;
}

/* the SD card does not answer any more (ETIMEDOUT, EIO): it can't be mounted again, so restart */
/* instead of stopping with the LED (a missing file is ENOENT and stops with the LED as before)  */
static void cfg_restart_on_sd_error(int err) {
  if (!g_booted) return;  /* not at boot: a broken card would restart again and again */
  if (err != ETIMEDOUT && err != EIO) return;
  MPLog("SD card error (errno %d): restart\n", err);
  usleep(200 * 1000);
  boardctl(BOARDIOC_RESET, 0);
}

static bool cfg_read(const String& name, DynamicJsonDocument* doc) {
  String path = cfg_path(name);
  if (cfg_load(path, doc)) return true;
  String tmp = cfg_tmp(name);
  if (!cfg_load(tmp, doc)) {
    cfg_restart_on_sd_error(errno);
    return false;
  }
  MPLog("Recovered %s from %s\n", path.c_str(), tmp.c_str());
  cfg_replace(tmp, path);
  return true;
}

static bool cfg_write(const String& name, DynamicJsonDocument* doc) {
  String path = cfg_path(name);
  String tmp  = cfg_tmp(name);
  if (CFG_FS.exists(tmp)) CFG_FS.remove(tmp);
  File f = CFG_FS.open(tmp, FILE_WRITE);
  if (!f) {
    int err = errno;
    MPLog("Cannot open %s (errno %d)\n", tmp.c_str(), err);
    cfg_restart_on_sd_error(err);
    return false;
  }
  size_t wsize = serializeJson(*doc, f);
  f.close();
  if (wsize == 0) {
    MPLog("serializeJson() failed: %s\n", tmp.c_str());
    return false;
  }
  return cfg_replace(tmp, path);
}

/* LED0 (FILE_ERROR) if the file is missing, LED1 (JSON_ERROR) if it is broken */
static int cfg_error(const String& name) {
  bool found = CFG_FS.exists(cfg_path(name)) || CFG_FS.exists(cfg_tmp(name));
  return found ? JSON_ERROR : FILE_ERROR;
}

static bool is_cfg_file(const char* base) {
  if (strcmp(base, "sys.txt") == 0) return true;
  /* AAnnn.txt (some pages have 4 digits) */
  int len = strlen(base);
  return len >= 9 && strncmp(base, "AA", 2) == 0 && strcmp(base + len - 4, ".txt") == 0;
}

/* move the setting files on the root (the old place, or copied there to update) to CFG_DIR */
/* call it after the storage begins */
void cfg_migrate() {
  if (!CFG_FS.exists(CFG_DIR)) {
    MPLog("Make %s\n", CFG_DIR);
    CFG_FS.mkdir(CFG_DIR);
  }
  /* collect the names first: renaming while reading the directory may skip files */
  String names;
  File root = CFG_FS.open("/");
  if (!root) return;
  while (true) {
    File e = root.openNextFile();
    if (!e) break;
    bool dir = e.isDirectory();
    const char* path = e.name();
    const char* base = strrchr(path, '/');
    base = (base == NULL) ? path : base + 1;
    if (!dir && is_cfg_file(base)) names += String(base) + "\n";
    e.close();
  }
  root.close();

  int moved = 0;
  int start = 0;
  while (start < (int)names.length()) {
    int end = names.indexOf('\n', start);
    String name = names.substring(start, end);
    start = end + 1;
    if (cfg_replace(name, cfg_path(name))) ++moved;
  }
  if (moved > 0) MPLog("Moved %d setting files to %s\n", moved, CFG_DIR);
}

/*
 * The data files are in the folders on the SD card: REC_DIR, CSV_DIR and CAP_DIR.
 * The old version put them on the root, so they are moved at boot.
 */
#define DATA_MOUNT "/mnt/sd0/"

/* make the folder if it is missing (the card may be changed after boot) */
void data_dir(const char* dir) {
  if (!theSD.exists(dir)) {
    MPLog("Make %s\n", dir);
    theSD.mkdir(dir);
  }
}

/* the folder of a data file on the root, NULL if it is not a data file */
static const char* data_dir_of(const char* base) {
  int len = strlen(base);
  if (len == 12 && strncmp(base, "REC", 3) == 0 && strcmp(base + 8, ".WAV") == 0) return REC_DIR;
  if (len == 12 && strncmp(base, "SCR", 3) == 0 && strcmp(base + 8, ".BMP") == 0) return CAP_DIR;
  if (len > 4 && strcmp(base + len - 4, ".csv") == 0) return CSV_DIR;
  return NULL;
}

/* move the data files on the root to their folders: call it after the SD card begins */
void data_migrate() {
  data_dir(REC_DIR);
  data_dir(CSV_DIR);
  data_dir(CAP_DIR);
  /* collect the names first: renaming while reading the directory may skip files */
  String names;
  File root = theSD.open("/");
  if (!root) return;
  while (true) {
    File e = root.openNextFile();
    if (!e) break;
    bool dir = e.isDirectory();
    const char* path = e.name();
    const char* base = strrchr(path, '/');
    base = (base == NULL) ? path : base + 1;
    if (!dir && data_dir_of(base) != NULL) names += String(base) + "\n";
    e.close();
  }
  root.close();

  int moved = 0;
  int start = 0;
  while (start < (int)names.length()) {
    int end = names.indexOf('\n', start);
    String name = names.substring(start, end);
    start = end + 1;
    String path = String(data_dir_of(name.c_str())) + "/" + name;
    /* the data is not replaced: keep the file on the root if the folder has the same name */
    if (theSD.exists(path)) {
      MPLog("%s is already in the folder: kept on the root\n", name.c_str());
      continue;
    }
    String from = String(DATA_MOUNT) + name;
    String to   = String(DATA_MOUNT) + path;
    if (rename(from.c_str(), to.c_str()) != 0) {
      MPLog("Cannot rename %s to %s (errno %d)\n", name.c_str(), path.c_str(), errno);
      continue;
    }
    ++moved;
  }
  if (moved > 0) MPLog("Moved %d data files to the folders\n", moved);
}

void readSysprop(int8_t sid, DynamicJsonDocument* doc) {
  sfile = "sys.txt";
  MPLog("Open system file : %s\n", sfile.c_str());
  if (!cfg_read(sfile, doc)) {
    MPLog("Cannot read %s\n", sfile.c_str());
    int err = cfg_error(sfile);
    while (true) {
      error_notifier(err);
    }
  }
  normalize_sysprop(doc);
  update_system_properties(sid, doc);
}

void updateSysprop(int8_t sid, DynamicJsonDocument* doc, struct Response* res) {
  
  if (res == NULL) return;
  /* a negative value means "no value" (-1 of the menus), but the volume is negative in dB */
  if (res->value < 0 && strcmp(res->label, "pvol") != 0) return;

  /* Read the current system property at first */
  sfile = "sys.txt";
  MPLog("Open system file : %s\n", sfile.c_str());
  if (!cfg_read(sfile, doc)) {
    MPLog("Cannot read %s\n", sfile.c_str());
    int err = cfg_error(sfile);
    while (true) error_notifier(err);
  }

  /* update the system properties according to the response */
  char* label = res->label;
  int   value = res->value;
  MPLog("Update System Property File : %s\n", sfile.c_str());
  MPLog("%s : %d\n", label, value);

  int8_t appid = (*doc)["id"];
  appid /= 100;
  (*doc)["app"] = appid;
  (*doc)[label] = value;
  if (strcmp(label, "pfil") == 0) (*doc)["pcmp"] = 0;  /* select the file to compare again */
  normalize_sysprop(doc);
  if (!cfg_write(sfile, doc)) {
    while (true) {
      error_notifier(FILE_ERROR);
    }
  }
  
  update_system_properties(sid, doc);
}

DynamicJsonDocument* updateJson(DynamicJsonDocument* doc, struct Response* res) {
 
  if (res == NULL) {
    // MPLog("Response is null. Open the home menu\n");
    dfile = "AA000.txt";
  } else {
    int cur0 = res->cur0;
    int cur1 = res->cur1;
    int id = res->next_id;

    /* update the document properties according to the response */
    MPLog("Update json file : %s\n", dfile.c_str());
    unpatchPage(doc);
    doc->remove("src");   /* added by patchSource() */
    doc->remove("mono");
    (*doc)["cur0"] = cur0;
    (*doc)["cur1"] = cur1;
    if (res->has_amp) (*doc)["amp"] = res->amp;  /* restored when the page is shown again */
    if (!cfg_write(dfile, doc)) {
      while (true) {
        error_notifier(FILE_ERROR);
      }
    }
    char fname[12] = {0};
    memset(fname, '\0', sizeof(char)*12);
    sprintf(fname, "AA%03d.txt", id);
    dfile = String(fname);
  }
    
  /* Open the new json file for the request to change page from Subcore */
  MPLog("Open json file : %s\n", dfile.c_str());
  if (!cfg_read(dfile, doc)) {
    MPLog("Cannot read %s\n", dfile.c_str());
    int err = cfg_error(dfile);
    while (true) {
      error_notifier(err);
    }
  }
  return doc;
}
