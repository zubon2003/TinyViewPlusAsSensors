/*
 * ofxAruco.h
 *
 *  Created on: 02/10/2011
 *      Author: arturo, t-asano
 */

#ifndef OFXARUCO_H_
#define OFXARUCO_H_

#include "ofConstants.h"
#include "ofPixels.h"
#include "ofThread.h"

#include "markerdetector.h"
#include "highlyreliablemarkers.h"
#include "Poco/Condition.h"

class ofxAruco: public ofThread {
public:
    ofxAruco();
    
    void setThreaded(bool threaded);
    void setUseHighlyReliableMarker(string dictionaryFile);
    void setup2d(float w, float h, float markerSize=.15);

    void detectMarkers(ofPixels & pixels);
    int getNumRectangles();
    int getNumMarkers();
    int getNumMarkersValidGate();
    const vector<aruco::Marker>& getMarkers() const; // Add this line
    void draw2dGate(ofColor valid, ofColor invalid, bool showId);
    double getFps();

    void setMarkerSize(float markerSizeInMeter);
    void setMinMaxMarkerDetectionSize(float minSize, float maxSize); // in fraction of camera width
    void setThresholdParams(double param1, double param2);
    void setThresholdMethod(aruco::MarkerDetector::ThresholdMethods method);

private:
    void threadedFunction();
    void findMarkers(ofPixels & pixels);
    bool isValidGate(cv::Point2f p0, cv::Point2f p1);

    ofPixels frontPixels, backPixels;
    bool newDetectMarkers;
    Poco::Condition condition;
    
    aruco::MarkerDetector detector;
    vector<aruco::Marker> markers, backMarkers, intraMarkers;

    aruco::CameraParameters camParams;
    float markerSize;
    cv::Size size;
    cv::Point2f center;

    bool threaded;
    bool foundMarkers;

    ofFpsCounter fpsCounter;
};

#endif /* OFXARUCO_H_ */
