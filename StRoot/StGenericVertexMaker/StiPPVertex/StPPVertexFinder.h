/*!
 * \class StPPVertexFinder
 * \author Jan Balewski, July 2004
 *
 *  StGenericVertexFinder implementation of PPV
 * $Id: StPPVertexFinder.h,v 1.23 2016/04/28 18:17:55 smirnovd Exp $
 *
 */
#ifdef __APPLE__
#include <sys/types.h>
#endif
#include "StGenericVertexMaker/StGenericVertexFinder.h"

#include "StPhysicalHelixD.hh" // dongx
class StiKalmanTrack;
class TrackData;
class VertexData;
class TGraphErrors;
class StEvent; 
class StiToolkit;
class StEEmcDb;

class EEmcGeomSimple;
class StBTofGeometry; 

class BtofHitList;  
class CtbHitList;
class BemcHitList;
class EemcHitList;
class Vertex3D;

class StPPVertexFinder: public StGenericVertexFinder {
 private:

  /// Takes a list of vertex candidates/seeds and updates each vertex position
  /// by fitting tracks pointing to it
  void fitTracksToVertex(VertexData &vertex) const;

  /// Creates DCA states for selected tracks (mTrackData) and fills the static
  /// container sDCAs
  void createTrackDcas(const VertexData &vertex) const;

  enum {mxH=32};
  bool examinTrackDca(const StiKalmanTrack*, TrackData &t);
  void matchTrack2BTOF(const StiKalmanTrack*, TrackData &t, StBTofGeometry *geom);  // dongx
  void matchTrack2CTB(const StiKalmanTrack*, TrackData &t);
  void matchTrack2EEMC(const StiKalmanTrack*, TrackData &t, float z);
  void matchTrack2BEMC(const StiKalmanTrack*, TrackData &t, float rxy);
  bool matchTrack2Membrane(const StiKalmanTrack*, TrackData &t);
  bool isPostCrossingTrack(const StiKalmanTrack* track);
  vector<TrackData>  mTrackData;
  vector<VertexData> mVertexData;
  Vertex3D *vertex3D; // for stand alone 3D vertex reco
  bool buildLikelihoodZ();
  bool findVertexZ(VertexData &);
  bool evalVertexZ(VertexData &);
  void exportVertices(); 

  void saveHisto(TString fname);
  int  mTotEve;
  int  eveID;
  uint  mAlgoSwitches; //binary, assign 1bit per change, use enum below
  enum {kSwitchOneHighPT=1}; 

  // params
  double mMinTrkPt;       //~ pT=0.16(GeV/c) == R=2 (m )in 2001
  double mMaxTrkDcaRxy;   //DCA to nominal beam line for each track
  float  mMaxZradius;     // used in matching: tracks to zVertex
  int    mMinMatchTr;     // for valid vertex
  float  mMaxZrange;      // cut off for tracks Z_DCA
  float  mDyBtof;         // BTOF delta y cut - dongx
  float  mMinZBtof;       // BTOF local z min cut - dongx
  float  mMaxZBtof;       // BTOF local z max cut - dongx
  float  mMinAdcBemc;     // BEMC towers with MIP response
  float  mMinAdcEemc;     // EEMC towers with MIP response
  float  mMinFitPfrac;    // nFit/nPossible
  bool   mFitPossWeighting; // Use nFit/nPossible in track weighting (ranking)
  bool   mDropPostCrossingTrack;  // enable/disable post crossing tarck rejection
  int    mStoreUnqualifiedVertex; // set the max # of vertices, sorted by rank
  float  mCut_oneTrackPT; // threshold for storing one track vertices
  int    mBeamLineTracks; // activates writing them out + lot of QA histos, 
                          // use  BFC option: VtxSeedCalG to enable it, expert only

  // beam line
  double          mX0  ;     // starting point of beam parameterization
  double          mY0  ;     // starting point of beam parameterization
  double          mdxdz;     // beam slope
  double          mdydz;     // beam slope

  // util
  StiToolkit     *mToolkit;
  BtofHitList    *btofList;  // dongx
  CtbHitList     *ctbList;
  BemcHitList    *bemcList;
  EemcHitList    *eemcList;
  StBTofGeometry *btofGeom;  // dongx btofGeometry
  StEEmcDb       *eeDb;
  EEmcGeomSimple *geomE;
  
  void dumpKalmanNodes(const StiKalmanTrack *track);
  //  void plotVertex(VertexData *);
  //  void plotTracksDca();
  void initHisto();
  
public:
  void UsePCT(bool x=true)			{setDropPostCrossingTrack(!x);}
  void setDropPostCrossingTrack(bool x=true)	{mDropPostCrossingTrack=x;}
  void Finish();

  TH1F *hA[mxH];
  TH2F *hACorr;
  TH1D *hL ;      // likelyhood distribution
  TH1D *hM, *hW ; // cumulative track mult & weight distribution, for better errZ calculation
  TObjArray * HList;
  StPPVertexFinder(VertexFit_t fitMode=VertexFit_t::Beamline1D);

  // mandatory implementations
  virtual  ~StPPVertexFinder();
  int       fit(StEvent*);        
  void      printInfo(ostream& = cout) const;
  void      UseVertexConstraint(double x0, double y0, double dxdz, double dydz, double weight);
 
  // over-written method
  virtual void  Init();
  virtual void  InitRun  (int runumber);
  virtual void  Clear(); 
  virtual void  CalibBeamLine(); // activates saving high quality prim tracks for 3D fit of the beamLine
};