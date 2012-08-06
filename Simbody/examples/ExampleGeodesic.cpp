/* -------------------------------------------------------------------------- *
 *                         Simbody(tm)  ExampleGeodesic                       *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK biosimulation toolkit originating from           *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org/home/simbody.  *
 *                                                                            *
 * Portions copyright (c) 2005-12 Stanford University and the Authors.        *
 * Authors: Ian Stavness, Michael Sherman, Andreas Scholz                     *
 * Contributors:                                                              *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

/**
 * This example demonstrates finding the geodesic between two points on
 * a ContactGeometry object.
 **/


#include "Simbody.h"

using namespace SimTK;
using std::cos;
using std::sin;
using std::cout;
using std::endl;

const Real vizInterval = 1/30.; // set to 1/30. to vizualize shooting


class VizPeriodicReporter : public PeriodicEventReporter {
public:
    VizPeriodicReporter(const Visualizer& viz, const State& dummyState, Real interval) :
        PeriodicEventReporter(interval), viz(viz), dummyState(dummyState) {
    }

    void handleEvent(const State& state) const {
        viz.report(dummyState);
    }

private:
    const Visualizer& viz;
    const State& dummyState;
};

int main() {
  try {

    // Test cylinder
    Real r			=  0.5;
    Real phiP		=  0.0*Pi;
    Real phiQ		=  1.0*Pi;
	Real heightP	= -0.5;
	Real heightQ	=  0.5;

    Vec3 P(r*cos(phiP), r*sin(phiP), heightP);
    Vec3 Q(r*cos(phiQ), r*sin(phiQ), heightQ);

	Vec3 O( r, -2,  heightP);
    Vec3 I(-r, -2,  heightQ);

    // move points off surface for testing
    // Q(0) -= r*0.2;
    // P(1) -= r*0.2;

    Vec3 r_OP = P - O;
    Vec3 r_IQ = Q - I;
    UnitVec3 e_OP(r_OP);
    UnitVec3 e_IQ(r_IQ);

    Vec3 r_PQ = Q - P;

    int n = 2; // problem size
    Vector x(n), dx(n), Fx(n), xold(n);
    Matrix J(n, n);

    //ContactGeometry::Sphere geom(r);
    ContactGeometry::Cylinder geom(r);

    bool inside; UnitVec3 normal;
    cout << "before P,Q=" << P << ", " << Q << " -- " 
         << geom.calcSurfaceValue(P) << " " << geom.calcSurfaceValue(Q) << endl;
    Vec3 newP = geom.findNearestPoint(P,inside,normal);
    UnitVec3 tP = normal.perp();
    Vec3 newQ = geom.findNearestPoint(Q,inside,normal);
    UnitVec3 tQ = normal.perp();
    cout << "after newP,Q=" << newP << ", " << newQ << " -- " 
         << geom.calcSurfaceValue(newP) 
         << " " << geom.calcSurfaceValue(newQ) << endl;

    cout << "curvature at newP along " << tP << ": " 
        << geom.calcSurfaceCurvatureInDirection(newP,tP) << "\n";
    cout << "curvature at newQ along " << tQ << ": " 
        << geom.calcSurfaceCurvatureInDirection(newQ,tQ) << "\n";

//    Vec3 radii(0.2,0.4,0.6);
//    ContactGeometry::Ellipsoid geom(radii);

    cout << "Gaussian curvature P,Q="
         << geom.calcGaussianCurvature(newP) << ","
         << geom.calcGaussianCurvature(newQ) << endl;

    Geodesic geod;

    // Create a dummy mb system for visualization
    MultibodySystem dummySystem;
    SimbodyMatterSubsystem matter(dummySystem);


//    matter.updGround().addBodyDecoration(Transform(), DecorativeEllipsoid(radii)
    matter.updGround().addBodyDecoration(Transform(), geom.createDecorativeGeometry()
            .setColor(Gray)
            .setOpacity(0.5)
            .setResolution(5));

    // Visualize with default options; ask for a report every 1/30 of a second
    // to match the Visualizer's default 30 frames per second rate.
    Visualizer viz(dummySystem);
    viz.setBackgroundType(Visualizer::SolidColor);

    // add vizualization callbacks for geodesics, contact points, etc.
    Vector tmp(6); // tmp = ~[P Q]
    tmp[0]=P[0]; tmp[1]=P[1]; tmp[2]=P[2]; tmp[3]=Q[0]; tmp[4]=Q[1]; tmp[5]=Q[2];
    viz.addDecorationGenerator(new PathDecorator(tmp, O, I, Green));
    viz.addDecorationGenerator(new PlaneDecorator(geom.getPlane(), Gray));
    viz.addDecorationGenerator(new GeodesicDecorator(geom.getGeodP(), Red));
    viz.addDecorationGenerator(new GeodesicDecorator(geom.getGeodQ(), Blue));
    viz.addDecorationGenerator(new GeodesicDecorator(geod, Orange));
    dummySystem.realizeTopology();
    State dummyState = dummySystem.getDefaultState();


    // calculate the geodesic
    geom.addVizReporter(new VizPeriodicReporter(viz, dummyState, vizInterval));
    viz.report(dummyState);

    const Real startReal = realTime(), startCpu = cpuTime();
    //geom.calcGeodesic(P, Q, e_OP, -e_IQ, geod);
    geom.calcGeodesicAnalytical(P, Q, e_OP, -e_IQ, geod);
    // geom.calcGeodesicUsingOrthogonalMethod(P, Q, geod);
    // geom.calcGeodesicUsingOrthogonalMethod(P, Q, e_OP, .5, geod);
    cout << "realTime=" << realTime()-startReal
         << " cpuTime=" << cpuTime()-startCpu << endl;

    const Array_<Transform>& frenet = geod.getFrenetFrames();
    const Array_<Real>& arcLength = geod.getArcLengths();
    const Array_<Vec2>& dirPtoQ = geod.getDirectionalSensitivityPtoQ();
    const Array_<Vec2>& dirQtoP = geod.getDirectionalSensitivityQtoP();
    for (int i=0; i < (int)dirPtoQ.size(); ++i) {
        cout << "\n" << arcLength[i] << ": " << dirPtoQ[i] << " " 
                                     << dirQtoP[i] << endl;
        cout << "p=" << frenet[i].p() << "\n";
        cout << "t=" << frenet[i].y() << "\n";
        cout << "b=" << frenet[i].x() << "\n";
        cout << "n=" << frenet[i].z() << "\n";
    }

//    geom.addVizReporter(new VizPeriodicReporter(viz, dummyState, 1/30.));
//    viz.report(dummyState);
//    GeodesicOptions opts;
//    geom.shootGeodesicInDirectionUntilLengthReached(P, UnitVec3(tP), 20, opts, geod);
//    geom.shootGeodesicInDirectionUntilPlaneHit(P, UnitVec3(tP), geom.getPlane(), opts, geod);

    viz.report(dummyState);
    cout << "geod shooting count = " << geom.getNumGeodesicsShot() << endl;
    cout << "num geod pts = " << geod.getFrenetFrames().size() << endl;


  } catch (const std::exception& e) {
    std::printf("EXCEPTION THROWN: %s\n", e.what());
    exit(1);

  } catch (...) {
    std::printf("UNKNOWN EXCEPTION THROWN\n");
    exit(1);
  }

    return 0;
}


