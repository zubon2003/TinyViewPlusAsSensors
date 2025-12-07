// charcter encoding is UTF-8

#include "ofApp.h"
#include <chrono> // Added for time measurement
#ifdef TARGET_WIN32
#include <sapi.h>
#include <atlcomcli.h>
#endif /* TARGET_WIN32 */





//--------------------------------------------------------------
void ofApp::setupInit() {
    // system
    ofSetEscapeQuitsApp(false);
    ofDirectory dir;
    if (dir.doesDirectoryExist("../data") == false) {
        // macOS binary release
        ofSetDataPathRoot("../Resources/data");
    }
    this->resourcesAllocated = false;
    this->sysStatEnabled = DFLT_SYS_STAT;
    // scene
    this->tvpScene = SCENE_INIT;
    ofResetElapsedTimeCounter();
    // screen
    ofSetWindowTitle("Tiny View Plus As Rssi Sensors");
    ofBackground(0, 0, 0);
    ofSetVerticalSync(VERTICAL_SYNC);
    ofSetFrameRate(FRAME_RATE);
    this->myFontNumber.load(FONT_P_FILE, NUMBER_HEIGHT);
    this->myFontLabel.load(FONT_P_FILE, LABEL_HEIGHT);
    this->myFontLap.load(FONT_P_FILE, LAP_HEIGHT);
    this->myFontLapHist.load(FONT_P_FILE, LAPHIST_HEIGHT);
    this->myFontNumberSub.load(FONT_P_FILE, NUMBER_HEIGHT / 2);
    this->myFontLabelSub.load(FONT_P_FILE, LABEL_HEIGHT / 2);
    this->myFontLapSub.load(FONT_P_FILE, LAP_HEIGHT / 2);
    this->myFontInfo1m.load(FONT_M_FILE, INFO_HEIGHT);
    this->myFontInfo1p.load(FONT_P_FILE, INFO_HEIGHT);
    this->myFontInfoWatch.load(FONT_M_FILE, WATCH_HEIGHT);
    this->loadOverlayFont();
    this->cameraTrimEnabled = DFLT_CAM_TRIM;
    this->fullscreenEnabled = DFLT_FSCR_ENBLD;
    this->cameraFrameEnabled = DFLT_CAM_FRAMED;
    // splash
    this->logoLargeImage.load(LOGO_LARGE_FILE);
    // logo
    this->logoSmallImage.load(LOGO_SMALL_FILE);
    // view common
    this->setupColors();
    this->hideCursorTimer = HIDECUR_TIME;
    // overlay
    this->setOverlayMode(OVLMODE_NONE);
    this->initOverlayMessage();
    // AR lap timer
    this->raceStarted = false;
    this->elapsedTime = 0;
    this->raceResultTimer = -1;
    this->arLapMode = DFLT_ARAP_MODE;
    this->flickerLength = 100;
    // extra camera
    this->camProfFpvExtra.enabled = false;
    //
    this->gateDetectAllFrames = DTCT_ALL_FRAME;
}

//--------------------------------------------------------------
void ofApp::loadSettingsFile() {
    xmlSettings.loadFile(SETTINGS_FILE);

    // SYSTEM
    // system statistics
    this->sysStatEnabled = xmlSettings.getValue(SNM_SYS_STAT, this->sysStatEnabled);
    this->logEnabled = xmlSettings.getValue(SNM_LOG_ENABLED, false);

    // VIEW
    // fullscreen
    this->fullscreenEnabled = xmlSettings.getValue(SNM_VIEW_FLLSCR, this->fullscreenEnabled);
    // camera view trimming
    this->cameraTrimEnabled = xmlSettings.getValue(SNM_VIEW_CAMTRM, this->cameraTrimEnabled);
    // camera frame visibility
    this->cameraFrameEnabled = xmlSettings.getValue(SNM_VIEW_CAMFRM, this->cameraFrameEnabled);

    // OSC
    this->oscSenderHost = xmlSettings.getValue(SNM_OSC_SENDER_HOST, OSC_SENDER_DEFAULT_HOST);
    this->oscSenderPort = xmlSettings.getValue(SNM_OSC_SENDER_PORT, OSC_SENDER_DEFAULT_PORT);
    this->oscReceivePort = xmlSettings.getValue(SNM_OSC_RECEIVER_PORT, OSC_RECEIVER_DEFAULT_PORT);
    // ArUco
    this->arucoMinSize = xmlSettings.getValue(SNM_ARUCO_MIN_SIZE, 5);
    this->arLapMode = xmlSettings.getValue("aruco:arMode", this->arLapMode);
    this->flickerLength = xmlSettings.getValue(SNM_FLICKER_LENGTH, this->flickerLength);

    //GATE DETECT FREQUENCY
    this->gateDetectAllFrames = xmlSettings.getValue(SNM_DTCTALL_FRM, this->gateDetectAllFrames);
}

void ofApp::saveSettingsFile() {
    // SYSTEM
    // system statistics
    xmlSettings.setValue(SNM_SYS_STAT, this->sysStatEnabled);
    xmlSettings.setValue(SNM_LOG_ENABLED, this->logEnabled);

    // VIEW
    // fullscreen
    xmlSettings.setValue(SNM_VIEW_FLLSCR, this->fullscreenEnabled);
    // camera view trimming
    xmlSettings.setValue(SNM_VIEW_CAMTRM, this->cameraTrimEnabled);
    // camera frame visibility
    xmlSettings.setValue(SNM_VIEW_CAMFRM, this->cameraFrameEnabled);

    // OSC
    xmlSettings.setValue(SNM_OSC_SENDER_HOST, this->oscSenderHost);
    xmlSettings.setValue(SNM_OSC_SENDER_PORT, this->oscSenderPort);
    xmlSettings.setValue(SNM_OSC_RECEIVER_PORT, this->oscReceivePort);

    // ArUco
    xmlSettings.setValue(SNM_ARUCO_MIN_SIZE, this->arucoMinSize);
    xmlSettings.setValue("aruco:arMode", this->arLapMode);
    xmlSettings.setValue(SNM_FLICKER_LENGTH, this->flickerLength);

    xmlSettings.setValue(SNM_DTCTALL_FRM, this->gateDetectAllFrames);

    xmlSettings.saveFile(SETTINGS_FILE);
}

//--------------------------------------------------------------
// New functions for frequency management and OSC communication
//--------------------------------------------------------------

void ofApp::loadFrequencies() {
    string filename = "frequencies.xml";
    if (frequencyXml.loadFile(filename)) {
        if (this->logEnabled) ofLogNotice("ofApp::loadFrequencies") << filename << " loaded.";
        
        cameraFrequencyMap.clear();
        
        frequencyXml.pushTag("frequencies");
        for (int i = 0; i < CAMERA_MAXNUM; ++i) {
            string tagName = "camera" + ofToString(i);
            if (frequencyXml.tagExists(tagName)) {
                int channel = frequencyXml.getValue(tagName, 0);
                if (channel != 0) {
                    cameraFrequencyMap[i] = channel;
                }
            }
        }
        frequencyXml.popTag();

    } else {
        if (this->logEnabled) ofLogWarning("ofApp::loadFrequencies") << "Could not load " << filename << ". Using default values.";
        
        // Default values
        cameraFrequencyMap[0] = 0;
        cameraFrequencyMap[1] = 0;
        cameraFrequencyMap[2] = 0;
        cameraFrequencyMap[3] = 0;
        
        saveFrequencies(); // Save defaults to file
    }

    // Update active camera indices based on loaded/default frequencies
    activeCameraIndices.clear();
    for (int i = 0; i < CAMERA_MAXNUM; ++i) { // Iterate through all possible camera indices
        if (cameraFrequencyMap.count(i) && cameraFrequencyMap[i] != 0) {
            activeCameraIndices.push_back(i);
        }
    }
    
    // Log active camera indices
    string activeCamsStr = "Active camera indices: ";
    if (activeCameraIndices.empty()) {
        activeCamsStr += "None";
    } else {
        for (int idx : activeCameraIndices) {
            activeCamsStr += ofToString(idx) + " ";
        }
    }
    if (this->logEnabled) ofLogNotice("ofApp::loadFrequencies") << activeCamsStr;

    // Update cameraNumVisible based on active cameras
    cameraNumVisible = static_cast<int>(activeCameraIndices.size());
    if (this->logEnabled) ofLogNotice("ofApp::loadFrequencies") << cameraNumVisible << " cameras are active.";
    this->setViewParams(); // Recalculate view parameters for active cameras
}

void ofApp::saveFrequencies() {
    frequencyXml.clear();
    frequencyXml.addTag("frequencies");
    frequencyXml.pushTag("frequencies");

    for (auto const& pair : cameraFrequencyMap) { // Modified loop
        int camIndex = pair.first;
        int channel = pair.second;
        string tagName = "camera" + ofToString(camIndex);
        frequencyXml.setValue(tagName, channel);
    }
    
    frequencyXml.popTag();
    frequencyXml.saveFile("frequencies.xml");
    if (this->logEnabled) ofLogNotice("ofApp::saveFrequencies") << "Frequencies saved to frequencies.xml";
}

//--------------------------------------------------------------



//--------------------------------------------------------------
int ofApp::calculate_pseudo_rssi(int camIndex) {
    // Simple pseudo-RSSI based on marker detection strength
    // This can be refined based on actual marker size, distance, etc.
    int anum = camView[camIndex].aruco.getNumMarkers();
    int vnum = camView[camIndex].aruco.getNumMarkersValidGate();

    if (vnum >= this->arMarkerNumThreshold) { // Valid gate markers detected
        return 200; // Strong signal
    } else if (anum >= this->arMarkerNumThreshold) { // Any markers detected
        return 150; // Medium signal
    } else { // No markers detected
        return 50; // Weak/no signal
    }
}


//--------------------------------------------------------------
void ofApp::loadCameraProfileFile() {
    if (this->xmlCamProfFpv.loadFile(CAM_FPV_FILE) == false) {
        return;
    }
    tvpCamProf *p = &this->camProfFpvExtra;
    ofxXmlSettings *s = &this->xmlCamProfFpv;
    // load
    p->enabled = true;
    p->camnum = s->getValue(CFNM_CAMNUM, 1);
    if (p->camnum > 1) this->isMultiView = true;
    else this->isMultiView = false;
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
void ofApp::setupCamCheck() {
    tvpScene = SCENE_CAMS;
    cameraNum = 0;
    camCheckCount = 0;
    reloadCameras();
}

//--------------------------------------------------------------
void ofApp::reloadCameras() {
    // Clear
    for (int i = 0; i < this->cameraNum; i++) {
        grabber[i].close();
    }
    // Load
    ofVideoGrabber tmpgrb;
    vector<ofVideoDevice> devices = tmpgrb.listDevices();
    tvpCamProf* prof = &this->camProfFpvExtra;
    int cidx = 0;
    this->cameraNum = 0;
    if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "Scanning camera... " << devices.size() << " devices found.";
    if (this->isMultiView == false){
        for (size_t i = 0; i < devices.size(); i++) {
            int w, h, aw, ah;
            bool extra = false;
            if (prof->enabled == true && regex_search(devices[i].deviceName, regex(prof->name)) == true) {
                extra = true;
            }
            if (regex_search(devices[i].deviceName, regex("USB2.0 PC CAMERA")) == false && extra == false) {
                if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "  Skipping device " << devices[i].id << ": " << devices[i].deviceName << " (not 'USB2.0 PC CAMERA' and not extra profile)";
                continue;
            }
            if (devices[i].bAvailable == false) {
                if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "  Skipping device " << devices[i].id << ": " << devices[i].deviceName << " (not available)";
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
                if (this->logEnabled) ofLogWarning("ofApp::reloadCameras") << "  Failed to initialize grabber for device " << devices[i].id << ": " << devices[i].deviceName << " with resolution " << w << "x" << h;
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
            if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "  Initialized camera [" << devices[i].id << "] " << devices[i].deviceName;
            if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "    Preferred resolution: " << w << " x " << h;
            if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "    Actual resolution: " << aw << " x " << ah;
            if (extra == true) {
                if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "    Crop: "
                    << prof->cropX << ", " << prof->cropY << ", "
                    << prof->cropW << ", " << prof->cropH;
            }
            cidx++;
            this->cameraNum++;
            if (this->cameraNum == CAMERA_MAXNUM) {
                if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "  Reached maximum number of cameras (" << CAMERA_MAXNUM << "). Stopping scan.";
                break;
            }
        }
    }
    else {
        for (size_t i = 0; i < devices.size(); i++) {
            if (regex_search(devices[i].deviceName, regex(prof->name)) == false) {
                if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "  Skipping device " << devices[i].id << ": " << devices[i].deviceName << " (does not match multi-view profile name)";
                continue;
            }
            if (devices[i].bAvailable == false) {
                if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "  Skipping device " << devices[i].id << ": " << devices[i].deviceName << " (not available)";
                continue;
            }
            //
            this->cameraNum = prof->camnum;
            if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "  Multi-view mode: Initializing " << this->cameraNum << " cameras from device " << devices[i].id << ": " << devices[i].deviceName;

            for (int j = 0; j <= this->cameraNum-1; j++) {
                grabber[j].setDeviceID(devices[i].id);
                camView[j].grabW = prof->grabW;
                camView[j].grabH = prof->grabH;
                // Only initialize the first grabber for multi-view
                if (j == 0) {
                    if (grabber[j].initGrabber(camView[j].grabW, camView[j].grabH) == false) {
                        if (this->logEnabled) ofLogWarning("ofApp::reloadCameras") << "  Failed to initialize grabber for multi-view camera " << j << " from device " << devices[i].id << ": " << devices[i].deviceName;
                        // If the first grabber fails, we cannot proceed with multi-view from this device.
                        break; // Exit the inner loop
                    }
                    if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "    Multi-view master camera " << j << " initialized. Actual resolution: " << grabber[j].getWidth() << "x" << grabber[j].getHeight();
                } else {
                    // For subsequent virtual cameras, just set the grabW/H, no need to initGrabber again
                    if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "    Multi-view virtual camera " << j << " configured.";
                }
                camView[j].needCrop = true;
                camView[j].needResize = true;
                camView[j].isWide = prof->isWide;

                if (j % 2 == 0) camView[j].cropX = prof->cropX / 2;
                else camView[j].cropX = camView[j].grabW / 2 + prof->cropX / 2;
                
                if (j <= 1) camView[j].cropY = prof->cropY / 2;
                else camView[j].cropY = camView[j].grabH / 2 + prof->cropY / 2;
                
                camView[j].cropW = prof->cropW / 2;
                camView[j].cropH = prof->cropH / 2;
                if (this->logEnabled) ofLogNotice("ofApp::reloadCameras") << "    Multi-view camera " << j << " crop: "
                    << camView[j].cropX << ", " << camView[j].cropY << ", "
                    << camView[j].cropW << ", " << camView[j].cropH;
            }
            break;
        }
    }
    tmpgrb.close();
}

//--------------------------------------------------------------
void ofApp::setupMain() {
    // system
    ofSetFullscreen(this->fullscreenEnabled);
    tvpScene = SCENE_MAIN;
    if (this->logEnabled) ofLogNotice("ofApp::setupMain") << "Scene set to SCENE_MAIN.";
    // camera
    this->cameraNumVisible = this->cameraNum;
    setViewParams();
    // AR laptimer
    for (int i = 0; i < this->cameraNum; i++) {
        camView[i].aruco.setUseHighlyReliableMarker(ARAP_MKR_FILE);
        camView[i].aruco.setMinMaxMarkerDetectionSize(this->arucoMinSize*0.01f, 0.25);
        camView[i].aruco.setThreaded(true);
        camView[i].aruco.setup2d(CAMERA_WIDTH, CAMERA_HEIGHT);
        camView[i].flickerEndtime = 0;
        camView[i].rssiOutput = false;
        camView[i].isDroneInGate = false;
        camView[i].flickerEndtime = 0;
    }

    this->initRaceVars();
    this->startRace();
}

//--------------------------------------------------------------
void ofApp::setup() {
    string logPath = ofFilePath::getAbsolutePath(ofFilePath::getEnclosingDirectory(ofFilePath::getCurrentExePath()) + "of_log.txt");
    ofLogToFile(logPath, false); // Output logs to of_log.txt and clear existing logs
    this->setupInit();
    this->loadSettingsFile();
    this->updateArLapModeSettings();
    this->loadCameraProfileFile();
    this->saveSettingsFile();

    // Initialize OSC Sender
    oscSender.setup(oscSenderHost, oscSenderPort);
    if (this->logEnabled) ofLogNotice("ofApp::setup") << "OSC sender setup to " << oscSenderHost << ":" << oscSenderPort;

    // Initialize OSC Receiver
    oscReceiver.setup(oscReceivePort);
    if (this->logEnabled) ofLogNotice("ofApp::setup") << "OSC receiver listening on port " << oscReceivePort;

    // Load frequency settings
    loadFrequencies();

    // Example of initializing frequencies for each camera
    // Here, set the appropriate value for each camView[camIdx].frequency.
    // Example:
    // camView[0].frequency = 5658;
    // camView[1].frequency = 5695;
    // ...
    for (int i = 0; i < CAMERA_MAXNUM; ++i) { // Assuming CAMERA_MAXNUM is defined
        camView[i].frequency = 0; // Default value. Replace with actual frequency.
        camView[i].loopTime = 0.0f; // Initialization
    }
}

//--------------------------------------------------------------
void ofApp::updateInit() {
    if (ofGetElapsedTimeMillis() >= 3000) {
        this->setupCamCheck();
    }
}

//--------------------------------------------------------------
void ofApp::updateCamCheck() {
    for (int i = 0; i < this->cameraNum; i++) {
        grabber[i].update();
    }
    if (camCheckCount > 180) {
        this->reloadCameras();
        camCheckCount = 0;
        return;
    }
    camCheckCount++;
}

//--------------------------------------------------------------
void ofApp::update() {
    // scene
    if (tvpScene == SCENE_INIT) {
        this->updateInit();
        return;
    } else if (tvpScene == SCENE_CAMS) {
        this->updateCamCheck();
        return;
    }
    // timer
    if (this->raceStarted == true) this->elapsedTime = ofGetElapsedTimef();

    // camera
    if (this->isMultiView == true) {
        this->grabberUpdateResizeMulti();
    }
    else {
        for (int i = 0; i < this->cameraNum; i++) {
            this->grabberUpdateResize(i);
        }
    }

    for (int camIdx = 0; camIdx < cameraNum; camIdx++) { // cameraNum is assumed to be the number of active cameras
        // Record the start time of processing for each camera
        auto startTime = std::chrono::high_resolution_clock::now();

        // ... existing processing code for camView[camIdx] ...
        // Example: marker detection, pseudo-RSSI calculation, etc.

        // Record the end time of processing for each camera and calculate loopTime
        // auto endTime = std::chrono::high_resolution_clock::now();
        // camView[camIdx].loopTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        // Calculate loopTime inversely from AR marker detection FPS
        float arFps = camView[camIdx].aruco.getFps();
        if (arFps > 0) {
            camView[camIdx].loopTime = 1000.0f / arFps;
        } else {
            camView[camIdx].loopTime = 0.0f; // Set to 0 if FPS is 0
        }

        // ... remaining processing code for camView[camIdx] ...
    }


    // lap
    this->frameTick = !this->frameTick;
    if (this->gateDetectAllFrames == true) this->frameTick = true;

    if (this->frameTick == true) {
        // Process active cameras for AR detection and send ts_lap_data
        for (int camIdx : this->activeCameraIndices) {
            // AR lap timer
            ofPixels pixelsToDetect;
            if (camView[camIdx].needCrop == true || camView[camIdx].needResize == true) {
                pixelsToDetect = camView[camIdx].resizedImage.getPixels();
            }
            else {
                pixelsToDetect = grabber[camIdx].getPixels();
            }

            // Ensure detection is done on a grayscale image for robustness
            if (pixelsToDetect.isAllocated()) {
                if (pixelsToDetect.getNumChannels() > 1) {
                    ofImage tempImage;
                    tempImage.setFromPixels(pixelsToDetect);
                    tempImage.setImageType(OF_IMAGE_GRAYSCALE);
                    camView[camIdx].aruco.detectMarkers(tempImage.getPixels());
                } else {
                    camView[camIdx].aruco.detectMarkers(pixelsToDetect);
                }
            }

            // all markers
            int anum = camView[camIdx].aruco.getNumMarkers();
            if (anum < this->arMarkerNumThreshold && camView[camIdx].foundMarkerNum >= this->arMarkerNumThreshold) {
                if (camView[camIdx].flickerCount == 0) {
                    camView[camIdx].flickerEndtime = elapsedTime + flickerLength * 0.001f;
                }
                camView[camIdx].flickerCount++;
                if ((elapsedTime < camView[camIdx].flickerEndtime) && (camView[camIdx].flickerEndtime > 0)) {
                    anum = camView[camIdx].foundMarkerNum; // anti flicker
                }
                else {
                    camView[camIdx].flickerCount = 0;
                    camView[camIdx].flickerEndtime = 0;
                }
            }
            else {
                camView[camIdx].flickerCount = 0;
                camView[camIdx].flickerEndtime = 0;
            }
            // vaild markers
            int vnum = camView[camIdx].aruco.getNumMarkersValidGate();
            if (vnum < this->arMarkerNumThreshold && camView[camIdx].foundValidMarkerNum >= this->arMarkerNumThreshold) {
                if (camView[camIdx].flickerValidCount == 0) {
                    camView[camIdx].flickerValidEndtime = elapsedTime + flickerLength * 0.001f;
                }
                camView[camIdx].flickerValidCount++;
                if ((elapsedTime < camView[camIdx].flickerValidEndtime) && (camView[camIdx].flickerValidEndtime > 0)){
                    vnum = camView[camIdx].foundValidMarkerNum; // anti flicker
                }
                else {
                    camView[camIdx].flickerValidCount = 0;
                    camView[camIdx].flickerEndtime = 0;
                }
            }
            else {
                camView[camIdx].flickerValidCount = 0;
                camView[camIdx].flickerEndtime = 0;
            }

            // Determine if the drone is in the gate
            bool currentlyInGate;
            if (this->arLapMode == ARAP_MODE_NORM) currentlyInGate = ((vnum >= this->arMarkerNumThreshold) && (vnum== anum));
            else if (this->arLapMode == ARAP_MODE_MIDDLE) currentlyInGate = (vnum >= this->arMarkerNumThreshold);
            else currentlyInGate = (anum >= this->arMarkerNumThreshold);

            if (currentlyInGate) {
                if (this->logEnabled) ofLogNotice("ofApp::update") << "Drone is currently in gate for camera " << camIdx << ". anum: " << anum << ", vnum: " << vnum;
                camView[camIdx].isDroneInGate = true;
                // Store the ID of the first detected marker if not already stored
                const auto& detectedMarkers = camView[camIdx].aruco.getMarkers();
                if (this->logEnabled) ofLogNotice("ofApp::update") << "Detected markers size: " << detectedMarkers.size();
                if (!detectedMarkers.empty() && camView[camIdx].lastValidMarkerId == -1) { // Only update if markers are actually detected and it's not set yet
                    camView[camIdx].lastValidMarkerId = detectedMarkers[0].id;
                    if (this->logEnabled) ofLogNotice("ofApp::update") << "  Stored first Marker ID: " << detectedMarkers[0].id;
                }
            } else {
                // If previously in gate and now out, detect lap
                if (camView[camIdx].isDroneInGate) {
                    // LAP DETECTED!
                    ofxOscMessage m;
                    m.setAddress("/ts_lap_data");
                    m.addFloatArg(ofGetElapsedTimef() - flickerLength * 0.001f - this->raceStartTime); // lap_time (float)
                    m.addIntArg(this->cameraFrequencyMap[camIdx]); // frequency (int)
                    m.addIntArg(this->calculate_pseudo_rssi(camIdx)); // peak_rssi (int)

                    // Add marker ID as a string to the OSC message
                    std::string markerIdStr = ofToString(camView[camIdx].lastValidMarkerId); // Use single ID
                    m.addStringArg(markerIdStr); // marker_ids (string)

                    this->oscSender.sendMessage(m, false); // Send immediately
                    if (this->logEnabled) ofLogNotice("ofApp::update") << "Sent /ts_lap_data for camera " << camIdx << " (Freq: " << this->cameraFrequencyMap[camIdx] << ") with Marker ID: " << markerIdStr;

                    camView[camIdx].rssiOutput = true; // Indicate that a lap was detected in this frame for heartbeat
                    camView[camIdx].lastValidMarkerId = -1; // Reset for the next lap
                }
                camView[camIdx].isDroneInGate = false; // Drone is out of gate
            }

            

            camView[camIdx].foundMarkerNum = anum;
            camView[camIdx].foundValidMarkerNum = vnum;
        }
        // tvpSerial.writeByte(serialSendByte); // REMOVED
        this->updateViewParams();
    }

    // Process incoming OSC messages
    while (oscReceiver.hasWaitingMessages()) {
        ofxOscMessage m;
        oscReceiver.getNextMessage(m);

        // Handling /get_server_info
        if (m.getAddress() == "/get_server_info") {
            if (this->logEnabled) ofLogNotice("ofApp::update") << "Received /get_server_info request.";
            ofxOscMessage response;
            response.setAddress("/server_info");
            response.addStringArg("1.0.0"); // release_version
            response.addStringArg("TinyViewPlusAsSensors"); // name
            oscSender.sendMessage(response, false);
            if (this->logEnabled) ofLogNotice("ofApp::update") << "Sent /server_info response.";
        }
        // Handling /set_frequencies
        else if (m.getAddress() == "/set_frequencies") {
            if (this->logEnabled) ofLogNotice("ofApp::update") << "Received /set_frequencies request.";
            // Implement frequency setting logic here (dummy for now)
            ofxOscMessage response;
            response.setAddress("/frequencies_set_ack");
            response.addIntArg(1); // Indicate success (1: success, 0: failure)
            oscSender.sendMessage(response, false);
            if (this->logEnabled) ofLogNotice("ofApp::update") << "Sent /frequencies_set_ack response.";
        }
        else if (m.getAddress() == "/stage_ready") {
            onStageReady(m);
        }
        else {
            if (this->logEnabled) ofLogNotice("ofApp::update") << "Received unknown OSC message: " << m.getAddress();
        }
    }

    // Send heartbeat OSC message periodically
    float currentTime = ofGetElapsedTimef();
    if (currentTime - this->lastHeartbeatTime > 0.5f) { // Send every 0.5 seconds
        ofxOscMessage m;
        m.setAddress("/heartbeat");

        // Prepare data for heartbeat
        ofJson current_rssi_json = ofJson::array();
        ofJson frequency_json = ofJson::array();
        ofJson crossing_flag_json = ofJson::array();
        ofJson loop_time_json = ofJson::array();

        for (int camIdx = 0; camIdx < cameraNum; camIdx++) { // Iterate over all cameras
            current_rssi_json.push_back(this->calculate_pseudo_rssi(camIdx));
            // Ensure cameraFrequencyMap has the key, otherwise use a default value
            frequency_json.push_back(this->cameraFrequencyMap.count(camIdx) ? this->cameraFrequencyMap[camIdx] : 0);
            crossing_flag_json.push_back(camView[camIdx].rssiOutput);
            loop_time_json.push_back(camView[camIdx].loopTime);
        }

        // Add data as stringified JSON to OSC message
        // Combine all data into a single JSON object
        ofJson heartbeat_data;
        heartbeat_data["current_rssi"] = current_rssi_json;
        heartbeat_data["frequency"] = frequency_json;
        heartbeat_data["crossing_flag"] = crossing_flag_json;
        heartbeat_data["loop_time"] = loop_time_json;
        heartbeat_data["flicker_length"] = this->flickerLength;

        // Send JSON string as binary data by converting to ofBuffer
        std::string json_str = heartbeat_data.dump();
        m.addStringArg(json_str); // Send JSON string as string type

        this->oscSender.sendMessage(m, false);
        if (this->logEnabled) ofLogVerbose("ofApp::update") << "Sent /heartbeat";
        this->lastHeartbeatTime = currentTime;
    }
}

//--------------------------------------------------------------
void ofApp::drawInit() {
    int x = (ofGetWidth() - this->logoLargeImage.getWidth()) / 2;
    int y = (ofGetHeight() - this->logoLargeImage.getHeight()) / 2;
    int elpm = ofGetElapsedTimeMillis();
    if (elpm >= 2700) {
        ofSetColor((3000 - elpm) * 255 / 300);
    } else {
        ofSetColor(255);
    }
    this->logoLargeImage.draw(x, y);
}

//--------------------------------------------------------------
void ofApp::drawCamCheck() {
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
    font = &this->myFontOvlayP2x;
    margin = font->getLineHeight();
    str = "Camera Setup";
    ofSetColor(this->myColorYellow);
    font->drawString(str, (ofGetWidth() - font->stringWidth(str)) / 2, y - margin);
    // camera
    ofSetColor(this->myColorDGray);
    ofFill();
    ofDrawRectangle(-2, y - 2, ofGetWidth() + 4, h + 4);
    if (this->cameraNum == 0) {
        isalt = true;
        str = "No device";
    } else {
        ofSetColor(this->myColorWhite);
        xoff = (ofGetWidth() - ((w + 4) * this->cameraNum)) / 2;
        ofNoFill();
        for (int i = 0; i < this->cameraNum; i++) {
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
        this->drawOverlayMessageCore(&this->myFontLap, str);
    }
    // footer
    font = &this->myFontOvlayP;
    ofSetColor(this->myColorYellow);

    str = "If all devices are found, press Space key to continue.";
    x = (ofGetWidth() - font->stringWidth(str)) / 2;
    y = y + h + margin;
    font->drawString(str, x, y);

    str = "Press Esc key to exit.";
    x = (ofGetWidth() - font->stringWidth(str)) / 2;
    y = y + margin;
    font->drawString(str, x, y);

    this->drawInfo();
}

//--------------------------------------------------------------
void ofApp::drawCameraImage(int camidx) {
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
    ofSetColor(this->myColorWhite);
    if ((camView[i].needCrop == true || camView[i].needResize == true)
        && camView[i].resizedImage.getWidth() > 0) {
        // if (this->logEnabled) ofLogNotice("ofApp::drawCameraImage") << "Drawing cam " << i << " resizedImage at " << x << "," << y << " with size " << w << "," << h;
        camView[i].resizedImage.draw(x, y, w, h);
    }
    else {
        if (camView[i].needCrop == true || camView[i].needResize == true) {
            // if (this->logEnabled) ofLogWarning("ofApp::drawCameraImage") << "camView[" << i << "] resizedImage not allocated. Crop/Resize is true.";
        } else {
            if (this->logEnabled) ofLogNotice("ofApp::drawCameraImage") << "Drawing cam " << i << " grabber at " << x << "," << y << " with size " << w << "," << h;
        }
        grabber[i].draw(x, y, w, h);
    }
}

//--------------------------------------------------------------
void ofApp::drawCameraARMarker(int idx, bool isSub) {
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
    camView[i].aruco.draw2dGate(this->myColorYellow, this->myColorAlert, false);
    ofPopMatrix();
    // meter
    string lv_valid = "";
    string lv_invalid = "";
    int x, y;
    int vnum = camView[i].foundValidMarkerNum;
    int ivnum = camView[i].foundMarkerNum - camView[i].foundValidMarkerNum;
    int offset = (this->cameraFrameEnabled == true) ? FRAME_LINEWIDTH : 0;
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
        ofSetColor(this->myColorYellow);
        if (isSub) {
            this->myFontLapSub.drawString(lv_valid, x, y);
        } else {
            this->myFontLap.drawString(lv_valid, x, y);
        }
    }
    if (ivnum > 0) {
        ofSetColor(this->myColorAlert);
        if (isSub) {
            if (vnum > 0) {
                x += 2;
            }
            x = x + this->myFontLapSub.stringWidth(lv_valid);
            this->myFontLapSub.drawString(lv_invalid, x, y);
        } else {
            if (vnum > 0) {
                x += 5;
            }
            x = x + this->myFontLap.stringWidth(lv_valid);
            this->myFontLap.drawString(lv_invalid, x, y);
        }
    }
}







void ofApp::drawCamera(int idx) {

    // image
    this->drawCameraImage(idx);
    // AR marker
    this->drawCameraARMarker(idx, false);
}



//--------------------------------------------------------------
void ofApp::drawInfo() {
    string str;
    int x, y;
    y = ofGetHeight() - (1 + 4);
    // logo
    if (tvpScene == SCENE_CAMS || this->overlayMode == OVLMODE_HELP || this->overlayMode == OVLMODE_RCRSLT) {
        ofSetColor(this->myColorWhite);
        this->logoSmallImage.draw(0, 0);
        // appinfo
        str = ofToString(APP_VER);
        this->drawStringWithShadow(&this->myFontInfo1p, this->myColorWhite, this->myColorBGMiddle, str, 4, y);
        // date/time
        str = ofGetTimestampString("%F %T");
        x = ofGetWidth() - this->myFontInfo1m.stringWidth(str);
        x = (int)(x / 5) * 5;
        this->drawStringWithShadow(&this->myFontInfo1m, this->myColorWhite, this->myColorBGMiddle, str, x, y);
    }
}

//--------------------------------------------------------------
void ofApp::drawStringWithShadow(ofxTrueTypeFontUC *font, ofColor color, ofColor bgcolor, string str, int x, int y) {
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
        this->drawInit();
        return;
    } else if (tvpScene == SCENE_CAMS) {
        this->drawCamCheck();
        return;
    }

    // camera (solo sub / solo off)
    for (int camIdx : this->activeCameraIndices) {
        this->drawCamera(camIdx);
    }
    // overlay
    switch (this->overlayMode) {
        case OVLMODE_HELP:
            this->drawHelp();
            break;
        case OVLMODE_MSG:
            this->drawOverlayMessage();
            break;
        default:
            break;
    }
    // more info
    this->drawInfo();
    // SYSTEM STATUS
    if (this->sysStatEnabled) {
        int x = 10;
        int y = 50;
        int h = 15;
        //screen fps
        ofSetColor(this->myColorYellow);
        ofDrawBitmapString("Screen FPS: " + ofToString(ofGetFrameRate()), x, y += h);
        //AR laptimer mode
        string arModeStr;
        if (this->arLapMode == ARAP_MODE_NORM) {
            arModeStr = "Normal";
        } else if (this->arLapMode == ARAP_MODE_MIDDLE) {
            arModeStr = "Middle";
        } else if (this->arLapMode == ARAP_MODE_LOOSE) {
            arModeStr = "Loose";
        } else if (this->arLapMode == ARAP_MODE_ULOOSE) {
            arModeStr = "UltraLoose";
        } else {
            arModeStr = "Off";
        }
        ofDrawBitmapString("AR Mode: " + arModeStr, x, y += h);
        //AR laptimer
        ofDrawBitmapString("AR Markers/Rects/FPS:", x, y += h);
        for (int i = 0; i < this->cameraNum; i++) {
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
void ofApp::keyPressedOverlayHelp(int key) {
    if (key == 'h' || key == 'H' || ofGetKeyPressed(OF_KEY_ESC)) {
        this->setOverlayMode(OVLMODE_NONE);
    } else if (ofGetKeyPressed(OF_KEY_LEFT)) {
        this->changeFlickerLength(TVP_VAL_MINUS);
    } else if (ofGetKeyPressed(OF_KEY_RIGHT)) {
        this->changeFlickerLength(TVP_VAL_PLUS);
    } else if (ofGetKeyPressed(OF_KEY_UP)) {
        this->changeArucoMinSize(TVP_VAL_PLUS);
    } else if (ofGetKeyPressed(OF_KEY_DOWN)) {
        this->changeArucoMinSize(TVP_VAL_MINUS);
    } else if (key == 's' || key == 'S') {
        this->toggleSysStat();
    } else if (key == 'l' || key == 'L') {
        this->toggleLog();
    } else if (key == 'a' || key == 'A') {
        this->toggleARLap();
    } else if (key == 'd' || key == 'D') {
        this->toggleGateDetectFrequency();
    } else {
        this->setOverlayMode(OVLMODE_NONE);
        this->keyPressedOverlayNone(key);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressedOverlayMessage(int key) {
    this->setOverlayMode(OVLMODE_NONE);
    this->keyPressedOverlayNone(key);
}

//--------------------------------------------------------------
void ofApp::keyPressedOverlayNone(int key) {
    if (ofGetKeyPressed(OF_KEY_ESC)) {
        if (this->fullscreenEnabled == true) {
            this->toggleFullscreen();
        }
    } else {
        if (key == 'h' || key == 'H') {
            this->setOverlayMode(OVLMODE_HELP);
        } else if (key == 'f' || key == 'F') {
            this->toggleFullscreen();
        } else if (key == 's' || key == 'S') {
            this->toggleSysStat();
        } else if (key == 'l' || key == 'L') {
            this->toggleLog();
        } else if (key == 'a' || key == 'A') {
            this->toggleARLap();
        } else if (key == 'd' || key == 'D') {
            this->toggleGateDetectFrequency();
        }
    }
}

void ofApp::updateArLapModeSettings() {
    switch (this->arLapMode) {
        case ARAP_MODE_ULOOSE:
            this->arMarkerNumThreshold = 1;
            break;
        default:
            this->arMarkerNumThreshold = 2;
            break;
    }
}

//--------------------------------------------------------------
void ofApp::toggleARLap() {
    if (this->arLapMode == ARAP_MODE_NORM) {
        this->arLapMode = ARAP_MODE_MIDDLE;
    } else if (this->arLapMode == ARAP_MODE_MIDDLE) {
        this->arLapMode = ARAP_MODE_LOOSE;
    } else if (this->arLapMode == ARAP_MODE_LOOSE) {
        this->arLapMode = ARAP_MODE_ULOOSE;
    } else {
        this->arLapMode = ARAP_MODE_NORM;
    }
    this->updateArLapModeSettings();
    this->saveSettingsFile();
}

//--------------------------------------------------------------
void ofApp::changeFlickerLength(int val) {
    this->flickerLength += val;
    if (this->flickerLength < 0) {
        this->flickerLength = 1000;
    } else if (this->flickerLength > 1000) {
        this->flickerLength = 0;
    }
    this->saveSettingsFile();
}

//--------------------------------------------------------------
void ofApp::changeArucoMinSize(int val) {
    this->arucoMinSize += val;
    if (this->arucoMinSize < 1) {
        this->arucoMinSize = 1;
    } else if (this->arucoMinSize > 20) {
        this->arucoMinSize = 20;
    }
    // Update the Aruco detector with the new size immediately
    this->setMinDetectSize();
    this->saveSettingsFile();
}

//--------------------------------------------------------------
void ofApp::toggleLog() {
    this->logEnabled = !this->logEnabled;
    this->saveSettingsFile();
}

//--------------------------------------------------------------
void ofApp::toggleGateDetectFrequency() {
    this->gateDetectAllFrames = !this->gateDetectAllFrames;
    this->saveSettingsFile();
}

//--------------------------------------------------------------
void ofApp::keyPressedCamCheck(int key) {
    if (key == ' ') {
        setupMain();
    } else if (ofGetKeyPressed(OF_KEY_ESC)) {
        ofExit();
    }
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (tvpScene == SCENE_INIT) {
        this->setupCamCheck();
        return;
    } else if (tvpScene == SCENE_CAMS) {
        this->keyPressedCamCheck(key);
        return;
    }
    this->raceResultTimer = -1;
    switch (this->overlayMode) {
        case OVLMODE_HELP:
            this->keyPressedOverlayHelp(key);
            break;
        case OVLMODE_MSG:
            this->keyPressedOverlayMessage(key);
            break;
        case OVLMODE_NONE:
            this->keyPressedOverlayNone(key);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------
void ofApp::setMinDetectSize() {
    for (int i = 0; i < this->cameraNum; i++) {
        camView[i].aruco.setMinMaxMarkerDetectionSize(this->arucoMinSize*0.01f, 0.25);
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
    this->activateCursor();
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    this->activateCursor();
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    this->activateCursor();
}



//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    // overlay
    this->loadOverlayFont();
    if (tvpScene != SCENE_MAIN) {
        return;
    }
    // view
    this->setViewParams();
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
    for (int i = 0; i < this->cameraNum; i++) {
        camView[i].aruco.setThreaded(false);
    }
}

//--------------------------------------------------------------
void ofApp::grabberUpdateResize(int cidx) {
    grabber[cidx].update();
    if (grabber[cidx].isFrameNew() == false) {
        return;
    }

    if (!resourcesAllocated && grabber[cidx].getWidth() > 0) {
        for (int i = 0; i < this->cameraNum; i++) {
            tvpCamView* cv = &camView[i];
            float aspectRatio = (float)cv->cropW / cv->cropH;
            int targetWidth = CAMERA_WIDTH;
            int targetHeight = CAMERA_HEIGHT;

            if (cv->isWide == true) {
                targetHeight = CAMERA_HEIGHT * 0.75;
            }

            int newWidth, newHeight;
            float targetAspectRatio = (float)targetWidth / targetHeight;

            if (aspectRatio > targetAspectRatio) {
                newWidth = targetWidth;
                newHeight = round(targetWidth / aspectRatio);
            } else {
                newHeight = targetHeight;
                newWidth = round(targetHeight * aspectRatio);
            }
            cv->resizedImage.allocate(newWidth, newHeight);
        }
        resourcesAllocated = true;
    }

    if (!resourcesAllocated) return;

    tvpCamView* cv = &camView[cidx];

    if (cv->needCrop == false && cv->needResize == false) {
        cv->resizedImage.setFromPixels(grabber[cidx].getPixels());
        return;
    }

    ofxCvColorImage sourceImage;
    sourceImage.allocate(grabber[cidx].getWidth(), grabber[cidx].getHeight());
    sourceImage.setFromPixels(grabber[cidx].getPixels());

    if (cv->needCrop == true) {
        sourceImage.setROI(cv->cropX, cv->cropY, cv->cropW, cv->cropH);
    }

    cv->resizedImage.scaleIntoMe(sourceImage, CV_INTER_AREA);

    if (cv->needCrop == true) {
        sourceImage.resetROI();
    }
}

void ofApp::grabberUpdateResizeMulti() {
    grabber[0].update();
    if (grabber[0].isFrameNew() == false) return;

    if (!resourcesAllocated && grabber[0].getWidth() > 0) {
        multicamSource.allocate(grabber[0].getWidth(), grabber[0].getHeight());
        
        for (int i = 0; i < this->cameraNum; i++) {
            tvpCamView* cv = &camView[i];
            float aspectRatio = (float)cv->cropW / cv->cropH;
            int targetWidth = CAMERA_WIDTH;
            int targetHeight = CAMERA_HEIGHT;

            if (cv->isWide == true) {
                targetHeight = CAMERA_HEIGHT * 0.75;
            }

            int newWidth, newHeight;
            float targetAspectRatio = (float)targetWidth / targetHeight;

            if (aspectRatio > targetAspectRatio) {
                newWidth = targetWidth;
                newHeight = round(targetWidth / aspectRatio);
            } else {
                newHeight = targetHeight;
                newWidth = round(targetHeight * aspectRatio);
            }
            cv->resizedImage.allocate(newWidth, newHeight);
        }
        resourcesAllocated = true;
    }

    if (!resourcesAllocated) return;

    multicamSource.setFromPixels(grabber[0].getPixels());

    tvpCamView* cv;
    for (int i = 0; i < this->cameraNum; i++) {
        cv = &camView[i];
        
        multicamSource.setROI(cv->cropX, cv->cropY, cv->cropW, cv->cropH);
        cv->resizedImage.scaleIntoMe(multicamSource, CV_INTER_AREA);
        multicamSource.resetROI();
    }
}

//--------------------------------------------------------------
void ofApp::setupColors() {
    // common
    this->myColorYellow = ofColor(COLOR_YELLOW);
    this->myColorWhite = ofColor(COLOR_WHITE);
    this->myColorLGray = ofColor(COLOR_LGRAY);
    this->myColorDGray = ofColor(COLOR_DGRAY);
    this->myColorBGDark = ofColor(COLOR_BG_DARK);
    this->myColorBGMiddle = ofColor(COLOR_BG_MIDDLE);
    this->myColorBGLight = ofColor(COLOR_BG_LIGHT);
    this->myColorAlert = ofColor(COLOR_ALERT);
}

//--------------------------------------------------------------
void ofApp::setViewParams() {
    int i;
    int width = ofGetWidth();
    int height = ofGetHeight();
    //float ratio = (float)width / (float)height;
    switch (this->cameraNumVisible) {
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
    for (i = 0; i < this->cameraNumVisible; i++) {
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
int ofApp::calcViewParam(int target, int current, int steps) {
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
void ofApp::updateViewParams() {
    int i, steps;
    for (i = 0; i < this->cameraNumVisible; i++) {
        // normal view
        steps = camView[i].moveSteps;
        if (steps == 0) {
            continue;
        }
        // camera
        camView[i].width = this->calcViewParam(camView[i].widthTarget, camView[i].width, steps);
        camView[i].height = this->calcViewParam(camView[i].heightTarget, camView[i].height, steps);
        camView[i].posX = this->calcViewParam(camView[i].posXTarget, camView[i].posX, steps);
        camView[i].posY = this->calcViewParam(camView[i].posYTarget, camView[i].posY, steps);
        if (this->logEnabled) ofLogNotice("ofApp::updateViewParams") << "Cam " << i << ": Pos(" << camView[i].posX << "," << camView[i].posY << ") Size(" << camView[i].width << "," << camView[i].height << ")";
        camView[i].imageScale = (float)(camView[i].width) / (float)CAMERA_WIDTH;
        if (camView[i].isWide == true) {
            camView[i].posYWide = camView[i].posY + (camView[i].height / 8);
            camView[i].heightWide = camView[i].height * 0.75;
        }
        // lap
        camView[i].lapPosX = this->calcViewParam(camView[i].lapPosXTarget, camView[i].lapPosX, steps);
        camView[i].lapPosY = this->calcViewParam(camView[i].lapPosYTarget, camView[i].lapPosY, steps);
        camView[i].moveSteps--;
    }
}

//--------------------------------------------------------------
void ofApp::initConfig() {
    // system
    this->sysStatEnabled = DFLT_SYS_STAT;
    this->cameraNumVisible = this->cameraNum;
    // view mode
    this->cameraTrimEnabled = DFLT_CAM_TRIM;
    this->fullscreenEnabled = DFLT_FSCR_ENBLD;
    this->cameraFrameEnabled = DFLT_CAM_FRAMED;
    this->setViewParams();
    // AR lap timer
    this->setOverlayMode(OVLMODE_NONE);
    this->raceStarted = false;
    this->initRaceVars();
    // finish
    this->xmlPilots.clear();
    this->saveSettingsFile();
    this->setOverlayMessage("Initialized settings");
}
//--------------------------------------------------------------
void ofApp::toggleSysStat() {
    this->sysStatEnabled = !this->sysStatEnabled;
    this->saveSettingsFile();
}
//--------------------------------------------------------------
void ofApp::initRaceVars() {
    for (int i = 0; i < this->cameraNum; i++) {
        camView[i].foundMarkerNum = 0;
        camView[i].foundValidMarkerNum = 0;
        camView[i].enoughMarkers = false;
        camView[i].flickerCount = 0;
        camView[i].flickerValidCount = 0;
        camView[i].rssiOutput = false;
        camView[i].isDroneInGate = false;
        camView[i].lastValidMarkerId = -1; // Initialize with an invalid ID
    }
    this->elapsedTime = 0;
}
void ofApp::onStageReady(ofxOscMessage& m) {
    if (this->logEnabled) ofLogNotice("ofApp::onStageReady") << "Received /stage_ready message. Starting race immediately.";

    this->raceStartTime = ofGetElapsedTimef(); // Set race start time to current time
    this->raceStarted = true;

    if (this->logEnabled) ofLogNotice("ofApp::onStageReady") << "Race started immediately. raceStartTime set to: " << this->raceStartTime;
}

//--------------------------------------------------------------
void ofApp::startRace() {
    if (this->raceStarted == true) {
        if (this->logEnabled) ofLogNotice("ofApp::startRace") << "Race already started by external command. Skipping internal start.";
        return;
    }
    this->initRaceVars();
    this->raceStarted = true;
    ofResetElapsedTimeCounter();
    // Store race start time for lap_time calculation
    this->raceStartTime = ofGetElapsedTimef();
}


//--------------------------------------------------------------
void ofApp::toggleFullscreen() {
    this->fullscreenEnabled = !this->fullscreenEnabled;
    ofSetFullscreen(this->fullscreenEnabled);
    this->saveSettingsFile();
}

//--------------------------------------------------------------
void ofApp::setOverlayMode(int mode) {
    this->overlayMode = mode;
    if (mode != OVLMODE_MSG) {
        this->initOverlayMessage();
    }
}

//--------------------------------------------------------------
void ofApp::loadOverlayFont() {
    int h = (ofGetHeight() - (OVLTXT_MARG * 2)) / OVLTXT_LINES * 0.7;
    if (this->myFontOvlayP.isLoaded()) {
        this->myFontOvlayP.unloadFont();
    }
    if (this->myFontOvlayP2x.isLoaded()) {
        this->myFontOvlayP2x.unloadFont();
    }
    if (this->myFontOvlayM.isLoaded()) {
        this->myFontOvlayM.unloadFont();
    }
    this->myFontOvlayP.load(FONT_P_FILE, h);
    this->myFontOvlayP2x.load(FONT_P_FILE, h * 2);
    this->myFontOvlayM.load(FONT_M_FILE, h);
}

//--------------------------------------------------------------
void ofApp::drawStringBlock(ofxTrueTypeFontUC *font, string text,
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
    ofSetColor(this->myColorWhite);
    font->drawString(text, x, y);
}

//--------------------------------------------------------------
void ofApp::drawLineBlock(int xblock1, int xblock2, int yline, int blocks, int lines) {
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
    ofSetColor(this->myColorYellow);
    ofDrawRectangle(x, y, w, h);
}

//--------------------------------------------------------------
void ofApp::drawULineBlock(int xblock1, int xblock2, int yline, int blocks, int lines) {
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
    ofSetColor(this->myColorDGray);
    ofDrawRectangle(x, y, w, h);
}

//--------------------------------------------------------------
void ofApp::drawHelp() {
    int szl = HELP_LINES;
    int line;
    // background
    ofSetColor(this->myColorBGDark);
    ofFill();
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    // title(3 lines)
    line = 1;
    ofSetColor(this->myColorYellow);
    this->drawStringBlock(&this->myFontOvlayP2x, "Settings / Commands", 0, line, ALIGN_CENTER, 1, szl);
    line += 2;
    // body
    this->drawHelpBody(line);
    // message(2 lines)
    line = HELP_LINES - 1;
    ofSetColor(this->myColorYellow);
    this->drawStringBlock(&this->myFontOvlayP, "Press H or Esc key to exit", 0, line, ALIGN_CENTER, 1, szl);
}

//--------------------------------------------------------------
void ofApp::drawHelpBody(int line) {
    string value;
    int szl = HELP_LINES;
    int szb = 21;
    int blk0 = 6;
    int blk1 = 3;
    int blk2 = 12;
    int blk3 = 16;
    int blk4 = 17;

    // SYSTEM
    ofSetColor(this->myColorWhite);
    this->drawStringBlock(&this->myFontOvlayP, "System Command", blk0, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "Setting", blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "Key", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    ofSetColor(this->myColorYellow);
    this->drawLineBlock(blk1, blk4, line, szb, szl);
    line++;
    ofSetColor(this->myColorWhite);
    // Set system statistics
    ofSetColor(this->myColorDGray);
    this->drawULineBlock(blk1, blk4, line + 1, szb, szl);
    ofSetColor(this->myColorWhite);
    value = this->sysStatEnabled ? "On" : "Off";
    this->drawStringBlock(&this->myFontOvlayP, "Set System Statistics", blk1, line, ALIGN_LEFT, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, value, blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "S", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    // Display help
    ofSetColor(this->myColorDGray);
    this->drawULineBlock(blk1, blk4, line + 1, szb, szl);
    ofSetColor(this->myColorWhite);
    this->drawStringBlock(&this->myFontOvlayP, "Display Help (Settings/Commands)", blk1, line, ALIGN_LEFT, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "-", blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "H", blk3, line, ALIGN_CENTER, szb, szl);
    line++;

    // VIEW
    line++;
    line++;
    ofSetColor(this->myColorWhite);
    this->drawStringBlock(&this->myFontOvlayP, "View Command", blk0, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "Setting", blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "Key", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    ofSetColor(this->myColorYellow);
    this->drawLineBlock(blk1, blk4, line, szb, szl);
    line++;
    ofSetColor(this->myColorWhite);
    // Set fullscreen mode
    ofSetColor(this->myColorDGray);
    this->drawULineBlock(blk1, blk4, line + 1, szb, szl);
    ofSetColor(this->myColorWhite);
    value = this->fullscreenEnabled ? "On" : "Off";
    this->drawStringBlock(&this->myFontOvlayP, "Set Fullscreen Mode", blk1, line, ALIGN_LEFT, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, value, blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "F, Esc", blk3, line, ALIGN_CENTER, szb, szl);
    // VIEW
    line++;
    line++;
    ofSetColor(this->myColorWhite);
    this->drawStringBlock(&this->myFontOvlayP, "Gate Detect Settings", blk0, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "Setting", blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "Key", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    ofSetColor(this->myColorYellow);
    this->drawLineBlock(blk1, blk4, line, szb, szl);
    line++;
    ofSetColor(this->myColorWhite);
    // Gate Detect Settings
    value = (this->arLapMode == ARAP_MODE_NORM) ? "Normal" : ((this->arLapMode == ARAP_MODE_MIDDLE) ? "Middle" : ((this->arLapMode == ARAP_MODE_LOOSE) ? "Loose" : "UltraLoose"));
    this->drawStringBlock(&this->myFontOvlayP, "Set AR Lap Timer Mode", blk1, line, ALIGN_LEFT, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, value, blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "A", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    // Flicker Threshold
    ofSetColor(this->myColorDGray);
    this->drawULineBlock(blk1, blk4, line + 1, szb, szl);
    ofSetColor(this->myColorWhite);
    value = ofToString(this->flickerLength);
    this->drawStringBlock(&this->myFontOvlayP, "Anti Flicker Length(Default 100 millisecondss)", blk1, line, ALIGN_LEFT, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, value, blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "Left/Right", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    // ArUco Min Size
    ofSetColor(this->myColorDGray);
    this->drawULineBlock(blk1, blk4, line + 1, szb, szl);
    ofSetColor(this->myColorWhite);
    value = ofToString(this->arucoMinSize, 2) +"%"; // Display up to 2 decimal places
    this->drawStringBlock(&this->myFontOvlayP, "ArUco Min Size (% of Screen Width)", blk1, line, ALIGN_LEFT, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, value, blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "Up/Down", blk3, line, ALIGN_CENTER, szb, szl);
    line++;
    // Gate Detect Frequency
    ofSetColor(this->myColorDGray);
    this->drawULineBlock(blk1, blk4, line + 1, szb, szl);
    ofSetColor(this->myColorWhite);
    value = this->gateDetectAllFrames ? "All Frames" : "Odd/Even Frames";
    this->drawStringBlock(&this->myFontOvlayP, "Gate Detect Frequency", blk1, line, ALIGN_LEFT, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, value, blk2, line, ALIGN_CENTER, szb, szl);
    this->drawStringBlock(&this->myFontOvlayP, "D", blk3, line, ALIGN_CENTER, szb, szl);
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

    ofSetColor(this->myColorWhite);



}

//--------------------------------------------------------------
void ofApp::initOverlayMessage() {
    this->ovlayMsgTimer = 0;
    this->ovlayMsgString = "";
}

//--------------------------------------------------------------
void ofApp::setOverlayMessage(string msg) {
    if (this->overlayMode != OVLMODE_NONE && this->overlayMode != OVLMODE_MSG) {
        return;
    }
    this->ovlayMsgTimer = OLVMSG_TIME;
    this->ovlayMsgString = msg;
    this->setOverlayMode(OVLMODE_MSG);
}

//--------------------------------------------------------------
void ofApp::drawOverlayMessageCore(ofxTrueTypeFontUC *font, string msg) {
    float sw, fh, sx, sy, margin;
    margin = 10;
    sw = font->stringWidth(msg);
    fh = font->getFontSize();
    sx = (ofGetWidth() / 2) - (sw / 2);
    sy = (ofGetHeight() / 2) + (fh / 2);
    // background
    ofSetColor(this->myColorBGDark);
    ofFill();
    ofDrawRectangle(sx - margin, sy - (fh + margin), sw + (margin * 2), fh + (margin * 2));
    // message
    ofSetColor(this->myColorWhite);
    font->drawString(msg, sx, sy);
}

//--------------------------------------------------------------
void ofApp::drawOverlayMessage() {
    ofxTrueTypeFontUC *font = &this->myFontLap;
    string msg = this->ovlayMsgString;
    this->drawOverlayMessageCore(font, msg);
}



//--------------------------------------------------------------
void ofApp::activateCursor() {
    if (this->hideCursorTimer <= 0) {
        ofShowCursor();
    }
    this->hideCursorTimer = HIDECUR_TIME;
}
