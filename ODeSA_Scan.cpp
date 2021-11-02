/************************************************************************
 *
 *  Filename: Scan.cpp
 *s
 *  Description:
 *
 *	Author(s):
 *     Michael T. Febbraro
 *     David Walter
 *     Toby King (rewrite)
 *
 *  Original Creation Date: 9/25/2016
 *  Rewrite Date: 11/01/2021
 *  Last modified: 
 *
 *
 *  If "error while loading shared libraries: libcore.so: ..." occurs, type
 *  "source `root-config --prefix`/bin/thisroot.sh"
 *
 * -----------------------------------------------------
 * 	Nuclear Astrophysics, Physics Division
 *  Oak Ridge National Laboratory
 *
 */

#include "ODeSA_Scan.hpp"
#include <string.h>

using namespace std;
#include "ODeSA_Scan.hpp"

int main(int argc, char *argv[]) {

    ODeSA_Scan scan; 

   scan.ParseCmdFlags(argc, argv);

    // cout<< " checking cmd flags" << endl;
    // cout<< " Number of Files= "<< scan.GetNumberOfFiles() <<endl;
    // cout<< " BatchMode= " << scan.GetBatchMode() <<endl;
    // cout<< " DebugMode= " << scan.GetDebugMode() <<endl;
    // cout<< " InputFile= " << scan.GetInputFileName() << endl;
    // cout<< " OutputFile= "<< scan.GetOutputFileName() <<endl;
    // cout<< " Header= " << scan.GetOutputFileHeader() << endl;

    //return Scan(numberOfFiles_, batchMode_, debugMode_, outName_, inName_, header_ );
}

int ODeSA_Scan::ParseCmdFlags(int argc, char *argv[]) {
    int idx = 0;
    int retval = 0;
    bool foundNumFiles_ = false;

    struct option longOpts[] = {
        {"numChans", required_argument, NULL, 'n'},
        {"batch", optional_argument, NULL, 'b'},
        {"output", required_argument, NULL, 'o'},
        {"header", required_argument, NULL, 'h'},
        {"input", required_argument, NULL, 'o'},
        {"debug", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, '?'},
        {NULL, no_argument, NULL, 0}};

    if (argc != 0) {
        while ((retval = getopt_long(argc, argv, "n:bh:i:o:d", longOpts, &idx)) != -1) {
            switch (retval) {
                case 'n':
                    if (optarg != 0) {
                        SetNumberOfFiles(atoi(optarg));
                        foundNumFiles_ = true;
                    } else {
                        cout << "ERROR:: Missing Number of channels (files) to merge and build." << endl;
                        return 1;
                    }
                    break;
                case 'b':
                    SetBatchMode(true);
                    break;
                case 'd':
                    SetDebugMode(true);
                    break;
                case 'o':
                    if (optarg != 0) {
                        SetOutputFileName(optarg);
                    } else {
                        cout << "ERROR:: Missing Output root file name." << endl;
                        return 1;
                    }
                    break;
                case 'h':
                    if (optarg != 0) {
                        SetOutputFileHeader(optarg);
                    } else {
                        cout << "ERROR:: Missing header string." << endl;
                        return 1;
                    }
                    break;
                case 'i':
                    if (optarg != 0) {
                        SetInputFileName(optarg);
                    } else {
                        cout << "ERROR:: Missing Input file name" << endl;
                        return 1;
                    }
                    break;
                case '?':
                    ScanHelp();
                    return 1;
                default:
                    cout << "ERROR:: Unknown comandline argument" << endl;
                    return 1;
            }  //end switch
        }      //end while
        if (!foundNumFiles_) {
            cout << "-n is mandatory" << endl;
            return 2;
        }
    }
    return 0;
}

void ODeSA_Scan::InitOutput(string outputFileName_, string outputFileHeader_, int numFiles_) {
    
    // static Can d0,d1,d2,d3,d4,d5,d6,d7

    TFile *ff = new TFile(outputFileName_.c_str(), "RECREATE");
    TTree *tt = new TTree("T", outputFileHeader_.c_str());

    tt->Branch("runtime", &runtime, "Runtime (ms)");  // Runtime in ms

    for (unsigned i = 0; i < numFiles_; ++i) {
        Can detector;
        detVec.emplace_back( detector );
        string key = "d" + to_string(i);
        tt->Branch(key.c_str(), &(detVec.at(i)), "l:s:amp:cfd:psd:trig:pp");
        
        if (GetDebugMode()){
            string traceKey_ = "trace" + to_string(i) + "_d";
            string traceTitle_ = "Debugging Traces for Channel " + to_string(i);
           // debug_Trace_vector.emplace_back(new TH1F(traceKey_.c_str(), traceTitle_.c_str(), 2000, 0, 1999));
            //tt->Branch(traceKey_.c_str(), "TH1F", &(debug_Trace_vector.at(i)));
        }
    }
}

