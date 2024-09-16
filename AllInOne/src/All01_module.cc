//
//  A first example for people who are familiar with most of the concepts
//  of HEP frameworks and need to learn how to work with Mu2e code in
//  the Mu2e environment.
//
//  Original author Rob Kutschke
//

#include "Offline/RecoDataProducts/inc/CaloCluster.hh"
#include "Offline/RecoDataProducts/inc/CrvCoincidenceCluster.hh"
#include "Offline/RecoDataProducts/inc/KalSeed.hh"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"

#include "art_root_io/TFileService.h"

#include "TH1F.h"
#include "TNtuple.h"

#include <iostream>
#include <limits>

using std::cout;
using std::endl;

namespace mu2e {

  class All01 : public art::EDAnalyzer {

  public:
    struct Config {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;

      fhicl::Atom<art::InputTag> kalSeedsTag{Name("kalSeedsTag"), Comment("Input tag for a KalSeedCollection.")};
      fhicl::Atom<art::InputTag>    crvCCTag{Name("crvCCTag"),    Comment("Input tag for a CrvCoincidenceClusterCollection.")};
      fhicl::Atom<int>              maxPrint{Name("maxPrint"),    Comment("Maximum number of events to print.")};
      fhicl::Atom<double>               tmin{Name("tmin"),        Comment("Fiducial time cut.")};
    };
    typedef art::EDAnalyzer::Table<Config> Parameters;

    explicit All01(const Parameters& conf);

    void beginJob() override;
    void analyze( const art::Event& event) override;

  private:

    // A copy of the run time configuration.
    Config _conf;

    // Values from the configuration
    art::ProductToken<KalSeedCollection>                 _kalSeedsToken;
    art::ProductToken<CrvCoincidenceClusterCollection>   _crvCCToken;
    double                                               _tmin;
    int                                                  _maxPrint;

    // Counter to limit printout.
    int _nEvents = 0;

    // Histograms and ntuples
    TH1F* _hNTracks    = nullptr;
    TH1F* _hHasCalo    = nullptr;
    TH1F* _hnDOF       = nullptr;
    TH1F* _ht0         = nullptr;
    TH1F* _hp          = nullptr;
    TH1F* _hpErr       = nullptr;
    TH1F* _hnSkip      = nullptr;
    TH1F* _heOverP     = nullptr;
    TH1F* _hnCrvCC     = nullptr;
    TH1F* _hdTCrv      = nullptr;
    TNtuple* _ntup     = nullptr;

  };

  All01::All01(const Parameters& conf)
    : art::EDAnalyzer(conf),
      _conf(conf()),
      _kalSeedsToken{consumes<KalSeedCollection>(conf().kalSeedsTag())},
    _crvCCToken{consumes<CrvCoincidenceClusterCollection>(conf().crvCCTag())},
    _tmin(conf().tmin()),
    _maxPrint(conf().maxPrint()){
    }

  void All01::beginJob(){
    art::ServiceHandle<art::TFileService> tfs;
    _hNTracks    = tfs->make<TH1F>("hNTracks", "Number of tracks per event.",                               10,     0.,    10.  );
    _hnDOF       = tfs->make<TH1F>("hnDOF",    "Number of degrees of freedom in fit.",                     100,     0.,   100.  );
    _hHasCalo    = tfs->make<TH1F>("hHasCalo", "Number of calorimeter hits.",                                2,     0.,     2.  );
    _ht0         = tfs->make<TH1F>("ht0",      "Track time at mid-point of Tracker ;(ns)",                 100,     0.,  2000.  );
    _hp          = tfs->make<TH1F>("hp",       "Track momentum at mid-point of tracker;( MeV/c)",          100,    70.,   120.  );
    _hpErr       = tfs->make<TH1F>("hpErr",    "Error on track momentum at mid-point of tracker;( MeV/c)", 100,     0.,     2.  );
    _hnSkip      = tfs->make<TH1F>("hnSkip",   "Cut tree for skipped tracks;( MeV/c)",                       3,     0.,     3.  );
    _heOverP     = tfs->make<TH1F>("heOverP",  "E/p for tracks with a matched Calo Cluster",               150,     0.,     1.5 );
    _hnCrvCC     = tfs->make<TH1F>("hnCrvCC",  "Number of CRV Coincidence clusters",                        10,     0.,    10.  );
    _hdTCrv      = tfs->make<TH1F>("hdTnCrv",  "delta(T) track-CRV Coincidence cluster;(ns)",              200, -2000.,  2000.  );

    // Time, Momentum, Error on Momentum at Front, Mid and Back planes of the tracker
    _ntup = tfs->make<TNtuple>( "ntup", "Intersection ntuple",
                                "t_f:p_f:perr_f:t_m:p_m:perr_m:t_b:p_b:perr_b");

  }

  void All01::analyze( const art::Event& event){

    // Fetch data products.
    auto const& kalSeeds = event.getProduct(_kalSeedsToken);
    auto const& crvCCs   = event.getProduct(_crvCCToken);

    _hNTracks->Fill( kalSeeds.size() );
    _hnCrvCC->Fill( crvCCs.size() );

    // Ntuple buffer;
    std::array<float,9> nt;

    // Loop over fitted tracks in the event
    for ( auto const& ks : kalSeeds ) {

      // Require final fit to have converged.
      if ( ! ks.status().hasAllProperties( TrkFitFlag::kalmanConverged ) ){
        _hnSkip->Fill(0.);
        continue;
      }

      // Learn where the track crossed 3 standard reference surfaces. These surfaces are
      // planes perpendicular to the z axis and at the front, middle and back of the tracker.
      // Tracks that reflect in the magnetic mirror may have multiple intersections.
      // See KalSeed.hh to understand the return type.
      // The return type is explicit on the first line for pedagogical purposes.
      // We recommend using "auto" instead of hand coding complicated return types.

      KalSeed::InterIterCol tt_front = ks.intersections( SurfaceIdEnum::TT_Front );
      auto  tt_mid = ks.intersections( SurfaceIdEnum::TT_Mid );
      auto tt_back = ks.intersections( SurfaceIdEnum::TT_Back );

      // This is not a recommendation for analysis cut.
      // It's done here to simplify the downstream code.
      // Require exactly one intersection at each surface.
      if ( tt_front.size() != 1 || tt_mid.size() != 1 || tt_back.size() != 1 ){
        _hnSkip->Fill(1.);
        continue;
      }

      // For convenience
      auto const& tt_front0 = tt_front.at(0);
      auto const& tt_mid0   = tt_mid.at(0);
      auto const& tt_back0  = tt_back.at(0);

      // Apply fiducial time cut;
      if ( tt_mid0->time() < _tmin ){
        _hnSkip->Fill(2.);
        continue;
      }

      // Time relative to Crv Coincidence clusters.
      if ( !crvCCs.empty() ){
        for ( auto const& cc : crvCCs ){
          float dt = tt_front0->time() - cc.GetStartTime();
          _hdTCrv->Fill(dt);
        }
      }

      // If the track has an associated cluster, plot E/p.
      if ( ks.hasCaloCluster() ){
        CaloCluster const& cluster= *ks.caloCluster();
        float eOverP = cluster.energyDep()/tt_back0->mom();
        _heOverP->Fill( eOverP);
      }

      // Global properties of the track (ie. not specific to an intersection ).
      _hnDOF->Fill( ks.nDOF() );
      _hHasCalo->Fill( ks.hasCaloCluster() );

      // Fill histograms at midpoint of the tracker.
      _ht0->Fill(  tt_mid0->time()   );
      _hp->Fill(   tt_mid0->mom()    );
      _hpErr->Fill(tt_mid0->momerr() );

      // Fill ntuple with information about all 3 intersections.
      nt[0] = tt_front0->time();
      nt[1] = tt_front0->mom();
      nt[2] = tt_front0->momerr();
      nt[3] = tt_mid0->time();
      nt[4] = tt_mid0->mom();
      nt[5] = tt_mid0->momerr();
      nt[6] = tt_back0->time();
      nt[7] = tt_back0->mom();
      nt[8] = tt_back0->momerr();
      _ntup->Fill( nt.data() );

      // See what intersections are present;
      if ( ++_nEvents < _maxPrint ) {

        for ( auto const& ki : ks.intersections() ){
          cout << " Intersection: "
               << " Event: "        << event.id()
               << " Track number: " << &ks-&kalSeeds.front()
               << " Surface ID: "   << ki.surfaceId().id()
               << " Time:       "   << ki.time()
               << " Momentum:   "   << ki.mom()
               << endl;
        }

      } // end test on maxPrint

    } // end loop over kalSeeds

  } // end analyze

} // end namespace mu2e

DEFINE_ART_MODULE(mu2e::All01)
