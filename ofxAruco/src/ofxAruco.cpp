/*
 * ofxAruco.cpp
 *
 *  Created on: 02/10/2011
 *      Author: arturo, t-asano
 */

#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxAruco.h"

ofxAruco::ofxAruco()
:threaded(true)
,newDetectMarkers(false) {
}

void ofxAruco::setThreaded(bool _threaded) {
    threaded = _threaded;
    if (isThreadRunning() && !threaded) {
        stopThread();
        condition.signal();
        waitForThread(false, 3000);
    }
}

void ofxAruco::setUseHighlyReliableMarker(string dictionaryFile) {
    // load dictionary
    aruco::Dictionary D;
    string path = ofFilePath::getAbsolutePath(dictionaryFile);
    if (D.fromFile(path) == false) {
        cerr << "Could not open dictionary" << endl;
    };
    if (D.size() == 0) {
        cerr << "Invalid dictionary" << endl;
    };
    aruco::HighlyReliableMarkers::loadDictionary(D);
    // setup parameters
    detector.enableLockedCornersMethod(false);
    detector.setMarkerDetectorFunction(aruco::HighlyReliableMarkers::detect);
    detector.setThresholdParams(21, 7);
    detector.setCornerRefinementMethod(aruco::MarkerDetector::LINES);
    detector.setWarpSize((D[0].n() + 2) * 8);
}

void ofxAruco::setup2d(float w, float h, float _markerSize) {
    size.width = w;
    size.height = h;
    center.x = w / 2;
    center.y = h / 2;
    markerSize = _markerSize;
    detector.setThresholdMethod(aruco::MarkerDetector::ADPT_THRES);
    if (threaded) startThread();
}

void ofxAruco::detectMarkers(ofPixels & pixels) {
    if (!threaded) {
        findMarkers(pixels);
        fpsCounter.newFrame();
    } else {
        lock();
        frontPixels = pixels;
        newDetectMarkers = true;
        if (foundMarkers) {
            swap(markers, intraMarkers); // copy to "markers"
            foundMarkers = false;
            fpsCounter.newFrame();
        }
        condition.signal();
        unlock();
    }
}

void ofxAruco::findMarkers(ofPixels & pixels) {
    cv::Mat mat = ofxCv::toCv(pixels);
    detector.detect(mat, backMarkers, camParams, markerSize);
    if (threaded) {
        lock();
        swap(backMarkers, intraMarkers); // copy to "intraMarkers"
        foundMarkers = true;
        unlock();
    } else {
        swap(backMarkers, markers);
    }
}

int ofxAruco::getNumRectangles() {
    return detector.getNumRectangles();
}

int ofxAruco::getNumMarkers() {
    return markers.size();
}

const vector<aruco::Marker>& ofxAruco::getMarkers() const {
    return markers;
}

int ofxAruco::getNumMarkersValidGate() {
    int num = 0;
    for (int i = 0; i < markers.size(); i++) {
        // check outer line
        if (isValidGate(markers[i][3], markers[i][2])) {
            num++;
        }
    }
    return num;
}

bool ofxAruco::isValidGate(cv::Point2f p0, cv::Point2f p1) {
    cv::Point2f vec_a = p1 - center;
    cv::Point2f vec_b = p0 - center;
    if ((vec_a.x * vec_b.y) > (vec_a.y * vec_b.x)) {
        return true;
    } else {
        return false;
    }
}

void ofxAruco::draw2dGate(ofColor valid, ofColor invalid, bool showId) {
    for (int i = 0; i < markers.size(); i++) {
        cv::Point2f p0, p1;
        ofPoint ctr(0, 0);
        // check outer line
        if (isValidGate(markers[i][3], markers[i][2])) {
            ofSetColor(valid);
        } else {
            ofSetColor(invalid);
        }
        for (int j = 0; j < 4; j++) {
            p0 = markers[i][j];
            p1 = markers[i][(j + 1) % 4];
            ofDrawLine(p0.x, p0.y, p1.x, p1.y);
            if (showId) {
                ctr.x += p0.x;
                ctr.y += p0.y;
            }
        }
        if (showId) {
            ctr.x /= 4.;
            ctr.y /= 4.;
            ofDrawBitmapString(ofToString(markers[i].id), ctr);
        }
    }
}

double ofxAruco::getFps()
{
    return fpsCounter.getFps();
}

void ofxAruco::setMarkerSize(float markerSize_)
{
    markerSize = markerSize_;
}

void ofxAruco::setMinMaxMarkerDetectionSize(float minSize, float maxSize)
{
    detector.setMinMaxSize(minSize, maxSize);
}

void ofxAruco::setThresholdParams(double param1, double param2) {
    detector.setThresholdParams(param1, param2);
}

void ofxAruco::setThresholdMethod(aruco::MarkerDetector::ThresholdMethods method) {
    detector.setThresholdMethod(method);
}

void ofxAruco::threadedFunction() {
    while (isThreadRunning()) {
        lock();
        if (!newDetectMarkers) {
            condition.wait(mutex);
            if (!threaded) {
                newDetectMarkers = false;
                unlock();
                continue;
            }
        }
        swap(frontPixels, backPixels);
        newDetectMarkers = false;
        unlock();
        findMarkers(backPixels);
    }
}