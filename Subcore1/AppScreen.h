#ifndef __APP_SCREEN_HEADER_GURAD__
#define __APP_SCREEN_HEADER_GURAD__

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define TITLE_DECO_LINE (35)
#define TITLE_SIDE (10)
#define TITLE_HEAD (10)

/* BUTTON CORDINATION */
#define BUTTON_DECO_LINE (SCREEN_HEIGHT-30)
#define BUTTON_HEAD    (220)
#define BUTTON0_SIDE   (20)
#define BUTTON1_SIDE   (80)
#define BUTTON2_SIDE   (160)
#define BUTTON3_SIDE   (250)


/* MENU SCREEN CORDINATION */
#define MENU_ITEM_SIDE  (40)
#define MENU_ITEM0_HEAD (55)
#define MENU_CUR_SIDE    TITLE_SIDE
#define MENU_CUR_SIZE   (15)
#define MENU_ITEM1_HEAD (MENU_ITEM0_HEAD+40)
#define MENU_ITEM2_HEAD (MENU_ITEM1_HEAD+40)
#define MENU_ITEM3_HEAD (MENU_ITEM2_HEAD+40)

/* CH STATUS CORDINATION */
#define DISP_CH_SIDE   (270)
#define DISP_CH0_HEAD   MENU_ITEM0_HEAD
#define DISP_CH1_HEAD   MENU_ITEM1_HEAD
#define DISP_CH2_HEAD   MENU_ITEM2_HEAD
#define DISP_CH3_HEAD   MENU_ITEM3_HEAD

/* INPUT SCREEN CORDINATION */
#define INPUT_MARGIN (5)
#define INPUT_LABEL_HEAD  MENU_ITEM0_HEAD
#define INPUT_LABEL_SIDE  TITLE_SIDE
#define INPUT_BOX_SIDE   (MENU_ITEM_SIDE-INPUT_MARGIN)  
#define INPUT_BOX_HEAD   (MENU_ITEM1_HEAD-INPUT_MARGIN) 
#define INPUT_ELEM_SIDE   MENU_ITEM_SIDE
#define INPUT_ELEM_HEAD   MENU_ITEM1_HEAD
#define INPUT_BOX_WIDTH  (100)
#define INPUT_BOX_HEIGHT (30)
#define INPUT_UNIT_SIDE  (150)
#define INPUT_UNIT_HEAD   MENU_ITEM1_HEAD


/* MENU SCREEN CORDINATION */
#define DMENU_CUR_SIDE0   MENU_CUR_SIDE
#define DMENU_CUR_SIDE1  (MENU_CUR_SIDE+130)
#define DMENU_CUR_SIZE    MENU_CUR_SIZE
#define DMENU_ITEM_SIDE0  MENU_ITEM_SIDE
#define DMENU_ITEM_SIDE1 (DMENU_CUR_SIDE1+30)
#define DMENU_ITEM0_HEAD  MENU_ITEM0_HEAD
#define DMENU_ITEM1_HEAD  MENU_ITEM1_HEAD
#define DMENU_ITEM2_HEAD  MENU_ITEM2_HEAD
#define DMENU_ITEM3_HEAD  MENU_ITEM3_HEAD


/* SENSOR SCREEN CORDINATION */
#define MON_MARGIN (5)
#define MON_ELEM_SIDE   MENU_ITEM_SIDE
#define MON_ELEM0_HEAD (TITLE_HEAD+50)
#define MON_ELEM1_HEAD (MON_ELEM0_HEAD+50)
#define MON_ELEM2_HEAD (MON_ELEM1_HEAD+50)
#define MON_BOX_SIDE   (MON_ELEM_SIDE-MON_MARGIN)  
#define MON_BOX0_HEAD  (MON_ELEM0_HEAD-MON_MARGIN) 
#define MON_BOX1_HEAD  (MON_ELEM1_HEAD-MON_MARGIN) 
#define MON_BOX2_HEAD  (MON_ELEM2_HEAD-MON_MARGIN) 
#define MON_BOX_WIDTH  (100)
#define MON_BOX_HEIGHT (30)
#define MON_UNIT_SIDE  (150)
#define MON_UNIT0_HEAD  MON_BOX0_HEAD+MON_BOX_HEIGHT/2
#define MON_UNIT1_HEAD  MON_BOX1_HEAD+MON_BOX_HEIGHT/2
#define MON_UNIT2_HEAD  MON_BOX2_HEAD+MON_BOX_HEIGHT/2

/* FFT 2-WAY SCREEN CORDINATION */
#define FFT_MARGIN (1)
#define FFT_GRAPH_WIDTH  (256)
#define FFT_GRAPH_HEIGHT (64)
#define FFT_GRAPH_SIDE    FFT_MARGIN 
#define FFT_GRAPH0_HEAD  (45 + FFT_MARGIN)
#define FFT_GRAPH1_HEAD  (FFT_GRAPH0_HEAD + FFT_GRAPH_HEIGHT + 18)
#define FFT_BOX_SIDE     (FFT_GRAPH_SIDE  - FFT_MARGIN)  
#define FFT_BOX0_HEAD    (FFT_GRAPH0_HEAD - FFT_MARGIN) 
#define FFT_BOX1_HEAD    (FFT_GRAPH1_HEAD - FFT_MARGIN) 
#define FFT_UNIT0_SIDE    TITLE_SIDE
#define FFT_UNIT1_SIDE    FFT_UNIT0_SIDE + 32
#define FFT_UNIT2_SIDE    FFT_UNIT1_SIDE + 32
#define FFT_UNIT3_SIDE    FFT_UNIT2_SIDE + 32
#define FFT_UNIT4_SIDE    FFT_UNIT3_SIDE + 32
#define FFT_UNIT5_SIDE    FFT_UNIT4_SIDE + 32
#define FFT_UNIT6_SIDE    FFT_UNIT5_SIDE + 32
#define FFT_UNIT7_SIDE    FFT_UNIT6_SIDE + 32
#define FFT_UNIT0_HEAD    FFT_GRAPH0_HEAD + FFT_BOX_HEIGHT + 2
#define FFT_UNIT1_HEAD    FFT_GRAPH1_HEAD + FFT_BOX_HEIGHT + 2

#define FFT_MODE_WAV_FFT  (1)
#define FFT_MODE_FFT_FFT  (2)
#define FFT_MODE_WAV_WAV  (3)

struct Response {
  char label[5];
  int value;
  int cur0;
  int cur1;
  int next_id;
};

struct SensorData {
  float acc;
  float vel;
  float dis;
};

struct FFTData {
  float* pWav;
  float* pFft;
  int len;
  float df;
};


bool updateScreen();
void clearScreen(DynamicJsonDocument* jdoc);
void buildTitle(DynamicJsonDocument* doc);
void buildAppSign(DynamicJsonDocument* doc);
void buildInput(DynamicJsonDocument* doc);
void buildMenu(DynamicJsonDocument* doc);
void buildDMenu(DynamicJsonDocument* doc);
void buildMonitor(DynamicJsonDocument* doc);
void build2WayGraph(DynamicJsonDocument* doc);
void buildChStatus(DynamicJsonDocument* doc);
void buildButton(DynamicJsonDocument* doc);
void buildNextBackConnection(DynamicJsonDocument* doc);
void putSensorValue(float acc, float vel, float dis);
void putDraw2WayGraph(float* pWav0, int len0, float* pFft1, int len1, float df = 0.0, int mode = FFT_MODE_WAV_FFT);


#endif /* __APP_SCREEN_HEADER_GURAD__ */
