/************************************************************************
 *
 *  Filename: Scan.cpp
 *s
 *  Description:
 *
 *	Author(s):
 *     Michael T. Febbraro
 *     David Walter
 *
 *  Creation Date: 9/25/2016
 *  Last modified: 8/24/2018
 *
 *  To compile: g++ -O3 -pedantic -o Scan.exe `root-config --cflags --libs` -lSpectrum NewScan.cpp
 *      - if errors copiling in Mac OSX
 *        - remove -03 option
 *        - clang++ instead of g++
 *        - $(root-config --cflags --libs) instead of 'root-config --cflags --libs'
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

#include <getopt.h>
#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include "PulseAnalysis.h"
#include "TTree.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TMath.h"
#include "TSpectrum.h"
#include "TF1.h"

#include <chrono>
#include <ctime>

using namespace std;

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


typedef struct
{
  Float_t l;               // Long integral
  Float_t s;               // Short integral
  Float_t amp;             // Amplitude
  Float_t cfd;             // Trigger time
  Float_t psd;             // PSD parameter s/l
  Float_t trg;             // Detector trigger

} Can;

int Scan (int NumFiles_, bool BatchMode_, bool DebugMode_, string OutFile_, string InFile_, string HeaderString_ ){


  /** ----------------------------------------------------
   *	Variable declaration
   *   ----------------------------------------------------
   */
  static Can det0, det1, det2, det3, det4, det5, det6, det7, det8, det9, det10, det11, det12;

  bool	beamON,
        trg,
        data;

  float	X, offset, sigma, mu;
  int	multi;

  ifstream fp[16];

  string 	line, fileheader;

  int i,j,k, pposition,
      Tracelength,
      eventlength;

  const int preset_pulse_length = 2000;

  float pulse[preset_pulse_length],
  CMAtrace[preset_pulse_length],
  SG_pulse[preset_pulse_length],
  SGderv2_pulse[preset_pulse_length],
  baseline[preset_pulse_length];

  Float_t amplitude,
          risetime,
          falltime,
          width,
          CFD,
          tac,
          paraL,
          paraS,
          runtime,
          steerer_X, steerer_Y,
          temp;

  // For SG filtered pulse
  Float_t trace_min, trace_max;
  int trace_min_loc, trace_max_loc;
  bool zero_cross;

  char prompt[10],
  //openfile[250],
  //prefix[250],
  runnum[250],
  interrputPrompt;

  string filename, prefix, openfile;


  //! Original
  /* char 	filename[250], */
  /*   prompt[10], */
  /*   openfile[250], */
  /*   prefix[250], */
  /*   runnum[250], */
  /*   interrputPrompt; */

  Float_t trgtime, prevtime, difftime;
  Float_t prevtrgtime[10];
  long	TEvt = 0;

  uint32_t buffer32;
  uint16_t buffer16;

  TSpectrum *s = new TSpectrum();

  TF1 *f1 = new TF1("f1","gaus",0,300);

  /** ----------------------------------------------------
   *	Calibrations and threshold
   *   ----------------------------------------------------
   */

  float cal[16] =
  {  1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1

  }; // calibration (keVee / (bit * sample)) from manual check on calibration

  float caloffset[16] =
  {   0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };

  float threshold[16] =
  {   15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700,
    15700
  };

  /** ----------------------------------------------------
   *	Get functions
   *   ----------------------------------------------------
   */

  PulseAnalysis *Analysis = new PulseAnalysis();


  /** ----------------------------------------------------
   *	Program start...
   *   ----------------------------------------------------
   */

  cout << " ------------------------------------------------ " << endl;
  cout << " | Scan.cpp - binary version                     |" << endl;
  cout << " |   Experiment: LANL 2021                       |" << endl;
  cout << " |   Date: December 2019                         |" << endl;
  cout << " |   Calibration used: None                      |" << endl;
  cout << " |   ORNL Nuclear Astrophysics                   |" << endl;
  cout << " ------------------------------------------------ " << endl;
 
  auto start = std::chrono::system_clock::now();
  //cout << " \n\n Current Time is " << std::ctime(start) << endl;

  //! Batch Mode switch. TTK Oct 26 2021
  if (!BatchMode_) {
    if (OutFile_ == " ") {
      cout << "We need the Root file prefix to be created (no extension): ";
      cin >> filename;
      filename += ".root";
    } else {
      cout << "Using commandline flag for the ROOT file prefix" << endl;
      filename = OutFile_ + ".root";
    }

    if (HeaderString_ == " "){
      cout << "Root file header: ";
      cin >> fileheader;
    }else {
      cout << "Using commandline flag for the ROOT file header" << endl;
      fileheader = HeaderString_;
    }

    if (InFile_ == " ") {
      cout << "Run binary file prefix ('../run#'): ";
      cin >> prefix;
    } else {
      cout << "Using commandline flag for the CAEN based input file prefix" << endl;
      prefix = InFile_;
    }
  } else {
    filename = OutFile_ + ".root";
    fileheader = HeaderString_;
    prefix = InFile_;
  } // end if batchmode

  //const int numFiles = 14;
  const int numFiles = NumFiles_;

  TFile *ff = new TFile(filename.c_str(), "RECREATE");

  TTree *tt = new TTree("T", fileheader.c_str());

  tt->Branch("d0",&det0,"l:s:amp:cfd:psd:trg");
  tt->Branch("d1",&det1,"l:s:amp:cfd:psd:trg");
  tt->Branch("d2",&det2,"l:s:amp:cfd:psd:trg");
  tt->Branch("d3",&det3,"l:s:amp:cfd:psd:trg");
  tt->Branch("d4",&det4,"l:s:amp:cfd:psd:trg");
  tt->Branch("d5",&det5,"l:s:amp:cfd:psd:trg");
  tt->Branch("d6",&det6,"l:s:amp:cfd:psd:trg");
  tt->Branch("d7",&det7,"l:s:amp:cfd:psd:trg");
  tt->Branch("runtime",&runtime,"Runtime (ms)");     // Runtime in ms


  //TH1F *traceCMA = new TH1F("traceCMA","Trace for CMA",200,0,199);
  //tt->Branch("traceCMA","TH1F", &traceCMA);

  //TH1F *trace0C = new TH1F("trace0C","Corrected trace for channel 0",200,0,199);
  //tt->Branch("trace0C","TH1F", &trace0C);

  int numSteps = 20;
  float psd_opt[20];
  //tt->Branch("psd_opt", &psd_opt, "psd_opt[20]/F");

  vector<TH2F*> trace_vector;
  vector<TH1*> debug_Trace_vector ;
  for( i= 0; i < numFiles; ++i){
    string key = "trace" + to_string(i) ;
    string title = "Traces for Channel " + to_string(i);
    trace_vector.emplace_back(new TH2F(key.c_str(),title.c_str(),2000,0,1999,16400,0,16400));
    if (DebugMode_) { 
      key += "_d";
      title += " Single Event";
      //TH1F *trace7 = new TH1F("trace7","Trace for channel 7",200,0,199);
      debug_Trace_vector.emplace_back( new TH1F(key.c_str(),title.c_str() ,2000,0,1999));
      tt->Branch(key.c_str(),"TH1F", debug_Trace_vector.at(i)) ; 
    }
  }

  // Open files
  for (i = 0; i < numFiles; i++)
  {
    //sprintf(openfile, "%s_wave%d.dat",prefix.c_str(),i);
    openfile = InFile_ + "_wave" + to_string(i) + ".dat"; 
    cout << " Opening file: " << openfile.c_str();
    fp[i].open(openfile, std::ifstream::in | std::ifstream::binary);

    if(fp[i].is_open()) {cout << " - Open!" << endl;}
    else {{cout << " - Failed!" << endl;} }
  }

  data = 1;
  runtime = 0;
  prevtime = 0;

  /** ----------------------------------------------------
   *	Process liquid scintillator det events
   *   ----------------------------------------------------
   */

  while (data)
  {
    multi = 0;
    X = -1;
    beamON = 0;
    for (j = 0; j < numFiles; j++)
    {
      if(j > -1)
      {
        //if(!fp[j].is_open()){data = 0; cout << "Could not open file!!" << endl;}

        // Stop after nth events...
        //if (TEvt > 1000000) {data = 0;}

        if(fp[j].is_open())
        {

          trace_min = 0;

          // Binary parsing
          if (!fp[j].read((char*)&buffer32, 4)) {data = 0; break;}
          Tracelength = (buffer32 - 16)/2;
          if( Tracelength > preset_pulse_length ){
            cout << "ERROR!!!!!! TRACE_LENGTH IS LONGER THAN PRE-ALLOCATED PULSE_ARRAY_SIZE.\nTHIS WILL SEGFAULT UNLESS FIXED IN CODE ABOVE HERE." << endl;
            break;
          }
          fp[j].read((char*)&buffer32, 4);
          fp[j].read((char*)&buffer32, 4);
          fp[j].read((char*)&buffer32, 4);
          //cout << Tracelength << endl;
          // Trigger time in 2 ADC clock cycles ( 8 ns )
          if (j == 0)
            trgtime = buffer32;

          // Reset variables
          CFD = -1;
          amplitude = -1;
          paraL = 0;
          paraS = 0;
          trg = 0;
          tac = 0;
          pposition = -1;
          //steerer_X = 0;
          //steerer_Y = 1;
          temp = 0;

          // Get traces
          for (i = 0; i < Tracelength; i++)
          {
            if (!fp[j].read((char*)&buffer16, 2)) {data = 0; break;}
            if (j < 12) {pulse[i] = 16383 - (float)buffer16; }
            else {pulse[i] = (float)buffer16;}

            if (pulse[i] > (16383 - threshold[j])) {trg = 1;}

            if (DebugMode_){
              debug_Trace_vector.at(j)->SetBinContent(i, pulse[i]);
            }


            // Added traces
            //if (j==7) {trace7->SetBinContent(i, pulse[i]);}
            //if (j==1) {trace1->SetBinContent(i, pulse[i]);}
          }

          /** Liquid can processing **/
          if(Tracelength > 1)
          {

            // Process trace
            if (j < 10) {
              Analysis->CMA_Filter(pulse, Tracelength, CMAtrace, 10, pulse[0], 3.5 );
              for (i = 0; i < Tracelength; i++) {
                pulse[i] -= CMAtrace[i];
                //if (j==7) {trace7->SetBinContent(i, pulse[i]);}
                //if (j==1) {trace1->SetBinContent(i, pulse[i]);}
                //trace0C->SetBinContent(i, pulse[i]);
                //traceCMA->SetBinContent(i, CMAtrace[i]);

                if (pulse[i] > amplitude) {
                  amplitude = pulse[i]; 
                  pposition = i;}
              }

              // CFD timing
              //f1->SetParameters(1.0, (double)pposition, 0.1);
              //trace0->GetXaxis()->SetRangeUser(pposition - 5, pposition + 1);
              //trace0->Fit("f1","RQ");
              //mu = (float)f1->GetParameter(1);
              //sigma = (float)f1->GetParameter(2);

              //CFD = mu - sqrtf(1.38629*sigma*sigma);
              

              // ----------------------------------------
              // CFD timing from peak position (tick dependent)
              // ----------------------------------------
              //CFD = (float)pposition;
              

              // ----------------------------------------
              // CFD timing using a linear regression
              // ----------------------------------------
              if (pposition > 10 && pposition < Tracelength - 10)
              {
                float xy_, x_, y_, x_2, m_, b_;
                int leadingEdge;

                for (i = pposition - 10; i <= pposition; i++)
                {
                  if (pulse[i] <= 0.5*amplitude) { leadingEdge = i;}
                }
                xy_ = 0;
                x_ = 0;
                y_ = 0;
                x_2 = 0;
                if (j==1){
                  for(i = leadingEdge - 1; i <= leadingEdge + 1; i++) {
                    x_ += i;
                    x_2 += i * i;
                    y_ += pulse[i];
                    xy_ += pulse[i] * i;
                  }
                  m_ = ((3.*xy_) - (x_*y_))/((3.0*x_2) - (x_*x_));
                  b_ = ((y_*x_2) - (x_*xy_))/((3.0*x_2) - (x_*x_));
                } else {
                  for(i = leadingEdge - 1; i <= leadingEdge + 1; i++) {
                    x_ += i;
                    x_2 += i * i;
                    y_ += pulse[i];
                    xy_ += pulse[i] * i;
                  }
                  m_ = ((3.*xy_) - (x_*y_))/((3.0*x_2) - (x_*x_));
                  b_ = ((y_*x_2) - (x_*xy_))/((3.0*x_2) - (x_*x_));

                  CFD = ((0.5 * amplitude) - b_) / m_;
                }
              }
              else {CFD = -1;}

              // PSD integration
              offset = 24;
              if (pposition - 20 > 0 && pposition + 200 < Tracelength) {
                for (i = (pposition - 20); i < (pposition + 200); i++) {
                  paraL += pulse[i];
                  if (i > pposition + offset) { paraS += pulse[i];}
                }
              }
            }
          }
        }


        switch(j) {
          case 0 :
            det0.amp = amplitude;
            det0.l = (paraL*cal[j]) + caloffset[j];
            det0.s = (paraS*cal[j]) + caloffset[j];
            det0.cfd = CFD;
            det0.trg = trg;
            det0.psd = paraS / paraL;
            if (det0.trg){ det0.trg = 1;}
            else {det0.trg = 0;}

            // Runtime clock
            difftime = trgtime - prevtime;
            if(difftime < 0) {
              difftime = difftime + 2147483647;
              runtime += ((8*difftime)/1.0E6);
              prevtime = trgtime;
            }
            else {
              runtime += ((8*difftime)/1.0E6);
              prevtime = trgtime;
            }

            break;

          case 1 :


            det1.amp = amplitude;
            det1.l = (paraL*cal[j]) + caloffset[j];
            det1.s = (paraS*cal[j]) + caloffset[j];
            det1.cfd = CFD;
            det1.trg = trg;
            det1.psd = paraS / paraL;
            if (det1.trg){ det1.trg = 1;}
            else {det1.trg = 0;}


            break;

          case 2 :

            det2.amp = amplitude;
            det2.l = (paraL*cal[j]) + caloffset[j];
            det2.s = (paraS*cal[j]) + caloffset[j];
            det2.cfd = CFD;
            det2.trg = trg;
            det2.psd = paraS / paraL;
            if (det2.trg){ det2.trg = 1;}
            else {det2.trg = 0;}

            break;

          case 3 :
            det3.amp = amplitude;
            det3.l = (paraL*cal[j]) + caloffset[j];
            det3.s = (paraS*cal[j]) + caloffset[j];
            det3.cfd = CFD;
            det3.trg = trg;
            det3.psd = paraS / paraL;
            if (det3.trg){ det3.trg = 1;}
            else {det3.trg = 0;}

            break;


          case 4 :
            det4.amp = amplitude;
            det4.l = (paraL*cal[j]) + caloffset[j];
            det4.s = (paraS*cal[j]) + caloffset[j];
            det4.cfd = CFD;
            det4.trg = trg;
            det4.psd = paraS / paraL;
            if (det4.trg){ det4.trg = 1;}
            else {det4.trg = 0;}

            break;


          case 5 :
            det5.amp = amplitude;
            det5.l = (paraL*cal[j]) + caloffset[j];
            det5.s = (paraS*cal[j]) + caloffset[j];
            det5.cfd = CFD;
            det5.trg = trg;
            det5.psd = paraS / paraL;
            if (det5.trg){ det5.trg = 1;}
            else {det5.trg = 0;}

            break;

          case 6 :
            det6.amp = amplitude;
            det6.l = (paraL*cal[j]) + caloffset[j];
            det6.s = (paraS*cal[j]) + caloffset[j];
            det6.cfd = CFD;
            det6.trg = trg;
            det6.psd = paraS / paraL;
            if (det6.trg){ det6.trg = 1;}
            else {det6.trg = 0;}

            break;

          case 7 :
            det7.amp = amplitude;
            det7.l = (paraL*cal[j]) + caloffset[j];
            det7.s = (paraS*cal[j]) + caloffset[j];
            det7.cfd = CFD;
            det7.trg = trg;
            det7.psd = paraS / paraL;
            if (det7.trg){ det7.trg = 1;}
            else {det7.trg = 0;}

            break;

          case 8 :
            det8.amp = amplitude;
            det8.l = (paraL*cal[j]) + caloffset[j];
            det8.s = (paraS*cal[j]) + caloffset[j];
            det8.cfd = CFD;
            det8.trg = trg;
            det8.psd = paraS / paraL;
            if (det8.trg){ det8.trg = 1;}
            else {det8.trg = 0;}

            break;

          case 9 :
            det9.amp = amplitude;
            det9.l = (paraL*cal[j]) + caloffset[j];
            det9.s = (paraS*cal[j]) + caloffset[j];
            det9.cfd = CFD;
            det9.trg = trg;
            det9.psd = paraS / paraL;
            if (det9.trg){ det9.trg = 1;}
            else {det9.trg = 0;}

            break;

          case 10 :
            det10.amp = amplitude;
            det10.l = (paraL*cal[j]) + caloffset[j];
            det10.s = (paraS*cal[j]) + caloffset[j];
            det10.cfd = CFD;
            det10.trg = trg;
            det10.psd = paraS / paraL;
            if (det10.trg){ det10.trg = 1;}
            else {det10.trg = 0;}

            break;

          case 11 :
            det11.amp = amplitude;
            det11.l = paraL;
            det11.s = paraS;
            det11.cfd = CFD;
            det11.trg = trg;
            det11.psd = det11.s / det11.l;
            if (det11.trg){ det11.trg = 1;}
            else {det11.trg = 0;}

            break;

          case 12 :
            steerer_X = pulse[0];
            break;

          case 13 :
            steerer_Y = pulse[0];
            break;

          case 14 :
            break;
        }

      }
    }
    tt->Fill();
    TEvt++;
    if (TEvt%1000==0) {cout << "\rEvent counter: " << TEvt << flush;}

  }

  for (i = 0; i < numFiles; i++)
  {
    if(fp[j].is_open()) fp[j].close();
  }
  ff->cd();
  tt->Write("",TObject::kWriteDelete);
  ff->Close();

  cout << "\nFinished! " << TEvt << " events" << endl;

  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::time_t end_time = std::chrono::system_clock::to_time_t(end);
  //cout << "Finished at " << std::ctime(end) << "\nScan took " << elapsed_seconds.count() << " seconds."<<endl;

  return 0;
}

int main(int argc, char *argv[]) {

  int idx = 0, retval = 0, numberOfFiles_ = 0 ;
  string outName_ = " " , inName_ = " ", header_ = " ";
  bool batchMode_ = false, foundNumFiles_ = false, debugMode_ = false;

  if ( argc != 0 ) {

    struct option longOpts[] = {
      {"numChans",      required_argument, NULL, 'n'},
      {"batch",         optional_argument, NULL, 'b'},
      {"output",        required_argument, NULL, 'o'},
      {"header",        required_argument, NULL, 'h'},
      {"input",         required_argument, NULL, 'o'},
      {"debug",         no_argument,       NULL, 'd'},
      {"help",          no_argument,       NULL, '?'},
      {NULL,            no_argument,       NULL,   0}
    };


    while (( retval = getopt_long(argc, argv, "n:bh:i:o:d" , longOpts, &idx )) != -1 ){
      switch(retval) {
        case 'n': 
          if (optarg != 0 ){
            numberOfFiles_ = atoi(optarg);
            foundNumFiles_ = true;
          } else {
            cout << "ERROR:: Missing Number of channels (files) to merge and build." << endl;  
            return 1;
          }
          break;
        case 'b':
          batchMode_ = true;
          break;
        case 'd':
          debugMode_ = true;
          break;
        case 'o':
          if (optarg != 0 ){
            outName_ = optarg;
          } else {
            cout << "ERROR:: Missing Output root file name." << endl;
            return 2;
          }
          break;
        case 'h':
          if (optarg != 0 ){
            header_ = optarg;
          } else {
            cout << "ERROR:: Missing header string." << endl;
            return 3;
          }
          break;
        case 'i':
          if (optarg != 0 ){
            inName_ = optarg;
          } else {
            cout << "ERROR:: Missing Input file name" << endl;
            return 4;
          }
          break;
        case '?':
          ScanHelp();
          return 5;
        default:
          cout << "ERROR:: Unknown comandline argument" <<endl;
          return 6;
      }//end switch 
    }//end while
    if (!foundNumFiles_) {
      cout << "-n is mandatory" << endl;
      return 7;
    }
  } 

  return Scan(numberOfFiles_, batchMode_, debugMode_, outName_, inName_, header_ );
}
