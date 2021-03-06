//
//  ofxThreadSafeLog.cpp
//  CollectionTable
//
//  Created by Oriol Ferrer Mesià on 17/02/15.
//
//

#include "ofxThreadSafeLog.h"

ofxThreadSafeLog* ofxThreadSafeLog::singleton = NULL;

static ofMutex ofxThreadSafeLogMutex;


ofxThreadSafeLog::ofxThreadSafeLog(){
	startThread();
}

void ofxThreadSafeLog::setPrintToConsole(bool print){
	alsoPrintToConsole = print;
}


void ofxThreadSafeLog::close(){
	stopThread();
	if(isThreadRunning()){
		waitForThread();
		lock();
		map<string, ofFile*>::iterator it = logs.begin();
		while(it != logs.end()){
			it->second->close();
			delete it->second;
			++it;
		}
		pendingLines.clear();
		logs.clear();
		logFilesPendingCreation.clear();
		unlock();
	}
}


ofxThreadSafeLog* ofxThreadSafeLog::one(){

	ofScopedLock lock(ofxThreadSafeLogMutex);
	if (!singleton){   // Only allow one instance of class to be generated.
		singleton = new ofxThreadSafeLog();
	}
	return singleton;
}


void ofxThreadSafeLog::append(const string& logFile, const string& line){

	lock();
	map<string,vector<string> >::iterator it = pendingLines.find(logFile);
	if(it == pendingLines.end()){ //new log file!
		logFilesPendingCreation.push_back(logFile);
	}
	pendingLines[logFile].push_back(line);
	if(alsoPrintToConsole){
		ofLogNotice(ofFilePath::getFileName(logFile)) << line;
	}
	unlock();
}


void ofxThreadSafeLog::threadedFunction(){

	#ifdef TARGET_WIN32
	#elif defined(TARGET_LINUX)
	pthread_setname_np(pthread_self(), "ofxThreadSafeLog");
	#else
	pthread_setname_np("ofxThreadSafeLog");
	#endif

	while(isThreadRunning()){
		lock();

		// 1 create pending log files (new log!)
		if(logFilesPendingCreation.size()){
			vector<string>::iterator it = logFilesPendingCreation.begin();
			while( it != logFilesPendingCreation.end()){
				ofFile * newLog = new ofFile();
				newLog->open(*it, ofFile::WriteOnly, false); //TODO append mode?
				logs[*it] = newLog;
				++it;
			}
			logFilesPendingCreation.clear();
		}

		// 2 write lines into logs
		map<string,vector<string> >::iterator it = pendingLines.begin();
		while( it != pendingLines.end()){
			string logFile = it->first;
			ofFile * log = logs[logFile];
			vector<string> & lines = it->second;

			vector<string>::iterator it2 = lines.begin();
			while(it2 != lines.end()){
				*log << ofGetTimestampString("%Y/%m/%d %H:%M:%S") << " - " << *it2 << endl;
				++it2;
			}
			lines.clear();
			++it;
		}
		unlock();
		ofSleepMillis(16);
	}
}
