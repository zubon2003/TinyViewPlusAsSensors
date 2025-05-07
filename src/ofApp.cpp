// charcter encoding is UTF-8

#include "ofApp.h"
#ifdef TARGET_WIN32
#include <sapi.h>
#include <atlcomcli.h>
#endif /* TARGET_WIN32 */

/* ---------- variables ---------- */

// system
int camCheckCount;
int tvpScene;
ofxXmlSettings xmlSettings, xmlCamProfFpv, xmlPilots;
bool sysStatEnabled;
// view
ofVideoGrabber grabber[CAMERA_MAXNUM];
ofColor myColorYellow, myColorWhite, myColorLGray, myColorDGray, myColorAlert;
ofColor myColorBGDark, myColorBGMiddle, myColorBGLight;
ofxTrueTypeFontUC myFontNumber, myFontLabel, myFontLap, myFontLapHist;
ofxTrueTypeFontUC myFontNumberSub, myFontLabelSub, myFontLapSub;
ofxTrueTypeFontUC myFontInfo1m, myFontInfo1p, myFontInfoWatch;
ofImage logoLargeImage, logoSmallImage;
ofImage wallImage;
float wallRatio;
int wallDrawWidth;
int wallDrawHeight;
tvpCamProf camProfFpvExtra;
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
// overlay
ofxTrueTypeFontUC myFontOvlayP, myFontOvlayP2x, myFontOvlayM;
int overlayMode;
int ovlayMsgTimer;
string ovlayMsgString;

//For Rotorhazard
uint8_t serialSendByte;
float sensorStartTime[4];
float sensorEndTime[4];

ofSerial tvpSerial;
string tvpComport;

// Perform gate detection on all frames (true = all frames, false = every two frames).
bool gateDetectAllFrames;
//--------------------------------------------------------------
void setupInit() {
    // system
    ofSetEscapeQuitsApp(false);
    ofDirectory dir;
    if (dir.doesDirectoryExist("../data") == false) {
        // macOS binary release
        ofSetDataPathRoot("../Resources/data");
    }
    sysStatEnabled = DFLT_SYS_STAT;
    // scene
    tvpScene = SCENE_INIT;
    ofResetElapsedTimeCounter();
    // screen
    ofSetWindowTitle("Tiny View Plus As Rssi Sensors");
    ofBackground(0, 0, 0);
    ofSetVerticalSync(VERTICAL_SYNC);
    ofSetFrameRate(FRAME_RATE);
    myFontNumber.load(FONT_P_FILE, NUMBER_HEIGHT);
    myFontLabel.load(FONT_P_FILE, LABEL_HEIGHT);
    myFontLap.load(FONT_P_FILE, LAP_HEIGHT);
    myFontLapHist.load(FONT_P_FILE, LAPHIST_HEIGHT);
    myFontNumberSub.load(FONT_P_FILE, NUMBER_HEIGHT / 2);
    myFontLabelSub.load(FONT_P_FILE, LABEL_HEIGHT / 2);
    myFontLapSub.load(FONT_P_FILE, LAP_HEIGHT / 2);
    myFontInfo1m.load(FONT_M_FILE, INFO_HEIGHT);
    myFontInfo1p.load(FONT_P_FILE, INFO_HEIGHT);
    myFontInfoWatch.load(FONT_M_FILE, WATCH_HEIGHT);
    loadOverlayFont();
    cameraTrimEnabled = DFLT_CAM_TRIM;
    fullscreenEnabled = DFLT_FSCR_ENBLD;
    cameraFrameEnabled = DFLT_CAM_FRAMED;
    // splash
    logoLargeImage.load(LOGO_LARGE_FILE);
    // logo
    logoSmallImage.load(LOGO_SMALL_FILE);
    // view common
    setupColors();
    hideCursorTimer = HIDECUR_TIME;
    // overlay
    setOverlayMode(OVLMODE_NONE);
    initOverlayMessage();
    // AR lap timer
    beepSound.load(SND_BEEP_FILE);
    beep3Sound.load(SND_BEEP3_FILE);
    countSound.load(SND_COUNT_FILE);
    finishSound.load(SND_FINISH_FILE);
    notifySound.load(SND_NOTIFY_FILE);
    cancelSound.load(SND_CANCEL_FILE);
    raceStarted = false;
    elapsedTime = 0;
    raceResultTimer = -1;
    // extra camera
    camProfFpvExtra.enabled = false;
    //SERIAL
    serialSendByte = 0;
    //
    gateDetectAllFrames = DTCT_ALL_FRAME;
}

//--------------------------------------------------------------
void loadSettingsFile() {
    xmlSettings.loadFile(SETTINGS_FILE);

    // SYSTEM
    // COMPORT
    tvpComport = xmlSettings.getValue(TVP_COMPORT, tvpComport);
    // system statistics
    sysStatEnabled = xmlSettings.getValue(SNM_SYS_STAT, sysStatEnabled);
    // serial

    // VIEW
    // fullscreen
    fullscreenEnabled = xmlSettings.getValue(SNM_VIEW_FLLSCR, fullscreenEnabled);
    // camera view trimming
    cameraTrimEnabled = xmlSettings.getValue(SNM_VIEW_CAMTRM, cameraTrimEnabled);
    // camera frame visibility
    cameraFrameEnabled = xmlSettings.getValue(SNM_VIEW_CAMFRM, cameraFrameEnabled);

    //GATE DETECT FREQUENCY
    gateDetectAllFrames = xmlSettings.getValue(SNM_DTCTALL_FRM, gateDetectAllFrames);
}

void saveSettingsFile() {
    // SYSTEM
    // 
    xmlSettings.setValue(TVP_COMPORT, tvpComport);
    // system statistics
    xmlSettings.setValue(SNM_SYS_STAT, sysStatEnabled);

    // VIEW
    // fullscreen
    xmlSettings.setValue(SNM_VIEW_FLLSCR, fullscreenEnabled);
    // camera view trimming
    xmlSettings.setValue(SNM_VIEW_CAMTRM, cameraTrimEnabled);
    // camera frame visibility
    xmlSettings.setValue(SNM_VIEW_CAMFRM, cameraFrameEnabled);


    xmlSettings.setValue(SNM_DTCTALL_FRM, gateDetectAllFrames);

    xmlSettings.saveFile(SETTINGS_FILE);
}

//--------------------------------------------------------------
void loadCameraProfileFile() {
    if (xmlCamProfFpv.loadFile(CAM_FPV_FILE) == false) {
        return;
    }
    tvpCamProf *p = &camProfFpvExtra;
    ofxXmlSettings *s = &xmlCamProfFpv;
    // load
    p->enabled = true;
    p->camnum = s->getValue(CFNM_CAMNUM, 1);
    if (p->camnum > 1) isMultiView = true;
    else isMultiView = false;
    p->name = s->getValue(CFNM_NAME, "tvp-no-named-camera");
    p->grabW = s->getValue(CFNM_GRAB_W, CAMERA_WIDTH);
    p->grabH = s->getValue(CFNM_GRAB_H, CAMERA_HEIGHT);
    p->cropX = s->getValue(CFNM_CROP_X, 0);
    p->cropY = s->getValue(CFNM_CROP_Y, 0);
    p->cropW = s->getValue(CFNM_CROP_W, CAMERA_WIDTH);
    p->cropH = s->getValue(CFNM_CROP_H, CAMERA_HEIGHT);
    p->drawAspr = s->getValue(CFNM_DRAW_ASPR, "4:3");
    // crop?
    if (p->cropX == 0 && p->cropY == 0 && p->cropW == p->grabW && p->cropH == p->grabH) {
        p->needCrop = false;
    } else {
        p->needCrop = true;
    }
    // resize?
    if ((p->needCrop == true && p->cropW == CAMERA_WIDTH && p->cropH == CAMERA_HEIGHT)
        || (p->needCrop == false && p->grabW == CAMERA_WIDTH && p->grabH && CAMERA_HEIGHT)) {
        p->needResize = false;
    } else {
        p->needResize = true;
    }
    // wide?
    p->isWide = (p->drawAspr == "16:9");
}


//--------------------------------------------------------------
void setupCamCheck() {
    tvpScene = SCENE_CAMS;
    cameraNum = 0;
    camCheckCount = 0;
    reloadCameras();
}

//--------------------------------------------------------------
void reloadCameras() {
    // clear
    for (int i = 0; i < cameraNum; i++) {
        grabber[i].close();
    }
    // load
    ofVideoGrabber tmpgrb;
    vector<ofVideoDevice> devices = tmpgrb.listDevices();
    tvpCamProf* prof = &camProfFpvExtra;
    int cidx = 0;
    cameraNum = 0;
    ofLog() << "Scanning camera... " << devices.size() << " devices";
    if (isMultiView == false){
        for (size_t i = 0; i < devices.size(); i++) {
            int w, h, aw, ah;
            bool extra = false;
            if (prof->enabled == true && regex_search(devices[i].deviceName, regex(prof->name)) == true) {
                extra = true;
            }
            if (regex_search(devices[i].deviceName, regex("USB2.0 PC CAMERA")) == false && extra == false) {
                continue;
            }
            if (devices[i].bAvailable == false) {
                continue;
            }
            grabber[cidx].setDeviceID(devices[i].id);
            if (extra == true) {
                w = prof->grabW;
                h = prof->grabH;
            }
            else {
                w = CAMERA_WIDTH;
                h = CAMERA_HEIGHT;
            }
            if (grabber[cidx].initGrabber(w, h) == false) {
                continue;
            }
            if (extra == true) {
                camView[cidx].needCrop = prof->needCrop;
                camView[cidx].needResize = prof->needResize;
                camView[cidx].isWide = prof->isWide;
            }
            else {
                camView[cidx].needCrop = false;
                camView[cidx].needResize = false;
                camView[cidx].isWide = false;
            }
            camView[cidx].cropX = prof->cropX;
            camView[cidx].cropY = prof->cropY;
            camView[cidx].cropW = prof->cropW;
            camView[cidx].cropH = prof->cropH;
            camView[cidx].grabW = w;
            camView[cidx].grabH = h;
            aw = grabber[cidx].getWidth();
            ah = grabber[cidx].getHeight();
            ofLog() << "[" << devices[i].id << "] " << devices[i].deviceName;
            ofLog() << "  preferred resolution: " << w << " x " << h;
            ofLog() << "  actual resolution: " << aw << " x " << ah;
            if (extra == true) {
                ofLog() << "  crop: "
                    << prof->cropX << ", " << prof->cropY << ", "
                    << prof->cropW << ", " << prof->cropH;
            }
            cidx++;
            cameraNum++;
            if (cameraNum == CAMERA_MAXNUM) {
                break;
            }
        }
    }
    else {
        for (size_t i = 0; i < devices.size(); i++) {
            if (regex_search(devices[i].deviceName, regex(prof->name)) == false) continue;
            if (devices[i].bAvailable == false) continue;
            //
            cameraNum = prof->camnum;

            for (int j = 0; j <= cameraNum-1; j++) {
                grabber[j].setDeviceID(devices[i].id);
                camView[j].grabW = prof->grabW;
                camView[j].grabH = prof->grabH;
                grabber[j].initGrabber(camView[j].grabW, camView[j].grabH);
                camView[j].needCrop = true;
                camView[j].needResize = true;
                camView[j].isWide = prof->isWide;

                if (j % 2 == 0) camView[j].cropX = prof->cropX / 2;
                else camView[j].cropX = camView[j].grabW / 2 + prof->cropX / 2;
                
                if (j <= 1) camView[j].cropY = prof->cropY / 2;
                else camView[j].cropY = camView[j].grabH / 2 + prof->cropY / 2;
                
                camView[j].cropW = prof->cropW / 2;
                camView[j].cropH = prof->cropH / 2;
            }
            break;
        }
    }
    tmpgrb.close();
}

//--------------------------------------------------------------
void setupMain() {
    // system
    ofSetFullscreen(fullscreenEnabled);
    tvpScene = SCENE_MAIN;
    // camera
    cameraNumVisible = cameraNum;
    setViewParams();
    for (int i = 0; i < cameraNum; i++) {
        camView[i].moveSteps = 1;
    }
    // AR laptimer
    for (int i = 0; i < cameraNum; i++) {
        camView[i].aruco.setUseHighlyReliableMarker(ARAP_MKR_FILE);
        camView[i].aruco.setMinMaxMarkerDetectionSize(0.05, 0.25);
        camView[i].aruco.setThreaded(true);
        camView[i].aruco.setup2d(CAMERA_WIDTH, CAMERA_HEIGHT);
    }
    //For RotorHazard
    tvpSerial.setup(tvpComport, 115200);
    for (int i = 0; i < cameraNum; i++) {
        camView[i].markerDetectStrengthLast = 0b00;
        camView[i].markerOutput = 0b00;
        camView[i].markerEndTime = 0;
        camView[i].rssiOutput = false;
    }

    initRaceVars();
    startRace();
}

//--------------------------------------------------------------
void ofApp::setup() {
    setupInit();
    loadSettingsFile();
    loadCameraProfileFile();
    saveSettingsFile();
}

//--------------------------------------------------------------
void updateInit() {
    if (ofGetElapsedTimeMillis() >= 3000) {
        setupCamCheck();
    }
}

//--------------------------------------------------------------
void updateCamCheck() {
    for (int i = 0; i < cameraNum; i++) {
        grabber[i].update();
    }
    if (camCheckCount > 180) {
        reloadCameras();
        camCheckCount = 0;
        return;
    }
    camCheckCount++;
}

//--------------------------------------------------------------
void ofApp::update() {
    // scene
    if (tvpScene == SCENE_INIT) {
        updateInit();
        return;
    } else if (tvpScene == SCENE_CAMS) {
        updateCamCheck();
        return;
    }
    // timer
    if (raceStarted == true) elapsedTime = ofGetElapsedTimef();

    // camera
    if (isMultiView == true) {
        grabberUpdateResizeMulti();
    }
    else {
        for (int i = 0; i < cameraNum; i++) {
            grabberUpdateResize(i);
        }
    }


    // lap
    frameTick = !frameTick;
    if (gateDetectAllFrames == true) frameTick = true;

    if (frameTick == true) {
        serialSendByte = 0;
        for (int i = 0; i < cameraNum; i++) {
            // AR lap timer
            if (camView[i].needCrop == true || camView[i].needResize == true) {
                camView[i].aruco.detectMarkers(camView[i].resizedPixels);
            }
            else {
                camView[i].aruco.detectMarkers(grabber[i].getPixels());
            }

            // all markers
            int anum = camView[i].aruco.getNumMarkers();
            if (anum < ARAP_MNUM_THR && camView[i].foundMarkerNum >= ARAP_MNUM_THR) {
                camView[i].flickerCount++;
                if (camView[i].flickerCount <= 3) {
                    anum = camView[i].foundMarkerNum; // anti flicker
                }
                else {
                    camView[i].flickerCount = 0;
                }
            }
            else {
                camView[i].flickerCount = 0;
            }
            // vaild markers
            int vnum = camView[i].aruco.getNumMarkersValidGate();
            if (vnum < ARAP_MNUM_THR && camView[i].foundValidMarkerNum >= ARAP_MNUM_THR) {
                camView[i].flickerValidCount++;
                if (camView[i].flickerValidCount <= 3) {
                    vnum = camView[i].foundValidMarkerNum; // anti flicker
                }
                else {
                    camView[i].flickerValidCount = 0;
                }
            }
            else {
                camView[i].flickerValidCount = 0;
            }

            if ((anum == 0) && (camView[i].markerDetectStrengthLast > 0) && (camView[i].rssiOutput == false)) {
                camView[i].markerEndTime = elapsedTime + 0.2f;
                camView[i].markerOutput = camView[i].markerDetectStrengthLast;
                camView[i].rssiOutput = true;
            }

            //RSSI pulse of gate passage is turned off after 0.2 seconds
            if (camView[i].markerEndTime < elapsedTime) {
                camView[i].markerOutput = 0b00;
                camView[i].markerEndTime = 0;
                camView[i].rssiOutput = false;
            }

            serialSendByte |= camView[i].markerOutput << i * 2;

            if ((vnum >= ARAP_MNUM_THR) && (vnum == anum)) camView[i].markerDetectStrengthLast = 0b11;
            else if (vnum >= ARAP_MNUM_THR) camView[i].markerDetectStrengthLast = 0b10;
            else if (anum >= ARAP_MNUM_THR) camView[i].markerDetectStrengthLast = 0b01;
            else camView[i].markerDetectStrengthLast = 0b00;

            camView[i].foundMarkerNum = anum;
            camView[i].foundValidMarkerNum = vnum;
        }
        tvpSerial.writeByte(serialSendByte);
        updateViewParams();
    }
}

//--------------------------------------------------------------
void drawInit() {
    int x = (ofGetWidth() - logoLargeImage.getWidth()) / 2;
    int y = (ofGetHeight() - logoLargeImage.getHeight()) / 2;
    int elpm = ofGetElapsedTimeMillis();
    if (elpm >= 2700) {
        ofSetColor((3000 - elpm) * 255 / 300);
    } else {
        ofSetColor(255);
    }
    logoLargeImage.draw(x, y);
}

//--------------------------------------------------------------
void drawCamCheck() {
    ofxTrueTypeFontUC *font;
    int w, h, x, xoff, y, margin;
    string str;
    bool isalt;
    // common
    w = (ofGetWidth() / 4) - 4;
    h = w / CAMERA_RATIO;
    y = (ofGetHeight() / 2) - (h / 2);
    ofSetColor(255);
    // header
    font = &myFontOvlayP2x;
    margin = font->getLineHeight();
    str = "Camera Setup";
    ofSetColor(myColorYellow);
    font->drawString(str, (ofGetWidth() - font->stringWidth(str)) / 2, y - margin);
    // camera
    ofSetColor(myColorDGray);
    ofFill();
    ofDrawRectangle(-2, y - 2, ofGetWidth() + 4, h + 4);
    if (cameraNum == 0) {
        isalt = true;
        str = "No device";
    } else {
        ofSetColor(myColorWhite);
        xoff = (ofGetWidth() - ((w + 4) * cameraNum)) / 2;
        ofNoFill();
        for (int i = 0; i < cameraNum; i++) {
            x = ((w + 4) * i) + xoff;
            if (grabber[i].isInitialized() == true) {
                grabber[i].draw(x, y, w, h);
            }
            ofDrawRectangle(x, y, w, h);
        }
        isalt = false;
    }
    if (camCheckCount >= 150 || camCheckCount < 30) {
        isalt = true;
        str = "Scanning...";
    }
    ofFill();
    // alert
    if (isalt == true) {
        drawOverlayMessageCore(&myFontLap, str);
    }
    // footer
    font = &myFontOvlayP;
    ofSetColor(myColorYellow);

    str = "If all devices are found, press Space key to continue.";
    x = (ofGetWidth() - font->stringWidth(str)) / 2;
    y = y + h + margin;
    font->drawString(str, x, y);

    str = "Press Esc key to exit.";
    x = (ofGetWidth() - font->stringWidth(str)) / 2;
    y = y + margin;
    font->drawString(str, x, y);

    drawInfo();
}

//--------------------------------------------------------------
void drawCameraImage(int camidx) {
    int i = camidx;
    int x, y, w, h;
    x = camView[i].posX;
    w = camView[i].width;
    if (camView[i].isWide == true) {
        y = camView[i].posYWide;
        h = camView[i].heightWide;
    } else {
        y = camView[i].posY;
        h = camView[i].height;
    }
    if (DEBUG_ENABLED == true && grabber[i].isInitialized() == false) {
        // dummy camera
        ofSetColor(0,19,127);
        ofFill();
        ofDrawRectangle(x, y, w, h);
        return;
    }
    if (camView[i].isWide == true) {
        // background for wide camera
        ofSetColor(0);
        ofFill();
        ofDrawRectangle(x, camView[i].posY, w, camView[i].height);
    }
    ofSetColor(myColorWhite);
    if ((camView[i].needCrop == true || camView[i].needResize == true)
        && camView[i].resizedImage.isAllocated() == true) {
        // wide
        camView[i].resizedImage.draw(x, y, w, h);
    }
    else {
        // normal
        grabber[i].draw(x, y, w, h);
    }
}

//--------------------------------------------------------------
void drawCameraARMarker(int idx, bool isSub) {
    int i = idx;
    // rect
    int tx, ty;
    float sc;
    tx = camView[i].posX;
    sc = camView[i].imageScale;
    if (camView[i].isWide == true) {
        ty = camView[i].posYWide;
    } else {
        ty = camView[i].posY;
    }
    ofPushMatrix();
    ofTranslate(tx, ty);
    ofScale(sc, sc, 1);
    ofSetLineWidth(ARAP_RECT_LINEW);
    camView[i].aruco.draw2dGate(myColorYellow, myColorAlert, false);
    ofPopMatrix();
    // meter
    string lv_valid = "";
    string lv_invalid = "";
    int x, y;
    int vnum = camView[i].foundValidMarkerNum;
    int ivnum = camView[i].foundMarkerNum - camView[i].foundValidMarkerNum;
    int offset = (cameraFrameEnabled == true) ? FRAME_LINEWIDTH : 0;
    for (int j = 0; j < vnum; j++) {
        lv_valid += "|";
    }
    for (int j = 0; j < ivnum; j++) {
        lv_invalid += "|";
    }
    x = camView[i].lapPosX + offset;
    y = isSub ? (camView[i].lapPosY + (LAP_HEIGHT / 2) + 5) : (camView[i].lapPosY + LAP_HEIGHT + 10);
    y = y + offset + 10;
    if (vnum > 0) {
        ofSetColor(myColorYellow);
        if (isSub) {
            myFontLapSub.drawString(lv_valid, x, y);
        } else {
            myFontLap.drawString(lv_valid, x, y);
        }
    }
    if (ivnum > 0) {
        ofSetColor(myColorAlert);
        if (isSub) {
            if (vnum > 0) {
                x += 2;
            }
            x = x + myFontLapSub.stringWidth(lv_valid);
            myFontLapSub.drawString(lv_invalid, x, y);
        } else {
            if (vnum > 0) {
                x += 5;
            }
            x = x + myFontLap.stringWidth(lv_valid);
            myFontLap.drawString(lv_invalid, x, y);
        }
    }
}







void drawCamera(int idx) {

    // image
    drawCameraImage(idx);
    // AR marker
    drawCameraARMarker(idx, false);
}



//--------------------------------------------------------------
void drawInfo() {
    string str;
    int x, y;
    y = ofGetHeight() - (1 + 4);
    // logo
    if (tvpScene == SCENE_CAMS || overlayMode == OVLMODE_HELP || overlayMode == OVLMODE_RCRSLT) {
        ofSetColor(myColorWhite);
        logoSmallImage.draw(0, 0);
        // appinfo
        str = ofToString(APP_VER);
        drawStringWithShadow(&myFontInfo1p, myColorWhite, myColorBGMiddle, str, 4, y);
        // date/time
        str = ofGetTimestampString("%F %T");
        x = ofGetWidth() - myFontInfo1m.stringWidth(str);
        x = (int)(x / 5) * 5;
        drawStringWithShadow(&myFontInfo1m, myColorWhite, myColorBGMiddle, str, x, y);
    }
}

//--------------------------------------------------------------
void drawStringWithShadow(ofxTrueTypeFontUC *font, ofColor color, ofColor bgcolor, string str, int x, int y) {
    // shadow
    ofRectangle rect;
    int margin = 4;
    rect = font->getStringBoundingBox(str, x, y);
    rect.width = rect.width + (rect.x - x) + (margin * 2);
    rect.height = rect.height + (margin * 2);
    rect.x = x - margin;
    rect.y = rect.y - margin;
    ofSetColor(bgcolor);
    ofDrawRectangle(rect);
    // string
    ofSetColor(color);
    font->drawString(str, x, y);
}

//--------------------------------------------------------------
void ofApp::draw() {
    if (tvpScene == SCENE_INIT) {
        drawInit();
        return;
    } else if (tvpScene == SCENE_CAMS) {
        drawCamCheck();
        return;
    }

    // camera (solo sub / solo off)
    for (int i = 0; i < cameraNum; i++) {
        drawCamera(i);
    }
    // overlay
    switch (overlayMode) {
        case OVLMODE_HELP:
            drawHelp();
            break;
        case OVLMODE_MSG:
            drawOverlayMessage();
            break;
        default:
            break;
    }
    // more info
    drawInfo();
    // SYSTEM STATUS
    if (sysStatEnabled == true) {
        int x = 10;
        int y = 50;
        int h = 15;
        //screen fps
        ofSetColor(myColorYellow);
        ofDrawBitmapString("Screen FPS: " + ofToString(ofGetFrameRate()), x, y += h);
        //AR laptimer
        ofDrawBitmapString("AR Markers/Rects/FPS:", x, y += h);
        for (int i = 0; i < cameraNum; i++) {
            int m, r;
            m = camView[i].aruco.getNumMarkers();
            r = camView[i].aruco.getNumRectangles();
            ofDrawBitmapString("  Cam" + ofToString(i + 1) + ": "
                               + ofToString(m) + "/" + ofToString(r) + "/"
                               + ofToString(camView[i].aruco.getFps()),
                               x, y += h);
        }
    }
}

//--------------------------------------------------------------
void keyPressedOverlayHelp(int key) {
    if (key == 'h' || key == 'H' || ofGetKeyPressed(OF_KEY_ESC)) {
        setOverlayMode(OVLMODE_NONE);
    } else if (key == 'N' || key == 'n'
               || key == 'F' || key == 'f'
               || key == 'T' || key == 't'
               || key == 'E' || key == 'e'
               || key == '1' || key == '2' || key == '3' || key == '4'
               || ofGetKeyPressed(TVP_KEY_ALT)
               || key == 'A' || key == 'a'
               || key == 'D' || key == 'd'
               || key == 'W' || key == 'w'
               || ofGetKeyPressed(OF_KEY_PAGE_UP)
               || ofGetKeyPressed(OF_KEY_PAGE_DOWN)
               || ofGetKeyPressed(OF_KEY_UP)
               || ofGetKeyPressed(OF_KEY_DOWN)
               || ofGetKeyPressed(OF_KEY_LEFT)
               || ofGetKeyPressed(OF_KEY_RIGHT)
               || key == 'G' || key == 'g'
               || key == 'L' || key == 'l'
               || key == 'C' || key == 'c'
               || key == 'S' || key == 's') {
        // stay at help screen
        keyPressedOverlayNone(key);
    } else {
        setOverlayMode(OVLMODE_NONE);
        keyPressedOverlayNone(key);
    }
}

//--------------------------------------------------------------
void keyPressedOverlayMessage(int key) {
    setOverlayMode(OVLMODE_NONE);
    keyPressedOverlayNone(key);
}

//--------------------------------------------------------------
void keyPressedOverlayNone(int key) {
    if (ofGetKeyPressed(OF_KEY_ESC)) {
        if (fullscreenEnabled == true) {
            toggleFullscreen();
        }
    } else {
        if (key == 'h' || key == 'H') {
            setOverlayMode(OVLMODE_HELP);
        } else if (key == 'f' || key == 'F') {
            toggleFullscreen();
        } else if (key == 's' || key == 'S') {
            toggleSysStat();
        }
    }
}

//--------------------------------------------------------------
void keyPressedCamCheck(int key) {
    if (ofGetKeyPressed(OF_KEY_ESC)) {
         ofExit();
    } else if (key == ' ') {
        if (DEBUG_ENABLED == true) {
            cameraNum = CAMERA_MAXNUM;
        }
        if (cameraNum > 0) {
            setupMain();
        }
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (tvpScene == SCENE_INIT) {
        setupCamCheck();
        return;
    } else if (tvpScene == SCENE_CAMS) {
        keyPressedCamCheck(key);
        return;
    }
    raceResultTimer = -1;
    switch (overlayMode) {
        case OVLMODE_HELP:
            keyPressedOverlayHelp(key);
            break;
        case OVLMODE_MSG:
            keyPressedOverlayMessage(key);
            break;
        case OVLMODE_NONE:
            keyPressedOverlayNone(key);
            break;
        default:
            break;
    }
}


//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
    activateCursor();
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    activateCursor();
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    activateCursor();
}



//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    // overlay
    loadOverlayFont();
    if (tvpScene != SCENE_MAIN) {
        return;
    }
    // view
    setViewParams();
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
    
}

//--------------------------------------------------------------
void ofApp::exit() {
    if (tvpScene != SCENE_MAIN) {
        return;
    }
    for (int i = 0; i < cameraNum; i++) {
        camView[i].aruco.setThreaded(false);
    }
    tvpSerial.close();
}

//--------------------------------------------------------------
void grabberUpdateResize(int cidx) {
    tvpCamView* cv = &camView[cidx];
        grabber[cidx].update();
        if (grabber[cidx].isFrameNew() == false
            || (cv->needCrop == false && cv->needResize == false)) {
            return;
        }
        cv->resizedPixels = grabber[cidx].getPixels();
        if (cv->needCrop == true) {
            cv->resizedPixels.crop(cv->cropX, cv->cropY, cv->cropW, cv->cropH);
        }
        if (cv->needResize == true) {
            if (cv->isWide == true) {
                cv->resizedPixels.resize(CAMERA_WIDTH, CAMERA_HEIGHT * 0.75);
            }
            else {
                cv->resizedPixels.resize(CAMERA_WIDTH, CAMERA_HEIGHT);
            }
        }
        cv->resizedImage.setFromPixels(cv->resizedPixels);
}
void grabberUpdateResizeMulti() {
    tvpCamView* cv;

    grabber[0].update();
    if (grabber[0].isFrameNew() == false) return;
    for (int i = 0; i < cameraNum; i++) {
        cv = &camView[i];
        cv->resizedPixels = grabber[0].getPixels();
        cv->resizedPixels.crop(cv->cropX, cv->cropY, cv->cropW, cv->cropH);
        if (cv->isWide == true) {
            cv->resizedPixels.resize(CAMERA_WIDTH, CAMERA_HEIGHT * 0.75);
        }
        else {
            cv->resizedPixels.resize(CAMERA_WIDTH, CAMERA_HEIGHT);
        }
        cv->resizedImage.setFromPixels(cv->resizedPixels);
    }
}

//--------------------------------------------------------------
void setupColors() {
    // common
    myColorYellow = ofColor(COLOR_YELLOW);
    myColorWhite = ofColor(COLOR_WHITE);
    myColorLGray = ofColor(COLOR_LGRAY);
    myColorDGray = ofColor(COLOR_DGRAY);
    myColorBGDark = ofColor(COLOR_BG_DARK);
    myColorBGMiddle = ofColor(COLOR_BG_MIDDLE);
    myColorBGLight = ofColor(COLOR_BG_LIGHT);
    myColorAlert = ofColor(COLOR_ALERT);
}

//--------------------------------------------------------------
void setViewParams() {
    int i;
    int width = ofGetWidth();
    int height = ofGetHeight();
    //float ratio = (float)width / (float)height;
    switch (cameraNumVisible) {
        case 1:
            // 1st visible camera
            camView[0].moveSteps = MOVE_STEPS;
            camView[0].heightTarget = (height / 2) - 1;
            camView[0].widthTarget = camView[0].heightTarget * CAMERA_RATIO;
            camView[0].posXTarget = (width / 2) - (camView[0].widthTarget + 1);
            camView[0].posYTarget = 0;
            break;
        case 2:
            // 1st camera
            camView[0].moveSteps = MOVE_STEPS;
            camView[0].heightTarget = (height / 2) - 1;
            camView[0].widthTarget = camView[0].heightTarget * CAMERA_RATIO;
            camView[0].posXTarget = (width / 2) - (camView[0].widthTarget + 1);
            camView[0].posYTarget = 0;
            // 2nd camera
            camView[1].moveSteps = MOVE_STEPS;
            camView[1].heightTarget = (height / 2) - 1;
            camView[1].widthTarget = camView[1].heightTarget * CAMERA_RATIO;
            camView[1].posXTarget = (width / 2) + 1;
            camView[1].posYTarget = 0;
            break;
        case 3:
            // 1st camera
            camView[0].moveSteps = MOVE_STEPS;
            camView[0].heightTarget = (height / 2) - 1;
            camView[0].widthTarget = camView[0].heightTarget * CAMERA_RATIO;
            camView[0].posXTarget = (width / 2) - (camView[0].widthTarget + 1);
            camView[0].posYTarget = 0;
            // 2nd camera
            camView[1].moveSteps = MOVE_STEPS;
            camView[1].heightTarget = (height / 2) - 1;
            camView[1].widthTarget = camView[1].heightTarget * CAMERA_RATIO;
            camView[1].posXTarget = (width / 2) + 1;
            camView[1].posYTarget = 0;
            // 3rd camera
            camView[2].moveSteps = MOVE_STEPS;
            camView[2].heightTarget = (height / 2) - 1;
            camView[2].widthTarget = camView[2].heightTarget * CAMERA_RATIO;
            camView[2].posXTarget = (width / 2) - (camView[2].widthTarget + 1);
            camView[2].posYTarget = height - camView[2].heightTarget;
            break;
        case 4:
                // 1st camera
                camView[0].moveSteps = MOVE_STEPS;
                camView[0].heightTarget = (height / 2) - 1;
                camView[0].widthTarget = camView[0].heightTarget * CAMERA_RATIO;
                camView[0].posXTarget = (width / 2) - (camView[0].widthTarget + 1);
                camView[0].posYTarget = 0;
                // 2nd camera
                camView[1].moveSteps = MOVE_STEPS;
                camView[1].heightTarget = (height / 2) - 1;
                camView[1].widthTarget = camView[1].heightTarget * CAMERA_RATIO;
                camView[1].posXTarget = (width / 2) + 1;
                camView[1].posYTarget = 0;
                // 3rd camera
                camView[2].moveSteps = MOVE_STEPS;
                camView[2].heightTarget = (height / 2) - 1;
                camView[2].widthTarget = camView[2].heightTarget * CAMERA_RATIO;
                camView[2].posXTarget = (width / 2) - (camView[2].widthTarget + 1);
                camView[2].posYTarget = height - camView[2].heightTarget;
                // 4th camera
                camView[3].moveSteps = MOVE_STEPS;
                camView[3].heightTarget = (height / 2) - 1;
                camView[3].widthTarget = camView[3].heightTarget * CAMERA_RATIO;
                camView[3].posXTarget = (width / 2) + 1;
                camView[3].posYTarget = height - camView[3].heightTarget;
            break;
        default:
            // none
            break;
    }
    for (i = 0; i < cameraNumVisible; i++) {
        //idx = getCameraIdxNthVisibleAll(i);
        //if (idx == -1) {
        //    break;
        //}
        camView[i].lapPosXTarget = max(0, camView[i].posXTarget) + LAP_MARGIN_X;
        camView[i].lapPosYTarget = max(0, camView[i].posYTarget) + LAP_MARGIN_Y;

        camView[i].imageScale = (float)(camView[i].width) / (float)CAMERA_WIDTH;
        if (camView[i].isWide == true) {
            camView[i].posYWideTarget = camView[i].posYTarget + (camView[i].heightTarget / 8);
            camView[i].heightWideTarget = camView[i].heightTarget * 0.75;
        }
    }
}

//--------------------------------------------------------------
int calcViewParam(int target, int current, int steps) {
    int val, diff;
    if (steps == 0 || target == current) {
        return target;
    }
    if (target > current) {
        diff = (target - current) / steps;
        val = current + diff;
    } else {
        diff = (current - target) / steps;
        val = current - diff;
    }
    return val;
}

//--------------------------------------------------------------
void updateViewParams() {
    int i, steps;
    for (i = 0; i < cameraNumVisible; i++) {
        // normal view
        steps = camView[i].moveSteps;
        if (steps == 0) {
            continue;
        }
        // camera
        camView[i].width = calcViewParam(camView[i].widthTarget, camView[i].width, steps);
        camView[i].height = calcViewParam(camView[i].heightTarget, camView[i].height, steps);
        camView[i].posX = calcViewParam(camView[i].posXTarget, camView[i].posX, steps);
        camView[i].posY = calcViewParam(camView[i].posYTarget, camView[i].posY, steps);
        camView[i].imageScale = (float)(camView[i].width) / (float)CAMERA_WIDTH;
        if (camView[i].isWide == true) {
            camView[i].posYWide = camView[i].posY + (camView[i].height / 8);
            camView[i].heightWide = camView[i].height * 0.75;
        }
        // lap
        camView[i].lapPosX = calcViewParam(camView[i].lapPosXTarget, camView[i].lapPosX, steps);
        camView[i].lapPosY = calcViewParam(camView[i].lapPosYTarget, camView[i].lapPosY, steps);
        camView[i].moveSteps--;
    }
}

//--------------------------------------------------------------
void initConfig() {
    // system
    sysStatEnabled = DFLT_SYS_STAT;
    cameraNumVisible = cameraNum;
    // view mode
    cameraTrimEnabled = DFLT_CAM_TRIM;
    fullscreenEnabled = DFLT_FSCR_ENBLD;
    cameraFrameEnabled = DFLT_CAM_FRAMED;
    setViewParams();
    // AR lap timer
    setOverlayMode(OVLMODE_NONE);
    raceStarted = false;
    initRaceVars();
    // finish
    xmlPilots.clear();
    saveSettingsFile();
    setOverlayMessage("Initialized settings");
}
//--------------------------------------------------------------
void toggleSysStat() {
    sysStatEnabled = !sysStatEnabled;
    saveSettingsFile();
}
//--------------------------------------------------------------
void initRaceVars() {
    for (int i = 0; i < cameraNum; i++) {
        camView[i].foundMarkerNum = 0;
        camView[i].foundValidMarkerNum = 0;
        camView[i].enoughMarkers = false;
        camView[i].flickerCount = 0;
        camView[i].flickerValidCount = 0;
    }
    elapsedTime = 0;
}
//--------------------------------------------------------------
void startRace() {
    if (raceStarted == true) {
        return;
    }
    initRaceVars();
    raceStarted = true;
    ofResetElapsedTimeCounter();
}


//--------------------------------------------------------------

//--------------------------------------------------------------
// 
//--------------------------------------------------------------
void toggleFullscreen() {
    fullscreenEnabled = !fullscreenEnabled;
    ofSetFullscreen(fullscreenEnabled);
    saveSettingsFile();
}

//--------------------------------------------------------------
void setOverlayMode(int mode) {
    overlayMode = mode;
    if (mode != OVLMODE_MSG) {
        initOverlayMessage();
    }
}

//--------------------------------------------------------------
void loadOverlayFont() {
    int h = (ofGetHeight() - (OVLTXT_MARG * 2)) / OVLTXT_LINES * 0.7;
    if (myFontOvlayP.isLoaded()) {
        myFontOvlayP.unloadFont();
    }
    if (myFontOvlayP2x.isLoaded()) {
        myFontOvlayP2x.unloadFont();
    }
    if (myFontOvlayM.isLoaded()) {
        myFontOvlayM.unloadFont();
    }
    myFontOvlayP.load(FONT_P_FILE, h);
    myFontOvlayP2x.load(FONT_P_FILE, h * 2);
    myFontOvlayM.load(FONT_M_FILE, h);
}

//--------------------------------------------------------------
void drawStringBlock(ofxTrueTypeFontUC *font, string text,
                     int xblock, int yline, int align, int blocks, int lines) {
    int bw, bh, x, y, xo, yo;
    int margin = OVLTXT_MARG;
    bw = (ofGetWidth() - (margin * 2)) / blocks;
    xo = (ofGetWidth() - (margin * 2)) % blocks / 2;
    bh = (ofGetHeight() - (margin * 2)) / lines;
    yo = (ofGetHeight() - (margin * 2)) % lines / 2;
    // pos-x
    switch (align) {
        case ALIGN_LEFT:
            x = bw * xblock;
            break;
        case ALIGN_CENTER:
            x = (bw * xblock) + (bw / 2) - (font->stringWidth(text) / 2);
            break;
        case ALIGN_RIGHT:
            x = (bw * xblock) + bw - font->stringWidth(text);
            break;
        default:
            return;
    }
    x += (margin + xo);
    // pos-y
    y = margin + ((yline + 1) * bh) + yo;
    // draw
    font->drawString(text, x, y);
}

//--------------------------------------------------------------
void drawLineBlock(int xblock1, int xblock2, int yline, int blocks, int lines) {
    int bw, bh, x, y, w, h, xo, yo;
    int margin = OVLTXT_MARG;

    bw = (ofGetWidth() - (margin * 2)) / blocks;
    xo = (ofGetWidth() - (margin * 2)) % blocks / 2;
    x = (bw * xblock1) + margin + xo;
    w = bw * (xblock2 - xblock1 + 1);

    bh = (ofGetHeight() - (margin * 2)) / lines;
    yo = (ofGetHeight() - (margin * 2)) % lines / 2;
    y = (bh * yline) + (bh * 0.5) + margin - 1 + yo;
    h = 2;

    ofFill();
    ofDrawRectangle(x, y, w, h);
}

//--------------------------------------------------------------
void drawULineBlock(int xblock1, int xblock2, int yline, int blocks, int lines) {
    int bw, bh, x, y, w, h, xo, yo;
    int margin = OVLTXT_MARG;

    bw = (ofGetWidth() - (margin * 2)) / blocks;
    xo = (ofGetWidth() - (margin * 2)) % blocks / 2;
    x = (bw * xblock1) + margin + xo;
    w = bw * (xblock2 - xblock1 + 1);

    bh = (ofGetHeight() - (margin * 2)) / lines;
    yo = (ofGetHeight() - (margin * 2)) % lines / 2;
    y = (bh * yline) + margin - 1 + yo;
    h = 2;

    ofFill();
    ofDrawRectangle(x, y, w, h);
}

//--------------------------------------------------------------
void drawHelp() {
    int szl = HELP_LINES;
    int line;
    // background
    ofSetColor(myColorBGDark);
    ofFill();
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    // title(3 lines)
    line = 1;
    ofSetColor(myColorYellow);
    drawStringBlock(&myFontOvlayP2x, "Settings / Commands", 0, line, ALIGN_CENTER, 1, szl);
    line += 2;
    // body
    drawHelpBody(line);
    // message(2 lines)
    line = HELP_LINES - 1;
    ofSetColor(myColorYellow);
    drawStringBlock(&myFontOvlayP, "Press H or Esc key to exit", 0, line, ALIGN_CENTER, 1, szl);
}

//--------------------------------------------------------------
void drawHelpBody(int line) {
    string value;
    int szl = HELP_LINES;
    int szb = 21;
    int blk0 = 6;
    int blk1 = 3;
    int blk2 = 12;
    int blk3 = 16;
    int blk4 = 17;

    // SYSTEM
    ofSetColor(myColorWhite);
    drawStringBlock(&myFontOvlayP, "System Command", blk0, line, ALIGN_CENTER, szb, szl);
    drawStringBlock(&myFontOvlayP, "Setting", blk2, line, ALIGN_CENTER, szb, szl);
    drawStringBlock(&myFontOvlayP, "Key", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    ofSetColor(myColorYellow);
    drawLineBlock(blk1, blk4, line, szb, szl);
    line++;
    ofSetColor(myColorWhite);
    // Set system statistics
    ofSetColor(myColorDGray);
    drawULineBlock(blk1, blk4, line + 1, szb, szl);
    ofSetColor(myColorWhite);
    value = sysStatEnabled ? "On" : "Off";
    drawStringBlock(&myFontOvlayP, "Set System Statistics", blk1, line, ALIGN_LEFT, szb, szl);
    drawStringBlock(&myFontOvlayP, value, blk2, line, ALIGN_CENTER, szb, szl);
    drawStringBlock(&myFontOvlayP, "S", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    // Display help
    ofSetColor(myColorDGray);
    drawULineBlock(blk1, blk4, line + 1, szb, szl);
    ofSetColor(myColorWhite);
    drawStringBlock(&myFontOvlayP, "Display Help (Settings/Commands)", blk1, line, ALIGN_LEFT, szb, szl);
    drawStringBlock(&myFontOvlayP, "-", blk2, line, ALIGN_CENTER, szb, szl);
    drawStringBlock(&myFontOvlayP, "H", blk3, line, ALIGN_CENTER, szb, szl);
    line++;

    // VIEW
    line++;
    line++;
    drawStringBlock(&myFontOvlayP, "View Command", blk0, line, ALIGN_CENTER, szb, szl);
    drawStringBlock(&myFontOvlayP, "Setting", blk2, line, ALIGN_CENTER, szb, szl);
    drawStringBlock(&myFontOvlayP, "Key", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    ofSetColor(myColorYellow);
    drawLineBlock(blk1, blk4, line, szb, szl);
    line++;
    ofSetColor(myColorWhite);
    // Set fullscreen mode
    ofSetColor(myColorDGray);
    drawULineBlock(blk1, blk4, line + 1, szb, szl);
    ofSetColor(myColorWhite);
    value = fullscreenEnabled ? "On" : "Off";
    drawStringBlock(&myFontOvlayP, "Set Fullscreen Mode", blk1, line, ALIGN_LEFT, szb, szl);
    drawStringBlock(&myFontOvlayP, value, blk2, line, ALIGN_CENTER, szb, szl);
    drawStringBlock(&myFontOvlayP, "F, Esc", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    line++;
    // Set camera view trimming
    //ofSetColor(myColorDGray);
    //drawULineBlock(blk1, blk4, line + 1, szb, szl);
    //ofSetColor(myColorWhite);
    //value = cameraTrimEnabled ? "On" : "Off";
    //drawStringBlock(&myFontOvlayP, "Set Camera View Trimming", blk1, line, ALIGN_LEFT, szb, szl);
    //drawStringBlock(&myFontOvlayP, value, blk2, line, ALIGN_CENTER, szb, szl);
    //drawStringBlock(&myFontOvlayP, "T", blk3, line, ALIGN_CENTER, szb, szl);
    line++;




    // RACE

    ofSetColor(myColorWhite);



}

//--------------------------------------------------------------
void initOverlayMessage() {
    ovlayMsgTimer = 0;
    ovlayMsgString = "";
}

//--------------------------------------------------------------
void setOverlayMessage(string msg) {
    if (overlayMode != OVLMODE_NONE && overlayMode != OVLMODE_MSG) {
        return;
    }
    ovlayMsgTimer = OLVMSG_TIME;
    ovlayMsgString = msg;
    setOverlayMode(OVLMODE_MSG);
}

//--------------------------------------------------------------
void drawOverlayMessageCore(ofxTrueTypeFontUC *font, string msg) {
    float sw, fh, sx, sy, margin;
    margin = 10;
    sw = font->stringWidth(msg);
    fh = font->getFontSize();
    sx = (ofGetWidth() / 2) - (sw / 2);
    sy = (ofGetHeight() / 2) + (fh / 2);
    // background
    ofSetColor(myColorBGDark);
    ofFill();
    ofDrawRectangle(sx - margin, sy - (fh + margin), sw + (margin * 2), fh + (margin * 2));
    // message
    ofSetColor(myColorWhite);
    font->drawString(msg, sx, sy);
}

//--------------------------------------------------------------
void drawOverlayMessage() {
    ofxTrueTypeFontUC *font = &myFontLap;
    string msg = ovlayMsgString;
    drawOverlayMessageCore(font, msg);
}



//--------------------------------------------------------------
void activateCursor() {
    if (hideCursorTimer <= 0) {
        ofShowCursor();
    }
    hideCursorTimer = HIDECUR_TIME;
}
