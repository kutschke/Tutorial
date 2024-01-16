//
//  A first example for people who are familiar with most of the concepts
//  of HEP frameworks and need to learn how to work with Mu2e code in
//  the Mu2e environment.
//
//  Original author Rob Kutschke
//

#include "Offline/RecoDataProducts/inc/KalSeed.hh"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"

#include "art_root_io/TFileService.h"

#include "TH1F.h"
#include "TNtuple.h"

#include <iostream>
using std::cout;
using std::endl;

namespace mu2e {

  class All01 : public art::EDAnalyzer {

  public:
    struct Config {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;

      fhicl::Atom<art::InputTag> kalSeedsTag{Name("kalSeedsTag"), Comment("Input tag for a KalSeedCollection.")};
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
    art::ProductToken<KalSeedCollection>   _kalSeedsToken;
    double                                 _tmin;
    int                                    _maxPrint;

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
    TNtuple* _ntup     = nullptr;

  };

  All01::All01(const Parameters& conf)
    : art::EDAnalyzer(conf),
      _conf(conf()),
      _kalSeedsToken{consumes<KalSeedCollection>(conf().kalSeedsTag())},
    _tmin(conf().tmin()),
    _maxPrint(conf().maxPrint()){
}

  void All01::beginJob(){
    art::ServiceHandle<art::TFileService> tfs;
    _hNTracks    = tfs->make<TH1F>("hNTracks", "Number of tracks per event.",                               10,  0.,    10. );
    _hnDOF       = tfs->make<TH1F>("hnDOF",    "Number of degrees of freedom in fit.",                     100,  0.,   100. );
    _hHasCalo    = tfs->make<TH1F>("hHasCalo", "Number of calorimeter hits.",                                2,  0.,     2. );
    _ht0         = tfs->make<TH1F>("ht0",      "Track time at mid-point of Tracker ;(ns)",                 100,  0.,  2000. );
    _hp          = tfs->make<TH1F>("hp",       "Track momentum at mid-point of tracker;( MeV/c)",          100, 70.,   120. );
    _hpErr       = tfs->make<TH1F>("hpErr",    "Error on track momentum at mid-point of tracker;( MeV/c)", 100,  0.,     2. );
    _hnSkip      = tfs->make<TH1F>("hnSkip",   "Cut tree for skipped tracks;( MeV/c)",                       3,  0.,     3. );

    // Time, Momentum, Error on Momentum at Front, Mid and Back planes of the tracker
    _ntup = tfs->make<TNtuple>( "ntup", "Intersection ntuple",
                                "t_f:p_f:perr_f:t_m:p_m:perr_m:t_b:p_b:perr_b");

  }

  void All01::analyze( const art::Event& event){

    auto const& kalSeeds = event.getProduct(_kalSeedsToken);
    _hNTracks->Fill( kalSeeds.size() );

    // Ntuple buffer;
    std::array<float,9> nt;

    // Loop over tracks in the event
    for ( auto const& ks : kalSeeds ) {

      // Require final fit to have converged.
      if ( ! ks.status().hasAllProperties( TrkFitFlag::kalmanConverged ) ){
        _hnSkip->Fill(0.);
        continue;
      }

      // Only consider tracks that have all 3 sets of intersection information.
      // The three locations are at the intersections of the trajectory with planes
      // perpendicular to the z axis and at the front, middle and back of the tracker.
      // Just because.  It's not a recommendation for analysis.
      std::vector<KalIntersection>::const_iterator front = ks.intersection( SurfaceIdEnum::TT_Front );
      auto mid  = ks.intersection( SurfaceIdEnum::TT_Mid );
      auto back = ks.intersection( SurfaceIdEnum::TT_Back );
      if ( front == ks.intersections().end() || mid == ks.intersections().end() || back == ks.intersections().end() ){
        _hnSkip->Fill(1.);
        continue;
      }

      // Apply fiducial time cut;
      if ( mid->time() < _tmin ){
        _hnSkip->Fill(2.);
        continue;
      }

      // Global properties
      _hnDOF->Fill( ks.nDOF() );
      _hHasCalo->Fill( ks.hasCaloCluster() );

      // Fill histograms at midpoint of the tracker.
      _ht0->Fill(  mid->time()   );
      _hp->Fill(   mid->mom()    );
      _hpErr->Fill(mid->momerr());

      nt[0] = front->time();
      nt[1] = front->mom();
      nt[2] = front->momerr();
      nt[3] = mid->time();
      nt[4] = mid->mom();
      nt[5] = mid->momerr();
      nt[6] = back->time();
      nt[7] = back->mom();
      nt[8] = back->momerr();
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

      }

    } // end loop over kalSeeds

  } // end analyze

} // end namespace mu2e

DEFINE_ART_MODULE(mu2e::All01)
