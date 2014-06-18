/***************************************************************************
*
* $Id: StIstClusterMaker.cxx,v 1.11 2014/04/15 06:46:59 ypwang Exp $
*
* Author: Yaping Wang, March 2013
****************************************************************************
* Description: 
* See header file.
****************************************************************************
*
* $Log: StIstClusterMaker.cxx,v $
* Revision 1.11  2014/04/15 06:46:59  ypwang
* updates for collections clear due to Clear() function removed from StIstCollection
*
* Revision 1.10  2014/03/25 03:06:52  ypwang
* updates on Db table accessory method
*
* Revision 1.9  2014/03/24 15:55:07  ypwang
* minor updates due to returned const pointers in StIstDbMaker
*
* Revision 1.8  2014/03/17 21:51:56  ypwang
* minor update due to some IST constants moved to StEnumurations.h
*
* Revision 1.7  2014/02/16 21:42:54  ypwang
* getting number of time bins used in current event by StIstCollection::getNumTimeBins() function
*
* Revision 1.6  2014/02/15 20:02:37  ypwang
* Clear() member function added, and mIstCollectionPtr data member defined
*
* Revision 1.4  2014/02/08 03:34:16  ypwang
* updating scripts
*
*
****************************************************************************
* StIstClusterMaker.cxx,v 1.0
* Revision 1.0 2013/11/04 15:55:30 Yaping
* Initial version
****************************************************************************/

#include "StIstClusterMaker.h"
#include "StEvent.h"
#include "StEvent/StEnumerations.h"
#include "StRoot/StIstUtil/StIstCollection.h"
#include "StRoot/StIstUtil/StIstRawHit.h"
#include "StRoot/StIstUtil/StIstRawHitCollection.h"
#include "StRoot/StIstUtil/StIstCluster.h"
#include "StRoot/StIstUtil/StIstClusterCollection.h"
#include "StRoot/StIstUtil/StIstConsts.h"
#include "StIstIClusterAlgo.h"
#include "StIstSimpleClusterAlgo.h"
#include "StIstScanClusterAlgo.h"

#include "StRoot/StIstDbMaker/StIstDbMaker.h"
#include "tables/St_istControl_Table.h"

StIstClusterMaker::StIstClusterMaker( const char* name ) : StMaker(name), mIstCollectionPtr(0), mClusterAlgoPtr(0), mIstDbMaker(0), mTimeBin(-1), mSplitCluster(1)
{
  /* nothing to do */
};

void StIstClusterMaker::Clear( Option_t *opts )
{
   if( mIstCollectionPtr ) {
	for ( unsigned char i = 0; i < kIstNumLadders; ++i ) {
	    mIstCollectionPtr->getRawHitCollection(i)->Clear( "" );
	    mIstCollectionPtr->getClusterCollection(i)->Clear( "" );
	}
   }
};

Int_t StIstClusterMaker::Make()
{
  Int_t ierr = kStOk;

  //input data
  TObjectSet* istDataSet = (TObjectSet*)GetDataSet("istRawHitAndCluster");
  if (! istDataSet) {
      LOG_WARN << "Make() - there is no istDataSet (raw hit and cluster) " << endm;
      ierr = kStWarn;
  }

  mIstCollectionPtr = (StIstCollection*)istDataSet->GetObject();
  if(!mIstCollectionPtr) {
      LOG_WARN << "Make() - no istCollection."<<endm;
      ierr = kStWarn;
  }   
 
  if( !ierr ){
      for( unsigned char ladderIdx=0; ladderIdx < kIstNumLadders; ++ladderIdx ){  
           StIstRawHitCollection *rawHitCollectionPtr   = mIstCollectionPtr->getRawHitCollection( ladderIdx );
           StIstClusterCollection *clusterCollectionPtr = mIstCollectionPtr->getClusterCollection( ladderIdx );
       
           if( rawHitCollectionPtr && clusterCollectionPtr ){
		UShort_t numRawHits = rawHitCollectionPtr->getNumRawHits();
                if(numRawHits<mMinNumOfRawHits)   {
                    LOG_WARN <<"no rawHits found in ladder " << (short) (ladderIdx+1) << "! " <<endl;
                    continue;
                }
                if(numRawHits>mMaxNumOfRawHits)   { // set to 460 per ladder (~10% occupancy)
                    LOG_WARN <<"too large number of raw hits found in ladder " << (short) (ladderIdx+1) << "! " <<endl;
                    continue;
                }

               // clustering and splitting
	       mClusterAlgoPtr->setUsedTimeBin(mTimeBin);
	       mClusterAlgoPtr->setSplitFlag(mSplitCluster);
               Int_t loc_ierr = mClusterAlgoPtr->doClustering(*mIstCollectionPtr, *rawHitCollectionPtr, *clusterCollectionPtr );
               if(loc_ierr!=kStOk) {
                   LOG_WARN <<"StClusterMaker::Make(): clustering for ladder " << (short) (ladderIdx+1) << " returned " << loc_ierr <<endm;
                   if(loc_ierr>ierr)
                       ierr=loc_ierr;
               }
           }           
      }
  }

  LOG_DEBUG << "End of ist-clust-maker, print all raw hits & clusters: " << endm;
  LOG_DEBUG << "Total raw hits=" <<mIstCollectionPtr->getNumRawHits()<<", total Clusters=" <<  mIstCollectionPtr->getNumClusters() <<endm;

  if(Debug()>2) {
    static unsigned char nTimeBin = mIstCollectionPtr->getNumTimeBins();
    Int_t rawHitIdx = 0, clusterIdx = 0;
    for(unsigned char iLadder=0; iLadder < kIstNumLadders; iLadder++) {
        LOG_DEBUG <<"Content: iLadder="<<(short) iLadder+1<< " # of : raw hits="<<mIstCollectionPtr->getNumRawHits(iLadder) <<"  clusters=" <<mIstCollectionPtr->getNumClusters( iLadder)<<endm;
        // ..... print all raw hits ....
        StIstRawHitCollection *rawHitPtr = mIstCollectionPtr->getRawHitCollection(iLadder);
        vector<StIstRawHit*> &rawHitVec = rawHitPtr->getRawHitVec();
        for( std::vector< StIstRawHit* >::iterator it=rawHitVec.begin();it!=rawHitVec.end();++it)    {
	    unsigned char maxTb = (*it)->getMaxTimeBin();
	    if( maxTb < 0 || maxTb >= nTimeBin)
		maxTb = (*it)->getDefaultTimeBin();

            LOG_DEBUG << "raw hit: Idx=" << rawHitIdx << " elecId=" << (*it)->getChannelId() << " Charge=" << (*it)->getCharge(maxTb) << " ChargeErr=" << (*it)->getChargeErr(maxTb) << " decode0: at ladder=" <<(short)(*it)->getLadder() << " sensor=" << (short)(*it)->getSensor() << " column=" <<(short)(*it)->getColumn() << " row=" << (short)(*it)->getRow() <<endm;
	    ++rawHitIdx;
        }
    
        // ..... print all 1D clusters  ....
        StIstClusterCollection *clustPtr= mIstCollectionPtr->getClusterCollection(iLadder);
        vector<StIstCluster*> &clustVec = clustPtr->getClusterVec();
        for( std::vector< StIstCluster* >::iterator it=clustVec.begin();it!=clustVec.end();++it)    {
            LOG_DEBUG << "cluster: Idx=" << clusterIdx << " totCharge=" << (*it)->getTotCharge() << " totChargeErr=" << (*it)->getTotChargeErr() << " meanColumn=" <<(*it)->getMeanColumn() << " meanRow= " << (*it)->getMeanRow() << " at ladder=" << (short)(*it)->getLadder() << " sensor=" << (short)(*it)->getSensor() << " clusterSize=" << (short)(*it)->getNRawHits() << " clusterSize(Z)=" << (short)(*it)->getNRawHitsZ() << " clusterSize(R-Phi)="<< (short)(*it)->getNRawHitsRPhi() << endm;
	    ++clusterIdx;
        }
    }
  }
  return ierr;

};

Int_t StIstClusterMaker::setClusterAlgo(StIstIClusterAlgo* algo)
{
    mClusterAlgoPtr=algo;
    return kStOk;
}

Int_t StIstClusterMaker::Init()
{
  Int_t ierr = kStOk;

  if( !mClusterAlgoPtr ){
     LOG_INFO << "IST clustering algorithm: Scanning algorithm" << endm;
     mClusterAlgoPtr=new StIstScanClusterAlgo();
  }

  if( !ierr )
     ierr = mClusterAlgoPtr->Init();

  mIstDbMaker = (StIstDbMaker*)GetMaker("istDb");
  if(!mIstDbMaker) {
      LOG_WARN << "Error getting IST Db maker handler" << endm;
      ierr = kStWarn;
  }

  return ierr;
};

Int_t StIstClusterMaker::InitRun(Int_t runnumber)
{
  Int_t ierr = kStOk;

  // control parameters
  const TDataSet *dbControl = mIstDbMaker->GetControl();
  St_istControl *istControl = 0;
  istControl = (St_istControl *)dbControl->Find("istControl");
  if(!istControl) {
       LOG_ERROR << "Dataset does not contain IST control table!" << endm;
       ierr = kStErr;
  }
  else {
       istControl_st *istControlTable = istControl->GetTable() ;
       if (!istControlTable)  {
            LOG_ERROR << "Pointer to IST control table is null" << endm;
            ierr = kStErr;
       }
       else {
	    mMinNumOfRawHits = istControlTable[0].kIstMinNumOfRawHits;
  	    mMaxNumOfRawHits = istControlTable[0].kIstMaxNumOfRawHits;
       }
  }

  return ierr;
};

ClassImp(StIstClusterMaker);