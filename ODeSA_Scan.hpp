///@file Scan.hpp
///@brief Processor for Clovers at Fragmentation Facilities (Based HEAVILY on CloverProcessor.cpp)
///@authors T.T. King

#ifndef ODESA_SCAN_CORE_H
#define ODESA_SCAN_CORE_H

//! Standard Header Section
#include <signal.h>

#include <fstream>
#include <iostream>
#include <string>
#include <getopt.h>

//! ROOT Header Section
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
    ODeSA_Scan()=default;

    ~ODeSA_Scan()=default;

    void ProcessFile(ifstream* file_);

    int OpenInputFiles(string prefix);

    int ParseCmdFlags(int argc, char* argv[]);

    void InitOutput(string outFile_, string outFileHeader_, int numFiles_);

    /* Set and Get Methods for various things */
    void SetNumberOfFiles(int a) { numberOfFiles_ = a; }
    void SetBatchMode(bool a) { batchMode_ = a; }
    void SetDebugMode(bool a) { debugMode_ = a; }
    void SetOutputFileName(string a) { outName_ = a; }
    void SetInputFileName(string a) { inName_ = a; }
    void SetOutputFileHeader(string a) { header_ = a; }

    int GetNumberOfFiles() { return numberOfFiles_; }
    bool GetBatchMode() { return batchMode_; }
    bool GetDebugMode() { return debugMode_; }
    string GetOutputFileName() { return outName_; }
    string GetInputFileName() { return inName_; }
    string GetOutputFileHeader() { return header_; }

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
        cout << "  --help (-?)           | Show this help\n\n\n";
    }

    typedef struct {
        Float_t l;    // Long integral
        Float_t s;    // Short integral
        Float_t amp;  // Amplitude
        Float_t cfd;  // Trigger time
        Float_t psd;  // PSD parameter s/l
        Float_t trg;  // Detector trigger
        Float_t pp;   // Peak position
    } Can;

    int numberOfFiles_ = 0;
    double runtime = 0; 

    bool batchMode_ = false;
    bool debugMode_ = false;
    string outName_ = " ";
    string inName_ = " ";
    string header_ = " ";
    vector<Can> detVec;
    vector<TH1F> debug_Trace_vector;

 
    
};
#endif