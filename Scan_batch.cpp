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


#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include "PulseAnalysis.h"
#include "TTree.h"
#include "TFile.h"
#include "TH1.h"
#include "TMath.h"
#include "TSpectrum.h"
#include "TF1.h"

using namespace std;

typedef struct
{
  Float_t l;               // Long integral
  Float_t s;               // Short integral
  Float_t amp;             // Amplitude
  Float_t cfd;             // Trigger time
  Float_t psd;             // PSD parameter s/l
  Float_t trg;             // Detector trigger

} Can;

int Scan (char filename[250], char prefix[250]){


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

  float pulse[300],
    CMAtrace[300],
    SG_pulse[300],
    SGderv2_pulse[300],
    baseline[300];

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

  char 	prompt[10],
    openfile[250],
    runnum[250],
    interrputPrompt;

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
    {   0.0359,
      	0.0327,
      	0.0362,
      	0.0269,
      	0.0327,
      	0.0345,
      	0.0394,
      	0.0376,
      	0.031,
      	0.0236,
      	1.0, 1., 1., 1., 1., 1.
    }; // calibration (keVee / (bit * sample)) from manual check on calibration

    float caloffset[16] =
      {   21.124,
  	      23.803,
  	      18.688,
  	      18.465,
  	      11.81,
    	    23.183,
        	22.302,
        	18.863,
        	13.401,
        	30.319,
    	    0, 0, 0, 0, 0, 0
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
  cout << " |   Experiment: 18O(a,n)                        |" << endl;
  cout << " |   Date: December 2019                         |" << endl;
  cout << " |   Calibration used: Detector_Parameters.xslx  |" << endl;
  cout << " |   ORNL Nuclear Astrophysics                   |" << endl;
  cout << " ------------------------------------------------ " << endl;

  // cout << "Root file name to be created: ";
  // cin >> filename;

  // cout << "Root file header: ";
  // cin >> fileheader;

  // cout << "Run binary file prefix ('../run#'): ";
  // cin >> prefix;

  TFile *ff = new TFile(filename, "RECREATE");

  TTree *tt = new TTree("T", fileheader.c_str());

  tt->Branch("d0",&det0,"l:s:amp:cfd:psd:trg");
  tt->Branch("d1",&det1,"l:s:amp:cfd:psd:trg");
  tt->Branch("d2",&det2,"l:s:amp:cfd:psd:trg");
  tt->Branch("d3",&det3,"l:s:amp:cfd:psd:trg");
  tt->Branch("d4",&det4,"l:s:amp:cfd:psd:trg");
  tt->Branch("d5",&det5,"l:s:amp:cfd:psd:trg");
  tt->Branch("d6",&det6,"l:s:amp:cfd:psd:trg");
  tt->Branch("d7",&det7,"l:s:amp:cfd:psd:trg");
  tt->Branch("d8",&det8,"l:s:amp:cfd:psd:trg");
  tt->Branch("d9",&det9,"l:s:amp:cfd:psd:trg");
  tt->Branch("d10",&det10,"l:s:amp:cfd:psd:trg");
  tt->Branch("runtime",&runtime,"Runtime (ms)");     // Runtime in ms
  tt->Branch("X",&steerer_X,"X steerer");
  tt->Branch("Y",&steerer_Y,"Y steerer");

  TH1F *trace0 = new TH1F("trace0","Trace for channel 0",200,0,199);
  //tt->Branch("trace0","TH1F", &trace0);
  TH1F *trace1 = new TH1F("trace1","Trace for channel 1",200,0,199);
  //tt->Branch("trace1","TH1F", &trace1);

  TH1F *traceCMA = new TH1F("traceCMA","Trace for CMA",200,0,199);
  //tt->Branch("traceCMA","TH1F", &traceCMA);

  TH1F *trace0C = new TH1F("trace0C","Corrected trace for channel 0",200,0,199);
  //tt->Branch("trace0C","TH1F", &trace0C);

  int numSteps = 20;
  float psd_opt[20];
  //tt->Branch("psd_opt", &psd_opt, "psd_opt[20]/F");

  const int numFiles = 14;

  // Open files
  for (i = 0; i < numFiles; i++)
  {
    sprintf(openfile, "%s_wave%d.dat",prefix,i);
    cout << " Opening file: " << openfile;
    fp[i].open(openfile, std::ifstream::in | std::ifstream::binary);

    if(fp[i].is_open()) {cout << " - Open!" << endl;}
    else { {cout << " - Failed!" << endl;} }
  }

  // Break condition for junk runs
  if (fp[0].is_open()==0){
    cout << "break" << endl;
    return 0;
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
        fp[j].read((char*)&buffer32, 4);
        fp[j].read((char*)&buffer32, 4);
        fp[j].read((char*)&buffer32, 4);

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

		        // Added traces
		        if (j==0) {trace0->SetBinContent(i, pulse[i]);}
		        if (j==1) {trace1->SetBinContent(i, pulse[i]);}
		    }

	      /** Liquid can processing **/
	      if(Tracelength > 1)
		    {

          // Process trace
          if (j < 10) {
            Analysis->CMA_Filter(pulse, Tracelength, CMAtrace, 10, pulse[0], 3.5 );
            for (i = 0; i < Tracelength; i++) {
              pulse[i] -= CMAtrace[i];
              if (j==0) {trace0->SetBinContent(i, pulse[i]);}
              if (j==1) {trace1->SetBinContent(i, pulse[i]);}
              trace0C->SetBinContent(i, pulse[i]);
              traceCMA->SetBinContent(i, CMAtrace[i]);
              if (pulse[i] > amplitude) {amplitude = pulse[i]; pposition = i;}
          }

          // CFD timing
          //f1->SetParameters(1.0, (double)pposition, 0.1);
          //trace0->GetXaxis()->SetRangeUser(pposition - 5, pposition + 1);
          //trace0->Fit("f1","RQ");
          //mu = (float)f1->GetParameter(1);
          //sigma = (float)f1->GetParameter(2);

          //CFD = mu - sqrtf(1.38629*sigma*sigma);
	        CFD = (float)pposition;

          // PSD integration
          offset = 12.0; // original 12
          if (pposition - 10 > 0 && pposition + 100 < Tracelength) {
            for (i = (pposition - 10); i < (pposition + 100); i++) {
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
  tt->Write();
  ff->Close();

  cout << "\nFinished! " << TEvt << " events" << endl;

  return 0;
}

int main() {

  int i;
  char file[250], pref[250], runno[50];

  // For loop to run over relevant run numbers - edit
  for (i = 205; i<784;i++){

    sprintf(runno,"%i",i);
    sprintf(file,"RootFiles/R%s.root",runno);
    sprintf(pref,"../18Oan/run%s",runno);

    cout << pref << endl;
    Scan(file, pref);

  }
  return 0;
  //return Scan();
}