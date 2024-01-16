//
//  Explore the nominal geometry of the tracker.
//
//    - Mu2e-doc-888  - describes the tracker geometry
//                    - the representation in Mu2e Offline is based on this
//    - Mu2e-doc-1120 - "Units and Coordinate Systems Used by Mu2e Offline"
//                    - The representation in Mu2e Offline is in the Detector
//                      coordinate system, section 2.2
//
//  Original author Rob Kutschke
//

#include "Offline/DataProducts//inc/StrawId.hh"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/TrackerGeom/inc/Tracker.hh"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"

#include <format>
#include <iomanip>
#include <iostream>
using std::cout;
using std::endl;
using std::setw;

namespace mu2e {

  class NominalTrackerGeom : public art::EDAnalyzer {

  public:
    struct Config {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;
      //fhicl::Atom<int> verbosity{Name("verbosity"), Comment("Control verbosty of printout.")};
    };
    typedef art::EDAnalyzer::Table<Config> Parameters;

    explicit NominalTrackerGeom(const Parameters& conf);

    void beginRun( const art::Run&   Run  ) override;
    void analyze(  const art::Event& event) override;

  private:

    //int verbosity_;

  };

  NominalTrackerGeom::NominalTrackerGeom(const Parameters& conf)
    : art::EDAnalyzer(conf)
      //, verbosity_(conf().verbosity())
  {
  }

  void NominalTrackerGeom::beginRun( const art::Run& Run ) {

    // The nominal geometry is avaiable at beginRun-time and later.
    Tracker const& trk = *GeomHandle<Tracker>();

    cout << "Tracker Geometry:  " << endl;
    cout << "Number of planes:  " << trk.nPlanes() << endl;
    cout << "Number of panels:  " << trk.nPanels() << endl;
    cout << "Number of straws:  " << trk.nStraws() << endl;

    // Distance along the wire from the wire center.
    float const offset{0.};

    // Loop over planes, panels within a plane and straws within a panel.
    unsigned n{0};
    for ( uint16_t ipln = 0; ipln < StrawId::_nplanes; ipln++  ){
      for ( uint16_t ipan = 0; ipan < StrawId::_npanels; ipan++ ){
        for ( uint16_t istr = 0; istr < StrawId::_nstraws; istr++ ){

          // StrawId is the identifier of a straw within Offline.
          // Digis, hits etc are identified by a StrawId.
          StrawId sid( ipln, ipan, istr);

          // Get properties of this straw
          Straw const& straw = trk.getStraw(sid);

          // Columns of the printout are:
          //  1 "Straw"
          //  2 Ordinal number  0 ... number of straws-1
          //  3 StrawId printed as a base 10 integer
          //  4 StrawId printed as a base 16 integer
          //  5 StrawId printed as a string
          //  6 plane number of tihs straw
          //  7 panel number within this plane of tihs straw
          //  8 straw number within this panel
          //  9 plane number of tihs straw
          // 10 Half length of this straw, in mm
          // 11 Radius from the centerline of the tracker of the midpoint of this straw, in mm
          // 12 3-position (x,y,z) of the midpoint of the wire wire in Detector Coordinates, in mm
          // 13 3-direction (x,y,z) of the wire, at the midpoint of the wire, in Detector Coordinates, in mm
          cout << "Straw: "
               << setw(6) << n++
               << setw(6) << sid.asUint16()
               << std::format(" {:8x}", sid.asUint16() )
               << setw(10) << sid
               << setw(3)  << sid.plane()
               << setw(2)  << sid.panel()
               << setw(3)  << sid.straw()
               << std::format(" {:8.3f}", straw.halfLength() )
               << std::format(" {:8.3f}", straw.strawPosition(offset).perp() )
               << " " << straw.strawPosition(offset)
               << " " << straw.strawDirection(offset)
               << endl;
        }
      }
    }

  }

  void NominalTrackerGeom::analyze( const art::Event& event){}

} // end namespace mu2e

DEFINE_ART_MODULE(mu2e::NominalTrackerGeom)
