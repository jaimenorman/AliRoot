// $Id$
#ifndef ALIHLTTPCHWCFDATATYPES_H
#define ALIHLTTPCHWCFDATATYPES_H
//* This file is property of and copyright by the ALICE HLT Project        * 
//* ALICE Experiment at CERN, All rights reserved.                         *
//* See cxx source for full Copyright notice                               *

//  @file   AliHLTTPCHWCFDataTypes.h
//  @author Sergey Gorbunov <sergey.gorbunov@fias.uni-frankfurt.de>
//  @author Torsten Alt <talt@cern.ch> 
//  @brief  Data types for FPGA ClusterFinder Emulator for TPC
//  @brief  ( see AliHLTTPCHWCFEmulator class )
//  @note

#include "AliHLTDataTypes.h"
#include "AliHLTTPCClusterMCData.h"
#include <vector>


struct AliHLTTPCHWCFDefinitions
{
  static const unsigned int kMaxNTimeBins = 1024+10; // max N time bins
  static const int kFixedPoint = 12; // N bits after fixed point 
                                     // for fixed point operations
};

typedef struct AliHLTTPCHWCFDefinitions AliHLTTPCHWCFDefinitions;

struct AliHLTTPCHWCFDigit
{
  //* constructor **/
  AliHLTTPCHWCFDigit(): fQ(0), fTime(0), fPeak(0), fMC()
  {}

  AliHLTUInt32_t fQ;    // charge
  AliHLTUInt32_t fTime; // timebin
  AliHLTUInt32_t fPeak;  // peak flag: 0:flat, 1:peak, 2: local minimum
  AliHLTTPCClusterMCLabel fMC; // mc label
};
typedef struct AliHLTTPCHWCFDigit AliHLTTPCHWCFDigit;

struct AliHLTTPCHWCFBunch
{
  //* constructor **/
  AliHLTTPCHWCFBunch(): fFlag(0), fRow(0), fPad(0), fBranch(0), fBorder(0),
			fEdge(0), fGain(0), fData()
  {}

  AliHLTUInt32_t fFlag; // 0 - Off, 1 - data, 2 - RCU trailer, 3 - end of data
  AliHLTUInt32_t fRow;  // row number
  AliHLTUInt32_t fPad;  // pad number
  bool fBranch;         // 0  - pad belongs to branch A, 1 - pad belongs to branch B
  bool fBorder;         // is the pad at the border of its branch
  bool fEdge;           // is the pad at the sector edge
  AliHLTUInt64_t fGain; // gain correction factor 
                        //   (fixed point integer with kFixedPoint bits after the point)
  std::vector<AliHLTTPCHWCFDigit> fData;      // signals
};
typedef struct AliHLTTPCHWCFBunch AliHLTTPCHWCFBunch;

struct AliHLTTPCHWCFClusterFragment
{
  //* constructor **/
  AliHLTTPCHWCFClusterFragment():  fFlag(0), fRow(0), fPad(0), fBranch(0), fBorder(0), fEdge(0),
    fQmax(0), fQ(0), fT(0), fP(0), fT2(0), fP2(0), fTMean(0), fLastQ(0), fLargestQ(0), fLargestQPad(0), fNPads(0), fSlope(0), fNDeconvolutedTime(0), fIsDeconvolutedPad(0),fConsecutiveTimeDeconvolution(0), fMC()
  {}

  AliHLTUInt32_t fFlag; // 0 - Off, 1 - data, 2 - RCU trailer, 3 - end of data
  AliHLTUInt32_t fRow;  // row number
  AliHLTUInt32_t fPad;  // pad number
  bool fBranch;         // 0  - pad belongs to branch A, 1 - pad belongs to branch B
  bool fBorder;         // is the pad at the border of its branch
  bool fEdge;           // is the pad at the sector edge
  AliHLTUInt64_t fQmax; // max charge, fixed point integer
  AliHLTUInt64_t fQ;    // total charge, fixed point integer
  AliHLTUInt64_t fT;    // sum of time*charge , fixed point integer
  AliHLTUInt64_t fP;    // sum of pad*charge  , fixed point integer
  AliHLTUInt64_t fT2;   // sum of time^2*charge , fixed point integer
  AliHLTUInt64_t fP2;   // sum of pad^2*charge  , fixed point integer
  AliHLTUInt64_t fTMean;// mean time, used for merging neighbouring pads
  AliHLTUInt64_t fLastQ; // for merged fragments, charge of the last (highest pad value)
                         //    fragment bein merged, needed for deconvolution
  AliHLTUInt64_t fLargestQ; //for merging: charge of the cluster fragment with the largest Q
  AliHLTUInt64_t fLargestQ2; //for merging: charge of the cluster fragment with the second largest Q
  AliHLTUInt32_t fLargestQPad; //for merging: pad that had the cluster fragment with the largest Q
  AliHLTUInt32_t fNPads; // how many pads are in the cluster 
  bool fSlope;           // for merged fragments, ==1 if fLastQ decreases
                         //   ( needed for deconvolution )
  AliHLTUInt32_t fNDeconvolutedTime; // how many cluster pads has been split in Time direction  
  bool fIsDeconvolutedPad; // tag shows if the cluster has been split in several clusters in Pad direction
  AliHLTUInt32_t fConsecutiveTimeDeconvolution; // ==2 when 2 or more cons. time sequences are deconvoluted
  std::vector<AliHLTTPCClusterMCLabel> fMC; // mc labels
};
typedef struct AliHLTTPCHWCFClusterFragment AliHLTTPCHWCFClusterFragment;

struct AliHLTTPCHWCFCluster
{
  //* constructor **/
AliHLTTPCHWCFCluster(): fFlag(0), fRowQ(0), fQ(0), fT(0), fP(0), fT2(0), fP2(0), fMC()
  {}

  AliHLTUInt32_t fFlag; // 0 - Off, 1 - data, 2 - RCU trailer, 3 - end of data
  AliHLTUInt32_t fRowQ; // bits 30-31 = 0x3
                        // bits 24-29 = row number
                        // bit  23    = flag: border cluster (edge or branch border or interleaved row)
                        // bits 0 -22 = max adc value as fixed point integer,
                        //              with 12 bits after the point
  AliHLTUInt32_t fQ;    // 0-29 total charge as fixed point integer, 12 bits after the point
                        // 30   flag: is the cluster deconvoluted in Time
                        // 31   flag: is the cluster deconvoluted in Pad
  AliHLTUInt32_t fT;    // mean time, 32-bit float stored as 32-bit integer
  AliHLTUInt32_t fP;    // mean pad,  32-bit float stored as 32-bit integer
  AliHLTUInt32_t fT2;   // mean time^2, 32-bit float stored as 32-bit integer
  AliHLTUInt32_t fP2;   // mean pad^2,  32-bit float stored as 32-bit integer
  AliHLTTPCClusterMCLabel fMC; // mc label
};
typedef struct AliHLTTPCHWCFCluster AliHLTTPCHWCFCluster;

#endif
