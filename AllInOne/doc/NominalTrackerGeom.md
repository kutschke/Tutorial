# Nominal Tracker Geometry

## Introduction

This example illustrates how to access information about the nominal, ie as designed, tracker geometry;
it prints out a summary table with information about each straw.
An exmaple, in preparation, will show how to access information about the "as aligned" tracker,
that describes out best knowledge about where all of the wires are located in space.

The code for this example is located in [Tutorial/src/NominalTrackerGeom_module.cc](../AllInOne/src/NominalTrackerGeom.cc)
and the fcl to run it is found in [Tutorial/fcl/nominalTrackerGeom..cc](../AllInOne/fcl/nominalTrackerGeom.cc)
When you build the [AllInOne](AllInOne.md) example, it build the code fort this example too,

To run this example
1. cd to your Tutorial working area and create the Mu2e interactive environment ( setup mu2e and muse setup ).
2. ``mu2e -c -c Tutorial/AllInOne/fcl/nominalTrackerGeom.fcl  >& out/nominalTrackerGeom.log``
3. Inspect the output file to see the printed table.  Look at the source code to understand the content of the table.


## Ideas

This example introduces several new ideas

1. GeometryService.  This is not explicitly seen in this example but it's there behing the scenes.
   This is an example of an art service ( you don't need to, but if you want to learn
   more about art services see [[https://mu2ewiki.fnal.gov/wiki/Services#Mu2e_Supplied_Services]] for a general description of art Services. )
   It holds many objects each of which describes one element of the Mu2e Geometry, such as the tracker, calorimeter, CRV etc.
   All elements needed for reconstruction and analysis are present in the Geometry Service; for these geometry elements, the Geant4 geometry
   is derived from the objects inthe GeometryService. Some elements of the geometry that are not used in reconstruction and analysis are not
   present in the GeometryService; exmaples are many details of the Mu2e hall; these details are only present in our Geant4 geometry.
1. The GeometryService holds an object of type Tracker that describes the tracker.  The code for this class is found
   in Offline/TrackerGeom/inc/Tracker.hh and Offline/TrackerGeom/src/Tracker.cc .  This object contains objects of type
   Plane, Panel, Straw that describes the substructure of the tracker.  You can also find the source code for those objects under
   Offline/TrackerGeom.
1. The GeometryService initializes its geometry at beginRun-time.  So it is not avaialble at module construction time or at beginJob time.
1. Within the Mu2e offline software, each straw in the tracker is uniquely identified using an object of type StrawId.
   See Offline/DataProducts/inc/StrawId.hh and Offline/DataProducts/src/StrawId.cc.  Objects such as
   digis and hits are labeled by a StrawId.  Note that this is not a dense identifier!  You will see what this means in the example.
1. The preferred idiom for accessing the tracker is:
   ``Tracker const& trk = *GeomHandle<Tracker>();``
   Note the reference (ampersand) on the left hand side; if you omit it, the code still run but it will make a copy of the tracker object,
   which wastes time and memory.` Watch for this when you use Geomhandles.
1. When you instantiate a GeomHandle<T>, it contacts the GeometryService and asks the service if it has an object of type T.
   If it does, the the GeomHandle will behave like a pointer to that object.  If it does not, then the constructor of
   the GeomHandle will throw an exception and art will shutdown gracefully.
1. In the preferred idiom, we create the GeomHandle and immediately turn it into a const& to the Tracker.
1. The next few lines of code in the tracker print some information about the tracker.
1. THe main body of the example is a triply nested to loop that accesses each straw and prints information about it.
   The idiom to get the information about the properties of one straw is:
   ``Straw const& straw = trk.getStraw(sid);``
   Again the amperand on the left hand side is importnat.
1. Look at the printout that was generated in the log file and look at the code to understand each column.  You will
   see that there are many differnet ways to printout a StrawId.
1. Now look at the table at ordinal numbers around 96.  You will see that the ordinal numbers are continuous, by construction, but that
   the integer base 10 representation of the StrawId jumps from 95 to 128.  This is because a StrawId is a packed bit field,
   which you can see by looking at it's source code.  The jump is from the last straw in (plane 0, panel 0) to the first straw
   in (plane 0, panel 1).  This jump is what we mean when we say that StrawID is not a dense identifier.
1. By convention, the StrawId of straw 0 in any panel, serves as the PanelId for that panel and the StrawId of straw 0 or panel 0
   in any plane serves as the PlaneId for that plane.
1. By convention the 96 straws in a panel are numbered from 0, at the lowest radius of the straw midpoint, to 95 at the largest radius
   of the straw midpoint, increasing monotonically outward.
1. In early versions of the code there was a concept of layers within
   a panel but this is not longer used in any modern code; the concept of layer is represented in StrawId in case people find it
   useful.



