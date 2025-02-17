#include <RawReader.hpp>
#include <OverlappedEventHandler.hpp>
#include <getopt.h>
#include <assert.h>
#include <SystemConfig.hpp>
#include <CoarseSorter.hpp>
#include <ProcessHit.hpp>
#include <SimpleGrouper.hpp>

#include <boost/lexical_cast.hpp>

#include <TFile.h>
#include <TNtuple.h>
#include <TProfile.h>

#include <iostream>

using namespace PETSYS;


enum FILE_TYPE { FILE_TEXT, FILE_BINARY, FILE_ROOT };

class DataFileWriter {
private:
	double frequency;
	FILE_TYPE fileType;
	int hitLimitToWrite;
	int eventFractionToWrite;
	long long eventCounter;
        
        int refChannel;
        
        TFile* pedestalFile;
        std::map<int,std::map<int,int> > pedValues;
  
	FILE *dataFile;
	FILE *indexFile;
	off_t stepBegin;
	
	TTree *hData;
	TTree *hIndex;
	TFile *hFile;
	// ROOT Tree fields
	float		brStep1;
	float		brStep2;
	long long 	brStepBegin;
	long long 	brStepEnd;

	unsigned short	brN;
	unsigned short	brJ[128];
	long long	brTime[128];
	long long	brTimeDelta[128];
	unsigned int	brChannelID[128];
	unsigned int    brChannelCount[128];
	float		brToT[128];
	float		brT1Coarse[128];
	float		brT1Fine[128];
	float		brT2Coarse[128];
	float		brT2Fine[128];
	float		brQCoarse[128];
	float		brQFine[128];
	float		brEnergy[128];
	float		brQT1[128];
	float		brQT2[128];
	unsigned short	brTacID[128];
	int		brXi[128];
	int		brYi[128];
	float		brX[128];
	float 		brY[128];
	float 		brZ[128];
	
	struct Event {
		uint8_t mh_n; 
		uint8_t mh_j;
		long long time;
		float e;
		int id;
	} __attribute__((__packed__));
	
public:
  DataFileWriter(char *fName, double frequency, FILE_TYPE fileType, int hitLimitToWrite, int eventFractionToWrite, bool coincidence, int refChannel, TFile* pedestalFile) {
		this->frequency = frequency;
		this->fileType = fileType;
		this->hitLimitToWrite = hitLimitToWrite;
		this->eventFractionToWrite = eventFractionToWrite;
		this->eventCounter = 0;
                this->refChannel = refChannel;
                this->pedestalFile = pedestalFile;
                
                
                if( pedestalFile != NULL )
                {
                  std::cout << "pedestals!" << std::endl;
                  for(int ch = 0; ch < 128; ++ch)
                  {
                    TProfile* prof = (TProfile*)( pedestalFile->Get(Form("p_qfine_ch%d_2",ch)) );
                    if( !prof ) continue;
                    for(int jj = 0; jj < 8; ++jj)
                      (pedValues[ch])[jj] = prof->GetBinContent(jj+1);
                  }
                }
                
                
		stepBegin = 0;
		
		if (fileType == FILE_ROOT){
			hFile = new TFile(fName, "RECREATE");
			int bs = 512*1024;

			hData = new TTree("data", "Event List", 2);
			hData->Branch("step1", &brStep1, bs);
			hData->Branch("step2", &brStep2, bs);
			
			hData->Branch("mh_n", &brN, bs);
			hData->Branch("mh_j", brJ, "mh_j[128]/s");
			hData->Branch("tot", brToT, "tot[128]/F");
			hData->Branch("t1coarse", brT1Coarse, "t1coarse[128]/F");
			hData->Branch("t1fine", brT1Fine, "t1fine[128]/F");
			hData->Branch("t2coarse", brT2Coarse, "t2coarse[128]/F");
			hData->Branch("t2fine", brT2Fine, "t2fine[128]/F");
			hData->Branch("qcoarse", brQCoarse, "qcoarse[128]/F");
			hData->Branch("qfine", brQFine, "qfine[128]/F");
			hData->Branch("time", brTime, "time[128]/L");
			hData->Branch("timeDelta", brTimeDelta, "timeDelta[128]/L");
			hData->Branch("channelID", brChannelID, "channelID[128]/i");
			hData->Branch("channelCount", brChannelCount, "channelCount[128]/i");
			hData->Branch("energy", brEnergy, "energy[128]/F");
			hData->Branch("qT1", brQT1, "qT1[128]/F");
			hData->Branch("qT2", brQT2, "qT2[128]/F");
			hData->Branch("tacID", brTacID, "tacID[128]/s");
			hData->Branch("xi", brXi, "xi[128]/I");
			hData->Branch("yi", brYi, "yi[128]/I");
			hData->Branch("x", brX, "x[128]/F");
			hData->Branch("y", brY, "y[128]/F");
			hData->Branch("z", brZ, "z[128]/F");
			hIndex = new TTree("index", "Step Index", 2);
			hIndex->Branch("step1", &brStep1, bs);
			hIndex->Branch("step2", &brStep2, bs);
			hIndex->Branch("stepBegin", &brStepBegin, bs);
			hIndex->Branch("stepEnd", &brStepEnd, bs);
		}
		else if(fileType == FILE_BINARY) {
			char fName2[1024];
			sprintf(fName2, "%s.ldat", fName);
			dataFile = fopen(fName2, "w");
			sprintf(fName2, "%s.lidx", fName);
			indexFile = fopen(fName2, "w");
			assert(dataFile != NULL);
			assert(indexFile != NULL);
		}
		else {
			dataFile = fopen(fName, "w"); // 
			assert(dataFile != NULL);
			indexFile = NULL;
		}
	};
	
	~DataFileWriter() {
		if (fileType == FILE_ROOT){
			hFile->Write();
			hFile->Close();
		}
		else if(fileType == FILE_BINARY) {
			fclose(dataFile);
			fclose(indexFile);
		}
		else {
			fclose(dataFile);
		}
	}
	
	void closeStep(float step1, float step2) {
		if (fileType == FILE_ROOT){
			brStepBegin = stepBegin;
			brStepEnd = hData->GetEntries();
			brStep1 = step1;
			brStep2 = step2;
			hIndex->Fill();
			stepBegin = hData->GetEntries();
			hFile->Write();
		}
		else if(fileType == FILE_BINARY) {
			fprintf(indexFile, "%llu\t%llu\t%e\t%e\n", stepBegin, ftell(dataFile), step1, step2);
			stepBegin = ftell(dataFile);
		}
		else {
			// Do nothing
		}
	};
	
  void addEvents(float step1, float step2,EventBuffer<GammaPhoton> *buffer, bool coincidence, int refChannel, bool pedestals) {
		bool writeMultipleHits = false;
                
		double Tps = 1E12/frequency;
		float Tns = Tps / 1000;
                
		long long tMin = buffer->getTMin() * (long long)Tps;
                
                
                // std::cout << "frequency: " << frequency << "   Tps: " << Tps << "   Tns: " << Tns << "   tMin: " << tMin << " (ps)" << std::endl;
		int N = buffer->getSize();
		for (int i = 0; i < N; i++) {
			long long tmpCounter = eventCounter;
			eventCounter += 1;
			if((tmpCounter % 1024) >= eventFractionToWrite) continue;

			GammaPhoton &p = buffer->get(i);
			
                        for(unsigned int m = 0; m < 128; ++m){
                            brJ[m] = 0;
                            brTime[m] = 0;
                            brTimeDelta[m] = 0;
                            brChannelID[m] = 0;
                            brChannelCount[m] = 0;
                            brToT[m] = 0;
                            brT1Coarse[m] = 0;
                            brT1Fine[m] = 0;
                            brT2Coarse[m] = 0;
                            brT2Fine[m] = 0;
                            brQCoarse[m] = 0;
                            brQFine[m] = 0;
                            brEnergy[m] = 0;
                            brQT1[m] = 0;
                            brQT2[m] = 0;
                            brTacID[m] = 0;
                            brX[m] = 0;
                            brY[m] = 0;
                            brZ[m] = 0;
                            brXi[m] = 0;
                            brYi[m] = 0;
                        }
                        
			if(!p.valid) continue;
			Hit &h0 = *p.hits[0];
			int limit = (hitLimitToWrite < p.nHits) ? hitLimitToWrite : p.nHits;
                        bool hasRefChannel = false;
			for(int m = 0; m < limit; m++) {
				Hit &h = *p.hits[m];
				float Eunit = 1.0;

                                if( h.raw->channelID == refChannel ) hasRefChannel = true;
                                
				if (fileType == FILE_ROOT){
					brStep1 = step1;
					brStep2 = step2;
                                        
					if( brChannelCount[h.raw->channelID] == 0 )
                                        {
                                          brN  = p.nHits;
                                          brJ[h.raw->channelID] = m;
                                          brTime[h.raw->channelID] = ((long long)(h.time * Tps)) + tMin;
                                          brTimeDelta[h.raw->channelID] = (long long)(h.time - h0.time) * Tps;
                                          brChannelID[h.raw->channelID] = h.raw->channelID;
                                          brChannelCount[h.raw->channelID] += 1;
                                          brToT[h.raw->channelID] = (h.timeEnd - h.time) * Tps;
					  brT1Coarse[h.raw->channelID] = h.raw->time & 0x1FFF;
					  brT1Fine[h.raw->channelID] = h.raw->t1fine;
					  brT2Coarse[h.raw->channelID] = h.raw->timeEnd & 0x3FF;
					  brT2Fine[h.raw->channelID] = h.raw->t2fine;
                                          brQCoarse[h.raw->channelID] = h.raw->timeEndQ & 0x3FF;
                                          brQFine[h.raw->channelID] = h.raw->qfine;
                                          if( !pedestals ) brEnergy[h.raw->channelID] = h.energy * Eunit;
                                          else brEnergy[h.raw->channelID] = h.raw->qfine -(pedValues[h.raw->channelID])[h.raw->tacID];
					  brQT1[h.raw->channelID] = h.qT1;
					  brQT2[h.raw->channelID] = h.qT2;
                                          brTacID[h.raw->channelID] = h.raw->tacID;
                                          brX[h.raw->channelID] = h.x;
                                          brY[h.raw->channelID] = h.y;
                                          brZ[h.raw->channelID] = h.z;
                                          brXi[h.raw->channelID] = h.xi;
                                          brYi[h.raw->channelID] = h.yi;
                                        }
                                        else
                                        {
                                          brChannelCount[h.raw->channelID] += 1;                                        
                                        }
				}
				else if(fileType == FILE_BINARY) {
					Event eo = { 
						(uint8_t)p.nHits, (uint8_t)m,
						((long long)(h.time * Tps)) + tMin,
						h.energy * Eunit,
						(int)h.raw->channelID
					};
					fwrite(&eo, sizeof(eo), 1, dataFile);
				}
				else {
					fprintf(dataFile, "%d\t%d\t%lld\t%f\t%d\n",
						p.nHits, m,
						((long long)(h.time * Tps)) + tMin,
						h.energy * Eunit,
						h.raw->channelID
					);
				}
			}
                        
                        if( !coincidence || (coincidence && limit > 1 && ( hasRefChannel || refChannel == -1) ) )
                          hData->Fill();
		}
		
	};
	
};

class WriteHelper : public OverlappedEventHandler<GammaPhoton, GammaPhoton> {
private: 
	DataFileWriter *dataFileWriter;
	float step1;
	float step2;
        bool coincidence;
        int refChannel;
        bool pedestals;
public:
         WriteHelper(DataFileWriter *dataFileWriter, float step1, float step2, bool coincidence, int refChannel, bool pedestals, EventSink<GammaPhoton> *sink) :
		OverlappedEventHandler<GammaPhoton, GammaPhoton>(sink, true),
		dataFileWriter(dataFileWriter), step1(step1), step2(step2), coincidence(coincidence), refChannel(refChannel), pedestals(pedestals)
	{
	};
	
	EventBuffer<GammaPhoton> * handleEvents(EventBuffer<GammaPhoton> *buffer) {
          dataFileWriter->addEvents(step1, step2, buffer, coincidence, refChannel, pedestals);
		return buffer;
	};
};

void displayHelp(char * program)
{
	fprintf(stderr, "Usage: %s --config <config_file> -i <input_file_prefix> -o <output_file_prefix> [optional arguments]\n", program);
	fprintf(stderr, "Arguments:\n");
	fprintf(stderr,  "  --config \t\t Configuration file containing path to tdc calibration table \n");
	fprintf(stderr,  "  -i \t\t\t Input file prefix - raw data\n");
	fprintf(stderr,  "  -o \t\t\t Output file name - by default in text data format\n");
	fprintf(stderr, "Optional flags:\n");
	fprintf(stderr,  "  --writeBinary \t Set the output data format to binary\n");
	fprintf(stderr,  "  --writeRoot \t\t Set the output data format to ROOT TTree\n");
	fprintf(stderr,  "  --writeMultipleHits N \t\t Writes multiple hits, up to the Nth hit\n");
	fprintf(stderr,  "  --writeFraction N \t\t Fraction of events to write. Default: 100%.\n");
        fprintf(stderr,  "  --coincidence \t Only save coincidences\n");
        fprintf(stderr,  "  --refChannel \t Reference channel\n");
        fprintf(stderr,  "  --pedestals \t Use pedestals in energy reconstruction\n");
	fprintf(stderr,  "  --help \t\t Show this help message and exit \n");	
	
};

void displayUsage(char *argv0)
{
	printf("Usage: %s --config <config_file> -i <input_file_prefix> -o <output_file_prefix> [optional arguments]\n", argv0);
}


int main(int argc, char *argv[])
{
	char *configFileName = NULL;
        char *inputFilePrefix = NULL;
        char *outputFileName = NULL;
	FILE_TYPE fileType = FILE_TEXT;
	int hitLimitToWrite = 128;
	long long eventFractionToWrite = 1024;
        bool coincidence = false;
        int refChannel = -1;
        bool pedestals;

        static struct option longOptions[] = {
                { "help", no_argument, 0, 0 },
                { "config", required_argument, 0, 0 },
		{ "writeBinary", no_argument, 0, 0 },
		{ "writeRoot", no_argument, 0, 0 },
		{ "writeMultipleHits", required_argument, 0, 0},
		{ "writeFraction", required_argument },
		{ "coincidence", no_argument, 0, 0 },
		{ "refChannel", required_argument, 0, 0 },
		{ "pedestals", no_argument, 0, 0 }
        };

        while(true) {
                int optionIndex = 0;
                int c = getopt_long(argc, argv, "i:o:c:",longOptions, &optionIndex);

                if(c == -1) break;
                else if(c != 0) {
                        // Short arguments
                        switch(c) {
                        case 'i':       inputFilePrefix = optarg; break;
                        case 'o':       outputFileName = optarg; break;
			default:        displayUsage(argv[0]); exit(1);
			}
		}
		else if(c == 0) {
			switch(optionIndex) {
			case 0:		displayHelp(argv[0]); exit(0); break;
                        case 1:		configFileName = optarg; break;
			case 2:		fileType = FILE_BINARY; break;
			case 3:		fileType = FILE_ROOT; break;
			case 4:		hitLimitToWrite = boost::lexical_cast<int>(optarg); break;
			case 5:		eventFractionToWrite = round(1024 *boost::lexical_cast<float>(optarg) / 100.0); break;
                        case 6:         coincidence = true; break;
                        case 7:         refChannel = boost::lexical_cast<int>(optarg); break;
                        case 8:         pedestals = true; break;
			default:	displayUsage(argv[0]); exit(1);
			}
		}
		else {
			assert(false);
		}
	}

	if(configFileName == NULL) {
		fprintf(stderr, "--config must be specified\n");
		exit(1);
	}
	
	if(inputFilePrefix == NULL) {
		fprintf(stderr, "-i must be specified\n");
		exit(1);
	}

	if(outputFileName == NULL) {
		fprintf(stderr, "-o must be specified\n");
		exit(1);
	}
        
        TFile* pedestalFile = NULL;
        if(pedestals){
          std::string pedestalFileName(Form("%s_pedestals.root",inputFilePrefix));
          size_t pos = pedestalFileName.find("raw");
          pedestalFileName.replace(pos,3,"reco");
          pedestalFile = TFile::Open(pedestalFileName.c_str(),"READ");
        }
        
	RawReader *reader = RawReader::openFile(inputFilePrefix);
	
	// If data was taken in ToT mode, do not attempt to load these files
	unsigned long long mask = SystemConfig::LOAD_ALL;
	if(reader->isTOT()) {
		mask ^= (SystemConfig::LOAD_QDC_CALIBRATION | SystemConfig::LOAD_ENERGY_CALIBRATION);
	}
	SystemConfig *config = SystemConfig::fromFile(configFileName, mask);
	
	DataFileWriter *dataFileWriter = new DataFileWriter(outputFileName, reader->getFrequency(), fileType, hitLimitToWrite, eventFractionToWrite, coincidence, refChannel, pedestalFile);
	
	for(int stepIndex = 0; stepIndex < reader->getNSteps(); stepIndex++) {
		float step1, step2;
		reader->getStepValue(stepIndex, step1, step2);
		printf("Processing step %d of %d: (%f, %f)\n", stepIndex+1, reader->getNSteps(), step1, step2);
		fflush(stdout);
		reader->processStep(stepIndex, true,
				new CoarseSorter(
				new ProcessHit(config, reader,
				new SimpleGrouper(config,
                                new WriteHelper(dataFileWriter, step1, step2, coincidence, refChannel, pedestals,
				new NullSink<GammaPhoton>()
				)))));
		
		dataFileWriter->closeStep(step1, step2);
	}

	delete dataFileWriter;
	delete reader;

	return 0;
}
