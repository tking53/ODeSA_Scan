///@file Scan.hpp
///@brief Processor for Clovers at Fragmentation Facilities (Based HEAVILY on CloverProcessor.cpp)
///@authors T.T. King

#ifndef ODESA_SCAN_CORE_H
#define ODESA_SCAN_CORE_H

//! Standard Header Section
#include <getopt.h>
#include <signal.h>

#include <algorithm>
#include <iterator>
#include <fstream>
#include <iostream>
#include <string>

//! ROOTde Header Section
#include "TF1.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TMath.h"
#include "TTree.h"

//! Custom Headers
#include "PulseAnalysis.h"

class ODeSA_Scan {
   public:

     typedef struct LiquidCan{
        Float_t l;    // Long integralP
        Float_t s;    // Short integral
        Float_t amp;  // Amplitude
        Float_t ampRaw;  // Non Baseline subtracted Amplitude 
        Float_t cfd;  // Trigger time
        Float_t psd;  // PSD parameter s/l
        Float_t pp;   // Peak position
        Bool_t trig;  // Detector trigger
    } LiquidCan; 

    ODeSA_Scan()=default;

    ~ODeSA_Scan()=default;

    int ProcessSingleEvent(ifstream* file_,  PulseAnalysis* Analysis_,  LiquidCan &detector, int chanNum_, double &cutTime, long &eventNum_, bool &gotMoreData, bool doTraceAnalysis  = true, pair<int,int> cfdRange_ = {1, 1}, pair<int,int> psdRange_ = {20, 200});

    map<int,ifstream*>  OpenInputFiles(string prefix_, int NumOfFiles_, vector<int> &listOfFiles_);

    int ParseCmdFlags(int argc, char* argv[]);

    TTree* InitOutput(string outFile_, string outFileHeader_, int numFiles_, vector<int> listOfFiles_);

    int ODeSA_Cfd(pair<int,int> cfdRange_, vector<float> trace_, int &peakPosition_, float &amplitude);

    int Extras_Cfd(pair<int, int> cfdRange_, vector<float> trace_, int &peakPosition_, float &amplitude);

    pair<double, double> ODeSA_PSD(pair<int, int> psdRange_, vector<float> trace_, int peakPosition_, int offset = 24);

    pair<double, double> Extras_PSD(pair<int, int> psdRange_, vector<float> trace_, int peakPosition_, int offset = 24);

    /* Set and Get Methods for various things */
    void SetNumberOfFiles(int a) { numberOfFiles_ = a; }
    void SetBatchMode(bool a) { batchMode_ = a; }
    void SetDebugMode(bool a) { debugMode_ = a; }
    void SetOutputFileName(string a, string b) { outName_ = a + b; }
    void SetInputFileName(string a) { inName_ = a; }
    void SetOutputFileHeader(string a) { header_ = a; }
    void SetRunTime(double a ) { runtime = a ; }
    void SetPreviousTime(double a) { previousTime = a; }
    void SetVerboseTerminalErrors(bool a) {verboseErrorMode_ =a;}

    int GetNumberOfFiles() { return numberOfFiles_; }
    bool GetBatchMode() { return batchMode_; }
    bool GetDebugMode() { return debugMode_; }
    bool GetVerboseTerminalErrors() {return verboseErrorMode_;}
    string GetOutputFileName() { return outName_; }
    string GetInputFileName() { return inName_; }
    string GetOutputFileHeader() { return header_; }
    double GetRunTime() { return runtime; }
    double GetPreviousTime() { return previousTime; }
    map<int,LiquidCan>* GetDetectorMap() { return &detMap; }

    //    private:
    /* Print help dialogue for command line options. */
    void ScanHelp() {
        cout << "\n SYNTAX: .\\odesa_scan [options]\n";
        cout << "  --numChans (-n)       | Set number of channels (i.e. numFiles)\n\n";
        cout << "  --batch (-b)          | Set batch mode\n";
        cout << "  --output (-o)         | Set output root file name\n";
        cout << "  --header (-h)         | Set header\n";
        cout << "  --input (-i)          | Set input filename \n";
        cout << "  --debug (-d)          | Set debug mode (addes traces to tree)\n\n";
        cout << "  --verbose (-v)        | Sets verbose CFD errors (default = false)\n\n";
        cout << "  --help (-?)           | Show this help\n\n\n";
    }

    void PrintStartMessage(){
//! __DATE__ and __TIME__ are compiler macros which process out to the time the binary exe is built. They are currently aligned (output takes less space than the actual macro call) 
    cout << " ------------------------------------------------ " << endl;
    cout << " |   ODeSA WaveDump Scan - binary version        |" << endl;
    cout << " |   Scan Build Date: "<<__DATE__<<"                |" << endl;
    cout << " |   Scan Build Time: "<<__TIME__<<"                   |" << endl;
    cout << " |   Experiment: LANSCE                          |" << endl;
    cout << " |   Exp Date: Oct 2021                          |" << endl;
    cout << " |   Calibration used: None                      |" << endl;
    cout << " |   ORNL Physics Division                       |" << endl;
    cout << " ------------------------------------------------ " << endl;

    }

    int numberOfFiles_ = 0;
    double runtime = 0;
    double previousTime =0 ;
    long TEvt = 0;

    bool batchMode_ = false;
    bool verboseErrorMode_ = false;
    // bool foundBadCFDtraces
    bool debugMode_ = false;
    string outName_ = " ";
    string inName_ = " ";
    string header_ = " ";
    
    map<int,TH1F*> debug_Trace_map;
    map<int,ODeSA_Scan::LiquidCan> detMap;
    static vector<int> listOfFiles_;

    /** ----------------------------------------------------
     *	Channel Map used for picking which CFD to use 
     *   ----------------------------------------------------
     */

    map<int, string> chanMap = {{0,  "RF"},
                                {1,  "SCATTERING"},
                                {2,  "ODESA"},
                                {3,  "ODESA"},
                                {4,  "ODESA"},
                                {5,  "ODESA"},
                                {6,  "ODESA"},
                                {7,  "ODESA"},
                                {8,  "ODESA"},
                                {9,  "ODESA"},
                                {10, "ODESA"},
                                {11, "ODESA"},
                                {12, "ODESA"},
                                {13, "ODESA"},
                                {14, "ODESA"},
                                {15, "ODESA"}};

    /** ----------------------------------------------------
     *	Calibrations and threshold
     *   ----------------------------------------------------
     */

    // map settings are coefficents in rising powers of X (i .e. <offset,slope>)
    map<int, pair<float, float>> calibrations = {
        {0,{0., 1.}},
        {1,{0., 1.}},
        {2,{0., 1.}},
        {3,{0., 1.}},
        {4,{0., 1.}},
        {5,{0., 1.}},
        {6,{0., 1.}},
        {7,{0., 1.}},
        {8,{0., 1.}},
        {9,{0., 1.}},
        {10,{0., 1.}},
        {11,{0., 1.}},
        {12,{0., 1.}},
        {13,{0., 1.}},
        {14,{0., 1.}},
        {15,{0., 1.}}};

    vector<float> threshold = {
        15500,
        15500,
        15500,
        15500,
        15800,
        15800,
        15800,
        15500,
        15700,
        15700,
        15700,
        15700,
        15700,
        15700,
        15700,
        15700};
};
#endif