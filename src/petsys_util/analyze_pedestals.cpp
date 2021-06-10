#include<iostream>

#include "TFile.h"
#include "TTree.h"
#include "TProfile.h"


int main(int argc, char** argv)
{
  if( argc < 4 )
  {
    std::cout << "analyze_pedstals.exe::usage:   ./analyize_pedestals.exe inFile_1.root inFile_2.root outFile.root" << std::endl;
    return(1);
  }
  
  
  
  std::vector<TFile*> inFiles;
  inFiles.push_back( TFile::Open(argv[1]) );
  inFiles.push_back( TFile::Open(argv[2]) );
  
  
  TFile* outFile = TFile::Open(argv[3],"RECREATE");
  outFile -> cd();
  
  std::map<int,std::map<int,TProfile*> > p_qfine;
  int nTACs = 8;
  
  
  
  int ii = 0;
  for(auto inFile : inFiles)
  {
    inFile -> cd();
    TTree* data = (TTree*)( inFile->Get("data") );
    
    unsigned int channelID;
    unsigned short int tacID;
    unsigned short qfine;
    
    data -> SetBranchStatus("*",0);
    data -> SetBranchStatus("channelID",1); data -> SetBranchAddress("channelID",&channelID);
    data -> SetBranchStatus("tacID",1);     data -> SetBranchAddress("tacID",    &tacID);
    data -> SetBranchStatus("qfine",1);     data -> SetBranchAddress("qfine",    &qfine);
    
    int nEntries = data -> GetEntries();
    for(int entry = 0; entry < nEntries; ++entry)
    {
      data -> GetEntry(entry);
      if( entry%100000 == 0) std::cout << ">>> reading entry " << entry << " / " << nEntries << std::endl;
      
      if( !(p_qfine[0])[channelID] )
      {
        for(int ii = 0; ii < 3; ++ii)
        {
          outFile -> cd();
          (p_qfine[ii])[channelID] = new TProfile(Form("p_qfine_ch%d_%d",channelID,ii),"",nTACs,-0.5,nTACs-0.5,0.,1024.,"s");
        }
      }
      
      (p_qfine[ii])[channelID] -> Fill(tacID,qfine);
      (p_qfine[2])[channelID] -> Fill(tacID,qfine);
    }
    
    ++ii;
  }
  
  
  
  int bytes = outFile -> Write();
  std::cout << "============================================"  << std::endl;
  std::cout << "nr of  B written:  " << int(bytes)             << std::endl;
  std::cout << "nr of KB written:  " << int(bytes/1024.)       << std::endl;
  std::cout << "nr of MB written:  " << int(bytes/1024./1024.) << std::endl;
  std::cout << "============================================"  << std::endl;
}
