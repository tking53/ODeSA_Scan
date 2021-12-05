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
#include <iterator>

#include "ODeSA_Scan.hpp"

using namespace std;

int main(int argc, char *argv[]) {

    ODeSA_Scan scan; 
    PulseAnalysis *Analysis = new PulseAnalysis();

    int FlagReturn = scan.ParseCmdFlags(argc, argv);
    if (FlagReturn != 0) {
        return FlagReturn;

    } else {

        cout << "\n checking cmd flags" << endl;
        cout << " Number of Files= " << scan.GetNumberOfFiles() << endl;
        cout << " BatchMode= " << scan.GetBatchMode() << endl;
        cout << " DebugMode= " << scan.GetDebugMode() << endl;
        cout << " VerboseMode= " << scan.GetVerboseTerminalErrors() << endl;
        cout << " InputFile= " << scan.GetInputFileName() << endl;
        cout << " OutputFile= " << scan.GetOutputFileName() << endl;
        cout << " Header= " << scan.GetOutputFileHeader() << endl
             << endl
             << endl;

        scan.PrintStartMessage();

        vector<int> listOfFiles;
        map<int, ifstream *> mapToFilePointers = scan.OpenInputFiles(scan.GetInputFileName(), scan.GetNumberOfFiles(),listOfFiles);

        TTree *RootTree = scan.InitOutput(scan.GetOutputFileName(), scan.GetOutputFileHeader(), scan.GetNumberOfFiles(),listOfFiles);

        bool dataLeftToProcess = true;
        long eventNumber = 0;

        while (dataLeftToProcess) {
            double curTime;
            // cout<<"While loop"<<endl;
            // (scan.GetDetectorMap()).erase(scan.GetDetectorMap().begin(),scan.GetDetectorMap().end());
            // scan.GetDetectorMap().clear();
            // cout<<"DetMap.size()= "<<scan.GetDetectorMap()->size()<<endl;
           
            for (auto itfile = mapToFilePointers.begin(); itfile != mapToFilePointers.end(); ++itfile) {
                ODeSA_Scan::LiquidCan temporaryDetector;
                if (itfile->second == NULL) {
                    // detVec.at(itfile->first) =
                    continue;
                } else {
                    int processingRetVal = scan.ProcessSingleEvent(itfile->second, Analysis, temporaryDetector, itfile->first, curTime, eventNumber, dataLeftToProcess);

                    if (processingRetVal == 0) {
                        if (scan.GetDetectorMap()->find(itfile->first) == scan.GetDetectorMap()->end()) {
                            cout << "ERROR:: Failed to find detecotor map key for updating. This is after the file processed" << endl;
                            break;
                        } else {
                            scan.GetDetectorMap()->find(itfile->first)->second = temporaryDetector;
                        }
                        // x.second = temporaryDetector;
                        // cout << "tempDetectorSruct.l= " << temporaryDetector.l << endl;
                        // ;
                        // cout << "detmap.find("<<itfile->first<<").l= " << scan.GetDetectorMap()->find(itfile->first)->second.l << endl;
                        
                        
                        // cout << "second detmap.find(" << itfile->first << ").l= " << scan.GetDetectorMap()->find(itfile->first)->second.l << endl;

                        // // cout<< endl<<" after set"<<endl;
                        // // cout<<"tempDetector.l= " << temporaryDetector.l <<endl;
                        // // cout<<"detmap.l= " << (scan.GetDetectorMap().at(itfile->first)).l << endl << endl;
                    }
                }
                
                if (itfile->first == 0) {
                    // Runtime clock
                    double difftime = curTime - scan.GetPreviousTime();
                    if (difftime < 0) {
                        difftime = difftime + 2147483647;
                        scan.SetRunTime(scan.GetRunTime() + ((8 * difftime) / 1.0E6));
                        scan.SetPreviousTime(curTime);
                    } else {
                        scan.SetRunTime(scan.GetRunTime() + ((8 * difftime) / 1.0E6));
                        scan.SetPreviousTime(curTime);
                    }
                }
            }
            // dataLeftToProcess=false;
            // cout<<"Prefill"<<endl;
            RootTree->Fill();
            ++eventNumber;
            // cout<<"PostFile"<<endl;
            if (eventNumber % 1000 == 0) {
                cout << "\rEvent counter: " << eventNumber << flush;
            }
        }
        cout<<endl<<"Scanning is finished. Check root for cfd= \"-99990\" || \"-99991\" which means the CFD failed for those events."<<endl;
        return 0;
    }
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
        {"verbose", optional_argument, NULL, 'v'},
        {"help", no_argument, NULL, '?'},
        {NULL, no_argument, NULL, 0}};

    if (argc != 0) {
        while ((retval = getopt_long(argc, argv, "n:bh:i:o:dv", longOpts, &idx)) != -1) {
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
                case 'v':
                    SetVerboseTerminalErrors(true);
                    break;
                case 'o':
                    if (optarg != 0) {
                        SetOutputFileName(optarg, ".root");
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

TTree* ODeSA_Scan::InitOutput(string outputFileName_, string outputFileHeader_, int numFiles_, vector <int> listOfFiles_) {
    
    // static Can d0,d1,d2,d3,d4,d5,d6,d7

    TFile *ff = new TFile(outputFileName_.c_str(), "RECREATE");
    TTree *tt = new TTree("T", outputFileHeader_.c_str());

    tt->Branch("runtime", &runtime, "Runtime (ms)");  // Runtime in ms

    for (unsigned i = 0; i < listOfFiles_.size(); ++i) {
        ODeSA_Scan::LiquidCan detector;
        string key = "d" + to_string(listOfFiles_.at(i));
        detMap.emplace( listOfFiles_.at(i), detector );
        tt->Branch(key.c_str(), &(detMap.at(listOfFiles_.at(i))), "l:s:amp:ampRaw:cfd:psd:pp/F:trig/O");
        if (GetDebugMode()){
            string traceKey_ = "trace" + to_string(listOfFiles_.at(i)) + "_d";
            string traceTitle_ = "Debugging Traces for Channel " + to_string(listOfFiles_.at(i));
            // TH1* tmpDebuggingTrace = new TH1F(traceKey_.c_str(),traceTitle_.c_mstr(),2000,0,1999);
            debug_Trace_map.emplace(listOfFiles_.at(i), new TH1F(traceKey_.c_str(),traceTitle_.c_str(),2000,0,1999)); 
            tt->Branch(traceKey_.c_str(), "TH1F", &debug_Trace_map.at(listOfFiles_.at(i)));
        }
    }
    return tt;
}

map<int,ifstream*> ODeSA_Scan::OpenInputFiles(string prefix_, int NumOfFiles_, vector<int> &listOfFiles_){
    string FullFileName;
    map<int, ifstream *> fileStreams;
   
    for( auto fileCounter = 0; fileCounter < NumOfFiles_; ++fileCounter){
        FullFileName = prefix_ + "_wave" + to_string(fileCounter) + ".dat";
        
        ifstream* FilePointer = new ifstream;
        FilePointer->open(FullFileName.c_str(), std::ifstream::in | std::ifstream::binary);
        if (FilePointer->is_open()){
            cout<< "Opening file: " << FullFileName << " -- Open!"<<endl;
            fileStreams.emplace(fileCounter,FilePointer);
            listOfFiles_.emplace_back(fileCounter);
        } else {
            cout<< "Opening file: " << FullFileName << " -- Failed!"<<endl;
        }
    }
    return fileStreams;
}

int ODeSA_Scan::ProcessSingleEvent(ifstream *file_, PulseAnalysis *Analysis_, LiquidCan &detector, int chanNum_, double &curTime, long &eventNum_,  bool &gotMoreData, bool doTraceAnalysis, pair<int, int> cfdRange_ , pair<int, int> psdRange_ ) {
    Float_t amplitude = 0, rawAmplitude=0;
    Float_t CFD_Phase = 0;
    int retVal = 0 ;

    bool trig = false;
   
    bool gotTraceData = true;

    int pposition = 0;
    int Tracelength;
    int eventlength;
    pair<double, double> psdValues = {0,0};

    // For SG filtered pulse
    Float_t trace_min, trace_max;
    int trace_min_loc, trace_max_loc;
    bool zero_cross;

    // const int preallocated_pulse_size = 2000;
    vector<float> pulse;
    vector<float> rawPulse;
    vector<float> CMAtrace;
    vector<float> SG_pulse;
    vector<float> SGderv2_pulse;
    vector<float> baseline;

    uint32_t buffer32;
    uint16_t buffer16;

    if (file_->is_open()) {
        trace_min = 0;
        if (!file_->read((char *)&buffer32, 4)) {
            gotMoreData = false;
            return 1;
        }
        Tracelength = (buffer32 - 16) / 2;
        file_->read((char *)&buffer32, 4);
        file_->read((char *)&buffer32, 4);
        file_->read((char *)&buffer32, 4);

        // Trigger time in 2 ADC clock cycles ( 8 ns )
        if (chanNum_ == 0) {
            curTime = buffer32;
        } else {
            curTime = -1;
        };
        // Get traces
        if (Tracelength == 0) {
            // no traces
            retVal = 2;
        } else {
            for (int i = 0; i < Tracelength; i++) {
                if (!file_->read((char *)&buffer16, 2)) {
                    gotTraceData = false;
                    break;
                }
                if (chanNum_ < 12) {
                    pulse.emplace_back( 16383 - (float)buffer16);
                } else {
                    pulse.emplace_back((float)buffer16);
                }

                if (pulse.at(i) > (16383 - threshold.at(chanNum_))) {
                    trig = true;
                }

                if (GetDebugMode()) {
                    debug_Trace_map.at(chanNum_)->Fill(i, pulse.at(i));
                }
                CMAtrace.emplace_back(0);
            }

            if (Tracelength > 1) {
                if (chanNum_ < 10) {
                    Analysis_->CMA_Filter(pulse.data(), Tracelength, CMAtrace.data(), 10, pulse.at(0), 3.5);
                } else {
                    // CMAtrace.fill(CMAtrace.begin(), CMAtrace.end(), 0)
                    std::fill(CMAtrace.begin(), CMAtrace.end(), 0);
                }

                for (int it = 0; it < pulse.size(); ++it) {
                    rawPulse.emplace_back(pulse.at(it));  // MUST BE DONE BEFORE Trace subtraction
                    pulse.at(it) -= CMAtrace.at(it);  // subtrace trace baseline from full trace
                }

                //! Find Peak Position and Amplitude from the subtraced trace.
                if (chanMap.find(chanNum_)->second == "RF" || chanMap.find(chanNum_)->second == "SCATTERING") {  //RF and Scattering detecotrs for LANL2021
                    //! for these channels I'm excluding the first and last 200ns ( 100 ticks @ 500MHz)
                    pair<unsigned int, unsigned int> blocking_range = {100, 100};
                    // vector<float>::iterator beginTest = pulse.begin();
                    // std::advance(beginTest, 100);
                    // vector<float>::iterator endTest = pulse.end();
                    // std::advance(endTest, -100);
                    // vector<float>::iterator beginTestRaw = rawPulse.begin();
                    // std::advance(beginTestRaw, 100);
                    // vector<float>::iterator endTestRaw = rawPulse.end();
                    // std::advance(endTestRaw, -100);

                    // pposition = std::max_element(beginTest , endTest) - pulse.begin();
                    // amplitude = *std::max_element(beginTest,endTest);
                    // rawAmplitude = *std::max_element(beginTestRaw,endTestRaw);

                    pposition = std::max_element(pulse.begin()+blocking_range.first, pulse.end()- blocking_range.second) - pulse.begin();
                    amplitude = *std::max_element(pulse.begin()+blocking_range.first, pulse.end()- blocking_range.second);
                    rawAmplitude = *std::max_element(rawPulse.begin()+blocking_range.first, rawPulse.end()- blocking_range.second);

                } else {
                    pposition = std::max_element(pulse.begin(), pulse.end()) - pulse.begin();
                    amplitude = *std::max_element(pulse.begin(), pulse.end());
                    rawAmplitude = *std::max_element(rawPulse.begin(), rawPulse.end());
                }

                //! Begin Timing Section
                float CFD_Phase;
                if (pposition > 10 && pposition < Tracelength - 10) {
                    if (chanMap.find(chanNum_)->second == "ODESA") {
                        CFD_Phase = ODeSA_Cfd(cfdRange_, pulse, pposition, amplitude);
                    } else {
                        CFD_Phase = Extras_Cfd(cfdRange_, pulse, pposition, amplitude);
                    }
                } else {
                    CFD_Phase = -99991;
                }
                if (GetVerboseTerminalErrors()) {
                    if (CFD_Phase == -99990) {
                        cout << endl
                             << "ERROR:: CFD for channel " << chanNum_ << " in event number " << eventNum_ << " failed due to a divide by zero" << endl;
                        // && pposition > 10 && chanNum_ != 1
                    } else if (CFD_Phase == -99991) {
                        cout << endl
                             << "ERROR:: CFD for channel " << chanNum_ << " in event number " << eventNum_ << " failed due due to bad peak position. Position / Trace Length = " << pposition << " / " << Tracelength << endl;
                    }
                }

                //! PSD Stuff
                if (pposition - psdRange_.first > 0 && pposition + psdRange_.second < Tracelength) {
                    if (chanMap.find(chanNum_)->second == "ODeSA") {
                        psdValues = ODeSA_PSD(psdRange_, pulse, pposition);
                    } else {
                        psdValues = Extras_PSD(psdRange_, pulse, pposition);
                    }
                }
            }
        }
        if (retVal == 0) {
            pair<float,float> chanCal = calibrations.find(chanNum_)->second;
            //fill detstruct
            detector.s = ( chanCal.second * psdValues.first ) + (chanCal.first);
            detector.l = (chanCal.second * psdValues.second) + (chanCal.first);
            detector.psd = psdValues.first / psdValues.second;
            detector.amp = amplitude;
            detector.ampRaw = rawAmplitude;
            detector.cfd = CFD_Phase;
            detector.pp = pposition;
            detector.trig = trig;
        }
        return retVal;
    } else {
        return -1;  //if file not open
    }
}

int ODeSA_Scan::ODeSA_Cfd(pair<int, int> cfdRange_, vector<float> trace_, int &peakPosition_, float &amplitude ) {
    int leadingEdge;
    float xy_ = 0, x_ = 0, y_ = 0, x_2 = 0, m_ = 0, b_ = 0, counter = 0;
    for (unsigned it = peakPosition_ - 10; it <= peakPosition_; ++it) {
        if (trace_.at(it) <= 0.5 * amplitude) {
            leadingEdge = it;
        }
    }
   
    for (unsigned it = leadingEdge - cfdRange_.first; it <= leadingEdge + cfdRange_.second; ++it) {
        ++counter;  // we need to know later how many points we looped over
        x_ += it;
        x_2 += it * it;
        y_ += trace_.at(it);
        xy_ += trace_.at(it) * it;
    }
    m_ = ((counter * xy_) - (x_ * y_)) / ((counter * x_2) - (x_ * x_));
    b_ = ((y_ * x_2) - (x_ * xy_)) / ((counter * x_2) - (x_ * x_));

    if (m_ != 0 ){
    return (((0.5 * amplitude) - b_) / m_);
    } else {
        return -99990;
    }

};  // end Func

int ODeSA_Scan::Extras_Cfd(pair<int, int> cfdRange_, vector<float> trace_, int &peakPosition_, float &amplitude ) {
    int leadingEdge;
    double xy_ = 0, x_ = 0, y_ = 0, x_2 = 0, m_ = 0, b_ = 0, counter = 0;
    for (unsigned it = peakPosition_ - 10; it<= peakPosition_; ++it) {
        if (trace_.at(it) <= 0.5 * amplitude) {
            leadingEdge = it;
        }
    }

    
    for (unsigned it =  leadingEdge - cfdRange_.first; it <= leadingEdge + cfdRange_.second; ++it) {
        ++counter; // we need to know later how many points we looped over
        x_ += it;
        x_2 += it * it;
        y_ += trace_.at(it);
        xy_ += trace_.at(it) * it;
    }
    m_ = ((counter * xy_) - (x_ * y_)) / ((counter * x_2) - (x_ * x_));
    b_ = ((y_ * x_2) - (x_ * xy_)) / ((counter * x_2) - (x_ * x_));

    if (m_ == (double)0.0) {
        return -99990;
    } else {
        return (((0.5 * amplitude) - b_) / m_);
        
    }
}

//! <short, Long>
pair<double, double> ODeSA_Scan::ODeSA_PSD(pair<int, int> psdRange_, vector<float> trace_, int peakPosition_, int offset) {
    // PSD integration
    double paraS = 0, paraL = 0;
    for (unsigned it = (peakPosition_ - psdRange_.first); it < (peakPosition_ + psdRange_.second); ++it) {
        paraL += trace_.at(it );
        if (it > peakPosition_ + offset) {
            paraS += trace_.at(it);
        }
    }
return make_pair(paraS,paraL);
};

//! <Short, Long>
pair<double, double> ODeSA_Scan::Extras_PSD(pair<int, int> psdRange_, vector<float> trace_, int peakPosition_, int offset) {
    // PSD integration
    double paraS = 0, paraL = 0;
    for (unsigned it = (peakPosition_ - psdRange_.first); it < (peakPosition_ + psdRange_.second); ++it) {
        paraL += trace_.at(it );
        if (it > peakPosition_ + offset) {
            paraS += trace_.at(it );
        }
    }
    return make_pair(paraS,paraL);
};


