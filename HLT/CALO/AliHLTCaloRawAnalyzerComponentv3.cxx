// $Id$
// clang-format off
/**************************************************************************
 * This file is property of and copyright by the ALICE HLT Project        * 
 * All rights reserved.                                                   *
 *                                                                        *
 * Primary Authors: Per Thomas Hille, Oystein Djuvsland                   *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          * 
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/
// clang-format on

#include "AliCaloAltroMapping.h"
#include "AliCaloRawAnalyzer.h"
#include "AliCaloBunchInfo.h"
#include "AliCaloFitResults.h"
#include "AliHLTCaloRawAnalyzerComponentv3.h"
#include "AliHLTCaloChannelDataHeaderStruct.h"
#include "AliHLTCaloChannelDataStruct.h"
#include "AliHLTCaloMapper.h"
#include "AliHLTCaloSanityInspector.h"
#include "AliRawReaderMemory.h"
#include "AliCaloRawStreamV3.h"
#include "AliHLTCaloConstantsHandler.h"
#include "AliHLTCaloChannelRawDataStruct.h"
#include "AliLog.h"
#include "AliDAQ.h"
#include "TStopwatch.h"
#include "TSystem.h"

#include "AliCaloRawAnalyzerFactory.h"

//#include "AliCaloConstants.h"

//#include "AliCaloRawAnalyzer.h"

// using namespace Algo;

#include <vector>
using namespace std;

ClassImp(AliHLTCaloRawAnalyzerComponentv3);

AliHLTCaloRawAnalyzerComponentv3::AliHLTCaloRawAnalyzerComponentv3(TString det, fitAlgorithm algo)
  : AliHLTCaloProcessor(),
    AliHLTCaloConstantsHandler(det),
    fAnalyzerPtr(0),
    fMapperPtr(0),
    fCurrentSpec(-1),
    fDebug(false),
    fAltroMappingCache(NULL),
    fSanityInspectorPtr(0),
    fDetector(det),
    fAlgorithm(algo),
    fOffset(0),
    fBunchSizeCut(0),
    fMinPeakPosition(0),
    fMaxPeakPosition(100),
    fDoPushBadRawData(false),
    fDoPushRawData(false),
    fDoL1PhaseSubtraction(false),
    fRawDataWriter(0)

{
  // Constructor
}

AliHLTCaloRawAnalyzerComponentv3::~AliHLTCaloRawAnalyzerComponentv3()
{
  // destructor
}

int AliHLTCaloRawAnalyzerComponentv3::DoInit(int argc, const char** argv)
{
  // See base class for documentation
  int iResult = 0;

  for (int i = 0; i < argc; i++) {
    if (!strcmp("-offset", argv[i])) {
      fOffset = atoi(argv[i + 1]);
    }
    if (!strcmp("-L1PhaseSubtraction", argv[i])){
      fDoL1PhaseSubtraction = true;
    }
    if (!strcmp("-bunchsizecut", argv[i])) {
      fBunchSizeCut = atoi(argv[i + 1]);
    }
    if (!strcmp("-minpeakposition", argv[i])) {
      fMinPeakPosition = atoi(argv[i + 1]);
    }
    if (!strcmp("-maxpeakposition", argv[i])) {
      fMaxPeakPosition = atoi(argv[i + 1]);
    }
    if (!strcmp("-pushrawdata", argv[i])) {
      fDoPushRawData = true;
    }
    if (!strcmp("-pushbaddata", argv[i])) {
      fDoPushBadRawData = true;
    }
    if (fDoPushBadRawData && fDoPushRawData) {
      HLTWarning("fDoPushBadRawData and fDoPushRawData in conflict, using fDoPushRawData");
      fDoPushBadRawData = false;
    }
    if (!strcmp("-suppressalilogwarnings", argv[i])) {
      AliLog::SetGlobalLogLevel(AliLog::kError); // PHOS sometimes produces bad data -> Fill up the HLT logs...
    }
  }

  fAnalyzerPtr = AliCaloRawAnalyzerFactory::CreateAnalyzer(fAlgorithm);
  fAnalyzerPtr->SetIsZeroSuppressed(true);
  fSanityInspectorPtr = new AliHLTCaloSanityInspector();

  if (fDoPushRawData == true) {
    fRawDataWriter = new RawDataWriter(fCaloConstants);
  }

  return iResult;
}

int AliHLTCaloRawAnalyzerComponentv3::DoDeinit()
{
  // comment
  if (fRawDataWriter)
    delete fRawDataWriter;
  fRawDataWriter = NULL;
  if (fSanityInspectorPtr)
    delete fSanityInspectorPtr;
  fSanityInspectorPtr = NULL;

  if(fAltroMappingCache){
    for(int i = 0; i < 20; i++){
      if(fAltroMappingCache[i]) delete fAltroMappingCache[i];
    }
    delete[] fAltroMappingCache;
  }

  return 0;
}

void AliHLTCaloRawAnalyzerComponentv3::PrintDebugInfo()
{
  // comment
  static TStopwatch watch; // CRAP PTH
  static double wlast = -1;
  static double wcurrent = 0;

  if (true == fDebug) {
    if (fCaloEventCount % 1000 == 0) {
      cout << __FILE__ << __LINE__ << " : Processing event " << fCaloEventCount << endl;
      wlast = wcurrent;
      wcurrent = watch.RealTime();
      cout << __FILE__ << __LINE__ << "The event rate is " << 1000 / (wcurrent - wlast) << "  Hz" << endl;
      watch.Start(kFALSE);
    }
  }
}

void AliHLTCaloRawAnalyzerComponentv3::GetOutputDataSize(unsigned long& constBase, double& inputMultiplier)
{
  // comment
  constBase = sizeof(AliHLTCaloChannelDataHeaderStruct);
  inputMultiplier = 1.5;
}

bool AliHLTCaloRawAnalyzerComponentv3::CheckInputDataType(const AliHLTComponentDataType& datatype)
{
  // comment
  vector<AliHLTComponentDataType> validTypes;
  GetInputDataTypes(validTypes);

  for (UInt_t i = 0; i < validTypes.size(); i++) {
    if (datatype == validTypes.at(i)) {
      return true;
    }
  }

  HLTDebug("Invalid Datatype");
  return false;
}

int AliHLTCaloRawAnalyzerComponentv3::DoEvent(const AliHLTComponentEventData& evtData,
                                              const AliHLTComponentBlockData* blocks,
                                              AliHLTComponentTriggerData& /*trigData*/, AliHLTUInt8_t* outputPtr,
                                              AliHLTUInt32_t& size, vector<AliHLTComponentBlockData>& outputBlocks)
{
  // comment
  if (!IsDataEvent()) {
    size = 0;
    return 0;
  }
  if (true == fDebug) {
    PrintDebugInfo();
  };

  Int_t blockSize = -1;
  UInt_t totSize = 0;
  UInt_t returnSize = 0;
  const AliHLTComponentBlockData* iter = NULL;
  unsigned long ndx;

  for (ndx = 0; ndx < evtData.fBlockCnt; ndx++) {
    iter = blocks + ndx;
    if (!CheckInputDataType(iter->fDataType)) {
      continue;
    }

    if (!fDetector.CompareTo("EMCAL")) {
      // don't use STU DDLs
      if (iter->fSpecification >= AliDAQ::GetFirstSTUDDL())
        continue;
    }
    // std::cout << "I-Raw Analyzer: Next Block: " << iter->fSpecification << std::endl;
    if (iter->fSpecification != fCurrentSpec) {
      fCurrentSpec = iter->fSpecification;
      InitMapping(iter->fSpecification);
    }

    blockSize = DoIt(iter, outputPtr, size, totSize); // Processing the block
    if (blockSize > 0) {
      // totSize += blockSize; //Keeping track of the used size
      AliHLTComponentBlockData bdChannelData;
      FillBlockData(bdChannelData);
      bdChannelData.fOffset = totSize - blockSize; // FIXME
      bdChannelData.fSize = blockSize;
      bdChannelData.fDataType = GetOutputDataType();
      bdChannelData.fSpecification = iter->fSpecification;
      outputBlocks.push_back(bdChannelData);
      outputPtr += blockSize; // Updating position of the output buffer
      returnSize = totSize;
    } else {
      break;
    }
  }

  fCaloEventCount++;
  size = returnSize; // telling the framework how much buffer space we have used.

  return 0;

} // end DoEvent

Int_t AliHLTCaloRawAnalyzerComponentv3::DoIt(const AliHLTComponentBlockData* iter, AliHLTUInt8_t* outputPtr,
                                             const AliHLTUInt32_t size, UInt_t& totSize)
{
  // comment
  int tmpsize = 0;
  Int_t crazyness = 0;
  Int_t nSamples = 0;
  Short_t channelCount = 0;

  AliHLTCaloChannelDataHeaderStruct* channelDataHeaderPtr =
    reinterpret_cast<AliHLTCaloChannelDataHeaderStruct*>(outputPtr);
  AliHLTCaloChannelDataStruct* channelDataPtr =
    reinterpret_cast<AliHLTCaloChannelDataStruct*>(outputPtr + sizeof(AliHLTCaloChannelDataHeaderStruct));
  totSize += sizeof(AliHLTCaloChannelDataHeaderStruct);
  if (totSize > size) {
    HLTError("Buffer overflow: Trying to write data of size: %d bytes. Output buffer available: %d bytes.", totSize,
             size);
    return -1;
  }
  AliRawReaderMemory rawReaderMemoryPtr;
  rawReaderMemoryPtr.SetMemory(reinterpret_cast<UChar_t*>(iter->fPtr), static_cast<ULong_t>(iter->fSize));
  rawReaderMemoryPtr.SetEquipmentID(fMapperPtr->GetDDLFromSpec(iter->fSpecification) + fCaloConstants->GetDDLOFFSET());

  if(!fAltroMappingCache) InitAltroMapping();
  AliCaloRawStreamV3 altroRawStreamPtr(&rawReaderMemoryPtr, fDetector, fAltroMappingCache);

  rawReaderMemoryPtr.Reset();
  rawReaderMemoryPtr.NextEvent();

  if (fDoPushRawData == true) {
    fRawDataWriter->NewEvent();
  }
  

  if (altroRawStreamPtr.NextDDL()) {
    int cnt = 0;
    fOffset = 0;
    while (altroRawStreamPtr.NextChannel()) {
      if(altroRawStreamPtr.GetCaloFlag() > 1) continue;
      ++cnt;
      // UShort_t* firstBunchPtr = 0;
      int chId = fMapperPtr->GetChannelID(iter->fSpecification, altroRawStreamPtr.GetHWAddress());
      if (fDoPushRawData == true) {
        fRawDataWriter->SetChannelId(chId);
      }

      vector<AliCaloBunchInfo> bvctr;
      while (altroRawStreamPtr.NextBunch() == true) {
        bvctr.push_back(AliCaloBunchInfo(altroRawStreamPtr.GetStartTimeBin(), altroRawStreamPtr.GetBunchLength(),
                                         altroRawStreamPtr.GetSignals()));

        nSamples = altroRawStreamPtr.GetBunchLength();
        if (fDoPushRawData == true) {
          fRawDataWriter->WriteBunchData(altroRawStreamPtr.GetSignals(), nSamples, altroRawStreamPtr.GetEndTimeBin());
        }
        // firstBunchPtr = const_cast< UShort_t* >(  fAltroRawStreamPtr->GetSignals()  );
      }

      if(fDoL1PhaseSubtraction) fAnalyzerPtr->SetL1Phase(altroRawStreamPtr.GetL1Phase());
      AliCaloFitResults res =
        fAnalyzerPtr->Evaluate(bvctr, altroRawStreamPtr.GetAltroCFG1(), altroRawStreamPtr.GetAltroCFG2());

      // Make boundary check in x/z. In case the channel is out of bounds, raise an
      // HLTError with the full information (position, channel ID and HW Address)
      UInt_t xpos = altroRawStreamPtr.GetRow(), zpos = altroRawStreamPtr.GetColumn();
      if (xpos >= fCaloConstants->GetNXCOLUMNSMOD() || zpos >= fCaloConstants->GetNZROWSMOD())
        HLTError(
          "CaloRawAnalyzerError: Channel out-of-bounds: x[%d, max %d], z[%d, max %d], raised by channel %d/HW "
          "address %d in DDL %d",
          xpos, fCaloConstants->GetNXCOLUMNSMOD() - 1, zpos, fCaloConstants->GetNZROWSMOD() - 1, chId,
          altroRawStreamPtr.GetHWAddress(), altroRawStreamPtr.GetDDLNumber());

      HLTDebug("Channel energy: %f, max sig: %d, gain = %d, x = %d, z = %d", res.GetAmp(), res.GetMaxSig(),
               altroRawStreamPtr.IsLowGain() ? 0 : 1, xpos, zpos);
      if(res.GetTime() > 0 && res.GetAmp() > 0){    // Only write positive entries
        totSize += sizeof(AliHLTCaloChannelDataStruct);
        if (totSize > size) {
          HLTError("Buffer overflow: Trying to write data of size: %d bytes. Output buffer available: %d bytes.",
                   totSize, size);
          return -1;
        }
        channelDataPtr->fChannelID = chId;
        channelDataPtr->fEnergy = static_cast<Float_t>(res.GetAmp());
        channelDataPtr->fLowGain = altroRawStreamPtr.IsLowGain();
        channelDataPtr->fRow = altroRawStreamPtr.GetRow();
        channelDataPtr->fColumn = altroRawStreamPtr.GetColumn();
        channelDataPtr->fModuleID = altroRawStreamPtr.GetModule();
        Float_t celltime = res.GetTof();
        HLTDebug("Module %d, x[%d], z[%d], time %f, energy %f, gain %d", channelDataPtr->fModuleID, channelDataPtr->fRow, channelDataPtr->fColumn, celltime*1e9, channelDataPtr->fEnergy, channelDataPtr->fLowGain ? 0 : 1);
        if(fDoL1PhaseSubtraction){
          if(!fDetector.CompareTo("EMCAL")) celltime -= altroRawStreamPtr.GetL1Phase();
        }
        
        channelDataPtr->fTime = static_cast<Float_t>(celltime* 1e9); // convert to ns - probably precision oss
        // Time from the raw rreader already converted - no need to multiply twice
        channelDataPtr->fCrazyness = static_cast<Short_t>(crazyness);
        channelCount++;
        channelDataPtr++; // Updating position of the free output.
      }
    }
    if (fDoPushRawData == true) {
      fRawDataWriter->NewChannel();
    }
  }

  channelDataHeaderPtr->fNChannels = channelCount;
  channelDataHeaderPtr->fAlgorithm = fAlgorithm;
  channelDataHeaderPtr->fInfo = 0;

  if (fDoPushRawData == true) {
    tmpsize += fRawDataWriter->CopyBufferToSharedMemory((UShort_t*)channelDataPtr, size, totSize);
  }

  channelDataHeaderPtr->fHasRawData = fDoPushRawData;
  HLTDebug("Number of channels: %d", channelCount);
  tmpsize += sizeof(AliHLTCaloChannelDataStruct) * channelCount + sizeof(AliHLTCaloChannelDataHeaderStruct);
  return tmpsize;
}

AliHLTCaloRawAnalyzerComponentv3::RawDataWriter::RawDataWriter(AliHLTCaloConstants* cConst)
  : fRawDataBuffer(0),
    fCurrentChannelSize(0),
    fBufferIndex(0),
    fBufferSize(64 * 56 * cConst->GetNGAINS() * cConst->GetALTROMAXSAMPLES() + 1000),
    fCurrentChannelIdPtr(0),
    fCurrentChannelSizePtr(0),
    fCurrentChannelDataPtr(0),
    fTotalSize(0)
{
  // comment
  fRawDataBuffer = new UShort_t[fBufferSize];
  Init();
}

void AliHLTCaloRawAnalyzerComponentv3::InitAltroMapping() {
  fAltroMappingCache = new AliAltroMapping*[20];
  memset(fAltroMappingCache, 0, sizeof(AliAltroMapping *) * 20);
  
  Int_t nmodules, nrcu, nsides;
  // PHOS and EMCAL have different number of RCU per module
  //For PHOS (different mappings for different modules)
  if(fDetector == "PHOS")  {
    nmodules = 5;
    nrcu     = 4;
    nsides   = 1;
  }
  //For EMCAL (the same mapping for all modules)
  if(fDetector == "EMCAL")  {
    nmodules = 1;
    nrcu     = 2;
    nsides   = 2;
  }
  TString sides[]={"A","C"};

  TString path = gSystem->Getenv("ALICE_ROOT");
  path += "/"+fDetector+"/mapping/";
  TString path1, path2;
  for(Int_t m = 0; m < nmodules; m++) {
    path1 = path;
    if(fDetector == "EMCAL") {
	    path1 += "RCU";
    } else if(fDetector == "PHOS" ) {
	    path1 += "Mod";
	    path1 += m;
	    path1 += "RCU";
    }
    for(Int_t j = 0; j < nsides; j++){
	    for(Int_t i = 0; i < nrcu; i++) {
	      path2 = path1;
	      path2 += i;
	      if(fDetector == "EMCAL") path2 += sides[j];
	      path2 += ".data";
	      HLTDebug(2,Form("Mapping file: %s",path2.Data()));
	      fAltroMappingCache[m*nsides*nrcu + j*nrcu + i] = new AliCaloAltroMapping(path2.Data());
	    }
    }
  }
}

AliHLTCaloRawAnalyzerComponentv3::RawDataWriter::~RawDataWriter() { delete[] fRawDataBuffer; }

void AliHLTCaloRawAnalyzerComponentv3::RawDataWriter::Init()
{
  // comment
  fCurrentChannelIdPtr = fRawDataBuffer;
  fCurrentChannelSizePtr = fRawDataBuffer + 1;
  fCurrentChannelDataPtr = fRawDataBuffer + 2;
  ResetBuffer();
}

void AliHLTCaloRawAnalyzerComponentv3::RawDataWriter::NewEvent()
{
  // comment
  Init();
  fTotalSize = 0;
}

void AliHLTCaloRawAnalyzerComponentv3::RawDataWriter::NewChannel()
{
  // comment
  *fCurrentChannelSizePtr = fCurrentChannelSize;
  fCurrentChannelIdPtr += fCurrentChannelSize;
  fCurrentChannelSizePtr += fCurrentChannelSize;
  fCurrentChannelDataPtr += fCurrentChannelSize;
  fBufferIndex = 0;
  fCurrentChannelSize = 2;
  fTotalSize += 2;
}

void AliHLTCaloRawAnalyzerComponentv3::RawDataWriter::WriteBunchData(const UShort_t* bunchdata, const int length,
                                                                     const UInt_t starttimebin)
{
  fCurrentChannelDataPtr[fBufferIndex] = starttimebin;
  fCurrentChannelSize++;
  fBufferIndex++;
  fCurrentChannelDataPtr[fBufferIndex] = length;
  fCurrentChannelSize++;
  fBufferIndex++;

  fTotalSize += 2;

  for (int i = 0; i < length; i++) {
    fCurrentChannelDataPtr[fBufferIndex + i] = bunchdata[i];
  }

  fCurrentChannelSize += length;
  fTotalSize += length;
  fBufferIndex += length;
}

void AliHLTCaloRawAnalyzerComponentv3::RawDataWriter::SetChannelId(const UShort_t channeldid)
{
  *fCurrentChannelIdPtr = channeldid;
}

void AliHLTCaloRawAnalyzerComponentv3::RawDataWriter::ResetBuffer()
{
  for (int i = 0; i < fBufferSize; i++) {
    fRawDataBuffer[i] = 0;
  }
}

int AliHLTCaloRawAnalyzerComponentv3::RawDataWriter::CopyBufferToSharedMemory(UShort_t* memPtr, const int sizetotal,
                                                                              const int sizeused)
{
  int sizerequested = (sizeof(int) * fTotalSize + sizeused);

  if (sizerequested > sizetotal) {
    return 0;
  } else {
    for (int i = 0; i < fTotalSize; i++) {
      memPtr[i] = fRawDataBuffer[i];
    }
    return fTotalSize;
  }
}
