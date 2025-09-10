#pragma once

#include <regex>
#include "ofMain.h"
#include "ofxTrueTypeFontUC.h"
#include "ofxOsc.h"
#include "ofxAruco.h"
#include "highlyreliablemarkers.h"
#include "ofxZxing.h"
#include "ofxJoystick.h"
#include "ofxXmlSettings.h"
#include "ofFileUtils.h" // Add this line
#include "ofxOpenCv.h"

/* ---------- definitions ---------- */

// system
#define APP_VER         "v1.0.2beta1"

#define DEBUG_ENABLED   false
#define HELP_LINES      35  // must be <= OVLTXT_LINES
#define SCENE_INIT      0
#define SCENE_CAMS      1
#define SCENE_MAIN      2
#ifdef TARGET_OSX
#define TVP_KEY_ALT     OF_KEY_COMMAND
#define TVP_STR_ALT     "command"
#else /* TARGET_OSX */
#define TVP_KEY_ALT     OF_KEY_ALT
#define TVP_STR_ALT     "Alt"
#endif /* TARGET_WIN32 TARGET_LINUX */
#define DFLT_SYS_STAT   false
#define TVP_VAL_PLUS    1
#define TVP_VAL_MINUS   -1

// settings
#define SETTINGS_FILE   "settings.xml"
#define SNM_SYS_STAT    "system:sysStat"
#define SNM_DTCTALL_FRM "system:allFrameDetect"
#define SNM_VIEW_FLLSCR "view:fullscreen"
#define SNM_VIEW_CAMTRM "view:camTrim"
#define SNM_VIEW_CAMFRM "view:camFrame"
#define SNM_LOG_ENABLED "system:logEnabled"

// OSC
#define SNM_OSC_SENDER_HOST "osc:host"
#define SNM_OSC_SENDER_PORT "osc:port"
#define SNM_OSC_RECEIVER_PORT "osc:receivePort"
#define SNM_ARUCO_MIN_SIZE "aruco:minSize"
#define SNM_FLICKER_LENGTH "aruco:flickerLength"

// AR lap timer
#define ARAP_MODE_NORM  0
#define ARAP_MODE_MIDDLE 1
#define ARAP_MODE_LOOSE 2
#define ARAP_MODE_ULOOSE 3
#define DFLT_ARAP_MODE  ARAP_MODE_NORM

// pilots
#define PILOTS_FILE     "pilots/pilots.xml"
#define PLT_PILOT_LABEL "pilot:label_"

// camera profile
#define CAM_FPV_FILE    "camera/fpv.xml"
#define CFNM_NAME       "camera:name"
#define CFNM_CAMNUM     "camera:camnum"
#define CFNM_GRAB_W     "camera:grab:width"
#define CFNM_GRAB_H     "camera:grab:height"
#define CFNM_CROP_X     "camera:crop:x"
#define CFNM_CROP_Y     "camera:crop:y"
#define CFNM_CROP_W     "camera:crop:width"
#define CFNM_CROP_H     "camera:crop:height"
#define CFNM_DRAW_ASPR  "camera:draw:aspectRatio"

// color
#define COLOR_YELLOW    255,215,0
#define COLOR_WHITE     255,255,255
#define COLOR_LGRAY     127,127,127
#define COLOR_DGRAY     15
#define COLOR_BG_DARK   0,0,0,223
#define COLOR_BG_MIDDLE 0,0,0,127
#define COLOR_BG_LIGHT  0,0,0,31
#define COLOR_ALERT     255,0,0
// view
#define FRAME_RATE      60
#define MOVE_STEPS      10
#define VERTICAL_SYNC   true
#define LOGO_LARGE_FILE "system/logo_large.png"
#define LOGO_SMALL_FILE "system/logo_small.png"
#define DFLT_WALL_FILE  "system/background.png"
#define DFLT_ICON_FILE  "system/pilot_icon.png"
#define CAMERA_MAXNUM   4
#define CAMERA_WIDTH    640
#define CAMERA_HEIGHT   480
#define CAMERA_RATIO    1.3333
#define FONT_P_FILE     "system/GenShinGothic-P-Bold.ttf"
#define FONT_M_FILE     "system/GenShinGothic-Monospace-Bold.ttf"
#define ICON_DIR        "pilots/"
#define ICON_WIDTH      50
#define ICON_HEIGHT     50
#define ICON_MARGIN_X   20
#define ICON_MARGIN_Y   0
#define NUMBER_HEIGHT   20
#define NUMBER_MARGIN_X 1
#define NUMBER_MARGIN_Y 35
#define LABEL_HEIGHT    30
#define LABEL_MARGIN_X  75
#define LABEL_MARGIN_Y  40
#define BASE_MARGIN_X   0
#define BASE_MARGIN_Y   0
#define BASE_WIDTH      20
#define BASE_HEIGHT     50
#define BASE_1_COLOR    201,58,64
#define BASE_2_COLOR    160,194,56
#define BASE_3_COLOR    0,116,191
#define BASE_4_COLOR    248,128,23
#define LAP_HEIGHT      20
#define LAP_MARGIN_X    20
#define LAP_MARGIN_Y    80
#define LAPHIST_MD_OFF  0
#define LAPHIST_MD_IN   1
#define LAPHIST_MD_OUT  2
#define LAPHIST_HEIGHT  15
#define LAPHIST_MARGIN  8
#define FRAME_LINEWIDTH 8
#define DFLT_FSCR_ENBLD false
#define DFLT_CAM_TRIM   false
#define DFLT_CAM_LAPHST LAPHIST_MD_OFF
#define DFLT_CAM_FRAMED false
#define ALIGN_LEFT      0
#define ALIGN_CENTER    1
#define ALIGN_RIGHT     2
#define HIDECUR_TIME    FRAME_RATE
// overlay
#define OVLMODE_NONE    0
#define OVLMODE_HELP    1
#define OVLMODE_MSG     2
#define OVLMODE_RCRSLT  3
#define OVLTXT_BLKS     13
#define OVLTXT_LINES    35
#define OVLTXT_LAPS     25
#define OVLTXT_MARG     10
#define OLVMSG_TIME     FRAME_RATE
#define INFO_HEIGHT     10
#define WATCH_HEIGHT    40
#define WATCH_OFFSET_Y  10

// OSC
#define OSC_SENDER_DEFAULT_HOST "localhost"
#define OSC_SENDER_DEFAULT_PORT 8000
#define OSC_RECEIVER_DEFAULT_PORT 8001
// AR lap timer
#define ARAP_MODE_NORM  0
#define ARAP_MODE_MIDDLE 1
#define ARAP_MODE_LOOSE 2
#define ARAP_MODE_ULOOSE 3
#define DFLT_ARAP_MODE  ARAP_MODE_NORM
#define DFLT_ARAP_RLAPS 10
#define DFLT_ARAP_RSECS 0
#define DFLT_ARAP_MNLAP 3
#define DFLT_ARAP_SGATE false
#define DFLT_ARAP_LAPTO false
#define ARAP_MKR_FILE   "system/marker.xml"
#define ARAP_RESULT_DIR "results/"
#define ARAP_MAX_RLAPS  10000
#define ARAP_MAX_MNLAP  100
#define ARAP_MAX_RSECS  36000
#define ARAP_RSLT_SCRN  0
#define ARAP_RSLT_FILE  1
#define ARAP_RSLT_DELAY (FRAME_RATE * 3)
#define ARAP_RECT_LINEW 5
#define WATCH_COUNT_SEC 5
#define DTCT_ALL_FRAME  false

/* ---------- classes ---------- */

class tvpCamView {
public:
    // camera
    int moveSteps;
    int width;
    int height;
    int heightWide;
    int widthTarget;
    int heightTarget;
    int heightWideTarget;
    int posX;
    int posY;
    int posYWide;
    int posXTarget;
    int posYTarget;
    int posYWideTarget;
    int grabW;
    int grabH;
    int cropX;
    int cropY;
    int cropW;
    int cropH;
    float imageScale;
    bool needCrop;
    bool needResize;
    bool isWide;
    ofxCvColorImage resizedImage;

    // lap
    int lapPosX;
    int lapPosY;
    int lapPosXTarget;
    int lapPosYTarget;
    // AR lap timer
    ofxAruco aruco;
    int foundMarkerNum;
    int foundValidMarkerNum;
    bool enoughMarkers;
    int flickerCount;
    int flickerValidCount;
    float flickerEndtime;
    float flickerValidEndtime;
    bool rssiOutput;
    // lap timer
    int lastValidMarkerId; // Changed from vector to int
    bool isDroneInGate;
    float loopTime;  // Added: Time taken for this camera's processing (milliseconds)
    int frequency;
};


class tvpCamProf {
public:
    bool enabled;
    int camnum;
    string name;
    int grabW;
    int grabH;
    int cropX;
    int cropY;
    int cropW;
    int cropH;
    string drawAspr;
    bool needCrop;
    bool needResize;
    bool isWide;
};

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void exit();

    // New functions for frequency management
    void loadFrequencies();
    void saveFrequencies();
    int calculate_pseudo_rssi(int camIndex);
    void onStageReady(ofxOscMessage& m); // Added

    // Main setup and race logic
    void setupMain();
    void initRaceVars();
    void startRace();

    // Splash and Camera Check
    void setupInit();
    void setupCamCheck();
    void updateInit();
    void updateCamCheck();
    void drawInit();
    void drawCamCheck();
    void reloadCameras();

    // Configuration and System
    void initConfig();
    void toggleSysStat();
    void toggleFullscreen();

    // Overlay and UI
    void setOverlayMode(int mode);
    void loadOverlayFont();
    void initOverlayMessage();
    void setOverlayMessage(string msg);
    void drawOverlayMessage();
    void drawOverlayMessageCore(ofxTrueTypeFontUC *font, string msg);
    void drawHelp();
    void drawHelpBody(int line);
    void drawStringBlock(ofxTrueTypeFontUC* font, string text, int xblock, int yline, int align, int blocks, int lines);
    void drawLineBlock(int xblock1, int xblock2, int yline, int blocks, int lines);
    void drawULineBlock(int xblock1, int xblock2, int yline, int blocks, int lines);

    // Drawing and View
    void drawCameraImage(int camidx);
    void drawCameraARMarker(int idx, bool isSub);
    void drawCamera(int idx);
    void drawInfo();
    void drawStringWithShadow(ofxTrueTypeFontUC* font, ofColor color, ofColor bgcolor, string str, int x, int y);
    void setViewParams();
    int calcViewParam(int target, int current, int steps);
    void updateViewParams();
    void grabberUpdateResize(int cidx);
    void grabberUpdateResizeMulti();
    void setupColors();
    void activateCursor();

    // Input Handlers
    void keyPressedOverlayHelp(int key);
    void keyPressedOverlayMessage(int key);
    void keyPressedOverlayNone(int key);
    void keyPressedCamCheck(int key);

    //Aruco
    void setMinDetectSize();

private:
    // system
    int camCheckCount;
    int tvpScene;
    bool resourcesAllocated;
    bool sysStatEnabled;
    bool logEnabled;

    // view
    ofVideoGrabber grabber[CAMERA_MAXNUM];
    ofColor myColorYellow, myColorWhite, myColorLGray, myColorDGray, myColorAlert;
    ofColor myColorBGDark, myColorBGMiddle, myColorBGLight;
    ofxTrueTypeFontUC myFontNumber, myFontLabel, myFontLap, myFontLapHist;
    ofxTrueTypeFontUC myFontNumberSub, myFontLabelSub, myFontLapSub;
    ofxTrueTypeFontUC myFontInfo1m, myFontInfo1p, myFontInfoWatch;
    ofImage logoLargeImage, logoSmallImage;
    ofImage wallImage;
    ofxCvColorImage multicamSource;
    float wallRatio;
    int wallDrawWidth;
    int wallDrawHeight;
    tvpCamView camView[CAMERA_MAXNUM];
    int cameraNum;
    int cameraNumVisible;
    bool cameraTrimEnabled;
    bool fullscreenEnabled;
    bool cameraFrameEnabled;
    int hideCursorTimer;
    bool isMultiView;
    // AR lap timer
    ofSoundPlayer beepSound, beep3Sound, notifySound, cancelSound;
    ofSoundPlayer countSound, finishSound;
    ofFile resultsFile;
    bool raceStarted;
    float elapsedTime;
    int raceResultTimer;
    bool frameTick;
    int flickerLength;

    // overlay
    ofxTrueTypeFontUC myFontOvlayP, myFontOvlayP2x, myFontOvlayM;
    int overlayMode;
    int ovlayMsgTimer;
    string ovlayMsgString;
    bool gateDetectAllFrames;

    // OSC Communication
    ofxOscSender oscSender;
    ofxOscReceiver oscReceiver; // OSC Receiver
    int oscReceivePort;    // OSC Receive Port
    string oscSenderHost; // OSC Receive Host
    int oscSenderPort;    // OSC Receive Port

    // Frequency settings
    ofxXmlSettings frequencyXml;
    std::map<int, int> cameraFrequencyMap;
    std::vector<int> activeCameraIndices;

    // Race start time for lap_time calculation
    float raceStartTime;

    // Heartbeat timer
    float lastHeartbeatTime;

    // Settings and Camera Profile
    ofxXmlSettings xmlSettings;
    ofxXmlSettings xmlCamProfFpv;
    ofxXmlSettings xmlPilots;
    tvpCamProf camProfFpvExtra;

    // ArUco settings
    int arucoMinSize;
    int arLapMode;
    int arMarkerNumThreshold;
    int flickerThreshold;

    // Custom functions for settings and camera profiles
    void loadSettingsFile();
    void loadCameraProfileFile();
    void saveSettingsFile();
    void toggleLog();
    void toggleARLap();
    void toggleGateDetectFrequency(); // Add this line
    void changeFlickerLength(int val);
    void changeArucoMinSize(int val);
    void updateArLapModeSettings();
};
