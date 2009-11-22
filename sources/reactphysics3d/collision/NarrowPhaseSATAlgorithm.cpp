/***************************************************************************
* Copyright (C) 2009      Daniel Chappuis                                  *
****************************************************************************
* This file is part of ReactPhysics3D.                                     *
*                                                                          *
* ReactPhysics3D is free software: you can redistribute it and/or modify   *
* it under the terms of the GNU Lesser General Public License as published *
* by the Free Software Foundation, either version 3 of the License, or     *
* (at your option) any later version.                                      *
*                                                                          *
* ReactPhysics3D is distributed in the hope that it will be useful,        *
* but WITHOUT ANY WARRANTY; without even the implied warranty of           *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
* GNU Lesser General Public License for more details.                      *
*                                                                          *
* You should have received a copy of the GNU Lesser General Public License *
* along with ReactPhysics3D. If not, see <http://www.gnu.org/licenses/>.   *
***************************************************************************/

// Libraries
#include "NarrowPhaseSATAlgorithm.h"
#include "ProjectionInterval.h"
#include "../body/OBB.h"
#include "../body/RigidBody.h"
#include "../constraint/Contact.h"
#include "../constraint/VertexVertexContact.h"
#include "../constraint/EdgeEdgeContact.h"
#include "../constraint/FaceFaceContact.h"
#include "../constraint/EdgeVertexContact.h"
#include "../constraint/FaceEdgeContact.h"
#include "../constraint/FaceVertexContact.h"
#include <cfloat>
#include <iostream> // TODO : Delete this
#include <cassert>

// We want to use the ReactPhysics3D namespace
using namespace reactphysics3d;

// TODO : Check and modify all comments on this file in order that
//        everything have to do with the new SAT algorithm

// Constructor
NarrowPhaseSATAlgorithm::NarrowPhaseSATAlgorithm() {

}

// Destructor
NarrowPhaseSATAlgorithm::~NarrowPhaseSATAlgorithm() {

}

// Return true and compute a collision contact if the two bounding volume collide.
// The method returns false if there is no collision between the two bounding volumes.
bool NarrowPhaseSATAlgorithm::testCollision(const BoundingVolume* const boundingVolume1, const BoundingVolume* const boundingVolume2, Contact** contact) {

    assert(boundingVolume1 != boundingVolume2);
    assert(*contact == 0);

    // If the two bounding volumes are OBB
    const OBB* const obb1 = dynamic_cast<const OBB* const>(boundingVolume1);
    const OBB* const obb2 = dynamic_cast<const OBB* const>(boundingVolume2);

    // If the two bounding volumes are OBB
    if (obb1 && obb2) {
        // Compute the collision test between two OBB
        return computeCollisionTest(obb1, obb2, contact);
    }
    else {
        return false;
    }
}


// This method returns true and computes a collision contact if the two OBB intersect.
// This method implements the separating algorithm between two OBB. The goal of this method is to test if the
// two OBBs intersect or not. If they intersect we report a collision contact and the method returns true. If
// they don't intersect, the method returns false. The separation axis that have to be tested for two
// OBB are the six face normals (3 for each OBB) and the nine vectors V = Ai x Bj where Ai is the ith face normal
// vector of OBB 1 and Bj is the jth face normal vector of OBB 2. We will use the notation Ai for the ith face
// normal of OBB 1 and Bj for the jth face normal of OBB 2.
bool NarrowPhaseSATAlgorithm::computeCollisionTest(const OBB* const obb1, const OBB* const obb2, Contact** contact) {

    double center;                              // Center of a projection interval
    double radius1;                             // Radius of projection interval [min1, max1]
    double radius2;                             // Radius of projection interval [min2, max2]
    double min1;                                // Minimum of interval 1
    double max1;                                // Maximum of interval 1
    double min2;                                // Minimm of interval 2
    double max2;                                // Maximum of interval 2
    Vector3D normal;                            // Contact normal (correspond to the separation axis with the smallest positive penetration depth)
                                                // 
    ContactType contactType;                    // Current contact type of the contact found so far
    double minPenetrationDepth = 0.0;           // Minimum penetration depth detected among all separated axis
    const double cutoff = 0.999999;             // Cutoff for cosine of angles between box axes
    bool existsParallelPair = false;            // True if there exists two face normals that are parallel.
                                                // This is used because if a parallel pair exists, it is sufficient
                                                // to test only the face normals of the OBBs for separation. Two nearly
                                                // parallel faces can lead to all face normal tests reporting no separation
                                                // along those directions. The cross product directions are tested next, but
                                                // Ai x Bj is nearly the zero vector and can cause a report that the two OBBs
                                                // are not intersecting when in fact they are.
    double c[3][3];                             // c[i][j] = DotProduct(obb1.Ai, obb2.Bj)
    double absC[3][3];                          // absC[i][j] = abs(DotProduct(obb1.Ai, obb2.Bj))
    double udc1[3];                             // DotProduct(obb1.Ai, obb2.center - obb1.center)
    double udc2[3];                             // DotProduct(obb2.Ai, obb2.center - obb1.center)

    Vector3D boxDistance = obb2->getCenter() - obb1->getCenter();   // Vector between the centers of the OBBs

    // Axis A0
    for (int i=0; i<3; ++i) {
        c[0][i] = obb1->getAxis(0).scalarProduct(obb2->getAxis(i));
        absC[0][i] = fabs(c[0][i]);
        if (absC[0][i] > cutoff) {
            existsParallelPair = true;
        }
    }
    udc1[0] = obb1->getAxis(0).scalarProduct(boxDistance);
    center = udc1[0];
    radius1 = obb1->getExtent(0);
    radius2 = obb2->getExtent(0)*absC[0][0] + obb2->getExtent(1)*absC[0][1] + obb2->getExtent(2) * absC[0][2];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    double penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // TODO : Delete this
    std::cout << "Speed : " << speed << std::endl;
    std::cout << "min1 : " << min1 << std::endl;
    std::cout << "max1 : " << max1 << std::endl;
    std::cout << "min2 : " << min2 << std::endl;
    std::cout << "max2 : " << max2 << std::endl;

    // Axis A1
    //std::cout << "----- AXIS A1 -----" << std::endl;
    for (int i=0; i<3; ++i) {
        c[1][i] = obb1->getAxis(1).scalarProduct(obb2->getAxis(i));
        absC[1][i] = fabs(c[1][i]);
        if (absC[1][i] > cutoff) {
            existsParallelPair = true;
        }
    }
    udc1[1] = obb1->getAxis(1).scalarProduct(boxDistance);
    center = udc1[1];
    radius1 = obb1->getExtent(1);
    radius2 = obb2->getExtent(0)*absC[1][0] + obb2->getExtent(1)*absC[1][1] + obb2->getExtent(2) * absC[1][2];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // TODO : Delete this
    std::cout << "speed : " << speed << std::endl;
    std::cout << "min1 : " << min1 << std::endl;
    std::cout << "max1 : " << max1 << std::endl;
    std::cout << "min2 : " << min2 << std::endl;
    std::cout << "max2 : " << max2 << std::endl;

    // Axis A2
    for (int i=0; i<3; ++i) {
        c[2][i] = obb1->getAxis(2).scalarProduct(obb2->getAxis(i));
        absC[2][i] = fabs(c[2][i]);
        if (absC[2][i] > cutoff) {
            existsParallelPair = true;
        }
    }
    udc1[2] = obb1->getAxis(2).scalarProduct(boxDistance);
    center = udc1[2];
    radius1 = obb1->getExtent(2);
    radius2 = obb2->getExtent(0)*absC[2][0] + obb2->getExtent(1)*absC[2][1] + obb2->getExtent(2)*absC[2][2];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // TODO : Delete this
    std::cout << "Speed : " << speed << std::endl;
    std::cout << "min1 : " << min1 << std::endl;
    std::cout << "max1 : " << max1 << std::endl;
    std::cout << "min2 : " << min2 << std::endl;
    std::cout << "max2 : " << max2 << std::endl;

    // Axis B0
    udc2[0] = obb2->getAxis(0).scalarProduct(boxDistance);
    center = udc2[0];
    radius1 = obb1->getExtent(0)*absC[0][0] + obb1->getExtent(1)*absC[1][0] + obb1->getExtent(2) * absC[2][0];
    radius2 = obb2->getExtent(0);
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // Axis B1
    //std::cout << "----- AXIS B1 -----" << std::endl;
    udc2[1] = obb2->getAxis(1).scalarProduct(boxDistance);
    center = udc2[1];
    radius1 = obb1->getExtent(0)*absC[0][1] + obb1->getExtent(1)*absC[1][1] + obb1->getExtent(2) * absC[2][1];
    radius2 = obb2->getExtent(1);
    min1 = - radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    std::cout << "Speed : " << speed << std::endl;
    std::cout << "min1 : " << min1 << std::endl;
    std::cout << "max1 : " << max1 << std::endl;
    std::cout << "min2 : " << min2 << std::endl;
    std::cout << "max2 : " << max2 << std::endl;
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }


    // Axis B2
    udc2[2] = obb2->getAxis(2).scalarProduct(boxDistance);
    center = udc2[2];
    radius1 = obb1->getExtent(0)*absC[0][2] + obb1->getExtent(1)*absC[1][2] + obb1->getExtent(2)*absC[2][2];
    radius2 = obb2->getExtent(2);
    min1 = - radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }


    // If there exists a parallel pair of face normals
    if (existsParallelPair) {
        // There exists a parallel pair of face normals and we have already checked all the face
        // normals for separation. Therefore the OBBs must intersect
        //std::cout << "PARALLEL PAIR" << std::endl;
        //std::cout << "Current -- 1 -- MIN Points : " << currentInterval1.getMinProjectedPoints().size() << " MAX : " << currentInterval1.getMaxProjectedPoints().size() << std::endl;
        //std::cout << "Current -- 1 -- min : " << currentInterval1.getMin() << std::endl;
        //std::cout << "Timefirst : " << timeFirst.getValue() << std::endl;
        std::cout << "CONTACT FOUND AND TIMEFIRST IS " << timeFirst.getValue() << std::endl;

        // TODO : The normal has to be unit before passing it to the computeContact() method
        computeContact(currentInterval1, currentInterval2, velocity1, velocity2, timeFirst, side, contact);

        //std::cout << "Contact 1 : " << contact << std::endl;
        assert(*contact != 0);
        return true;
    }

    // Axis A0 x B0
    center = udc1[2] * c[1][0] - udc1[1] * c[2][0];
    radius1 = obb1->getExtent(1) * absC[2][0] + obb1->getExtent(2) * absC[1][0];
    radius2 = obb2->getExtent(1) * absC[0][2] + obb2->getExtent(2) * absC[0][1];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    //Vector3D axis = obb1->getAxis(0).crossProduct(obb2->getAxis(0));
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // Axis A0 x B1
    center = udc1[2] * c[1][1] - udc1[1] * c[2][1];
    radius1 = obb1->getExtent(1) * absC[2][1] + obb1->getExtent(2) * absC[1][1];
    radius2 = obb2->getExtent(0) * absC[0][2] + obb2->getExtent(2) * absC[0][0];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    //axis = obb1->getAxis(0).crossProduct(obb2->getAxis(1));
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // Axis A0 x B2
    center = udc1[2] * c[1][2] - udc1[1] * c[2][2];
    radius1 = obb1->getExtent(1) * absC[2][2] + obb1->getExtent(2) * absC[1][2];
    radius2 = obb2->getExtent(0) * absC[0][1] + obb2->getExtent(1) * absC[0][0];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    //axis = obb1->getAxis(0).crossProduct(obb2->getAxis(2));
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // Axis A1 x B0
    center = udc1[0] * c[2][0] - udc1[2] * c[0][0];
    radius1 = obb1->getExtent(0) * absC[2][0] + obb1->getExtent(2) * absC[0][0];
    radius2 = obb2->getExtent(1) * absC[1][2] + obb2->getExtent(2) * absC[1][1];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    //axis = obb1->getAxis(1).crossProduct(obb2->getAxis(0));
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // Axis A1 x B1
    center = udc1[0] * c[2][1] - udc1[2] * c[0][1];
    radius1 = obb1->getExtent(0) * absC[2][1] + obb1->getExtent(2) * absC[0][1];
    radius2 = obb2->getExtent(0) * absC[1][2] + obb2->getExtent(2) * absC[1][0];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    //axis = obb1->getAxis(1).crossProduct(obb2->getAxis(1));
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // Axis A1 x B2
    center = udc1[0] * c[2][2] - udc1[2] * c[0][2];
    radius1 = obb1->getExtent(0) * absC[2][2] + obb1->getExtent(2) * absC[0][2];
    radius2 = obb2->getExtent(0) * absC[1][1] + obb2->getExtent(1) * absC[1][0];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    //axis = obb1->getAxis(1).crossProduct(obb2->getAxis(2));
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // Axis A2 x B0
    center = udc1[1] * c[0][0] - udc1[0] * c[1][0];
    radius1 = obb1->getExtent(0) * absC[1][0] + obb1->getExtent(1) * absC[0][0];
    radius2 = obb2->getExtent(1) * absC[2][2] + obb2->getExtent(2) * absC[2][1];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    //axis = obb1->getAxis(2).crossProduct(obb2->getAxis(0));
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // Axis A2 x B1
    center = udc1[1] * c[0][1] - udc1[0] * c[1][1];
    radius1 = obb1->getExtent(0) * absC[1][1] + obb1->getExtent(1) * absC[0][1];
    radius2 = obb2->getExtent(0) * absC[2][2] + obb2->getExtent(2) * absC[2][0];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    //axis = obb1->getAxis(2).crossProduct(obb2->getAxis(1));
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // Axis A2 x B2
    center = udc1[1] * c[0][2] - udc1[0] * c[1][2];
    radius1 = obb1->getExtent(0) * absC[1][2] + obb1->getExtent(1) * absC[0][2];
    radius2 = obb2->getExtent(0) * absC[2][1] + obb2->getExtent(1) * absC[2][0];
    min1 = -radius1;
    max1 = radius1;
    min2 = center - radius2;
    max2 = center + radius2;
    //axis = obb1->getAxis(2).crossProduct(obb2->getAxis(2));
    penetrationDepth = computePenetrationDepth(min1, max1, min2, max2, minPenetrationDepth);
    if (penetrationDepth < 0) { // We have found a separation axis, therefore the two OBBs don't collide
        return false;
    }

    // TODO : Delete this
    //(*contact) = new Contact(obb1->getBodyPointer(), obb2->getBodyPointer(), Vector3D(1,0,0), timeFirst);
    std::cout << "Contact2 : " << contact << std::endl;
    std::cout << "CONTACT FOUND AND TIMEFIRST IS " << timeFirst.getValue() << std::endl;

    // Compute the collision contact
    // TODO : The normal has to be unit before passing it to the computeContact() method
    computeContact(contact);

    // We have found no separation axis, therefore the two OBBs must collide
    assert(*contact != 0);

    return true;
}

// This method computes penetration depth between two intervals and update the minimum penetration
// depth found so far if the computed penetration depth is positive (the penetration depth in case
// of collision has to be positive) and smaller than the current minimum penetration
// depth. This method returns the computed penetration depth (note that it could return a negative
// penetration depth if the intervals are separated.
double NarrowPhaseSATAlgorithm::computePenetrationDepth(double min1, double max1, double min2, double max2, double& minPenetrationDepth) {

    // Compute the length of both intervals
    double lengthInterval1 = max1 - min1;
    double lengthInterval2 = max2 - min2;

    // Compute the total length of both intervals
    double minExtreme = min(min1, min2);
    double maxExtreme = max(max1, max2);
    double lengthBothIntervals = maxExtreme - minExtreme;

    // Compute the current penetration depth
    double penetrationDepth = (lengthInterval1 + lengthInterval2) - lengthBothIntervals;

    // If the current penetration depth is smaller than the minimum penetration depth
    if (penetrationDepth < minPenetrationDepth && penetrationDepth >= 0) {
        minPenetrationDepth = penetrationDepth;
        side = currentSide;
    }

    // Return the computed penetration depth
    return penetrationDepth;
}

// Compute a new collision contact between two projection intervals.
// Warning : If the side value is true the max of interval1 collides with the min of interval2. If the
// side value is false the max value of interval2 collides with the min value of interval1.
void NarrowPhaseSATAlgorithm::computeContact(const ProjectionInterval& interval1, const ProjectionInterval& interval2,
                                             bool side, Contact** contact) {

    assert(*contact == 0);

    ProjectionInterval intervalLeft = (side) ? interval1 : interval2;
    ProjectionInterval intervalRight = (!side) ? interval2 : interval1;

    //Vector3D velocityLeft = (side) ? velocity1 : velocity2;
    //Vector3D velocityRight = (!side) ? velocity2 : velocity1;

    // Compute the extreme points of the two intervals at the instant of contact
    //std::vector<Vector3D> leftExtremePointsAtContact = movePoints(intervalLeft.getMaxProjectedPoints(), velocityLeft * time.getValue());
    //std::vector<Vector3D> rightExtremePointsAtContact = movePoints(intervalRight.getMinProjectedPoints(), velocityRight * time.getValue());

    // TODO : ADD THE BODY ADRESS INTO THE CONTACT HERE
    // Get the rigid bodies
    //RigidBody* body1 = dynamic_cast<RigidBody*>(intervalLeft.getBoundingVolumePointer()->getBodyPointer());
    //RigidBody* body2 = dynamic_cast<RigidBody*>(intervalRight.getBoundingVolumePointer()->getBodyPointer());

    //assert(body1 != 0 && body2 != 0);
    RigidBody* body1 = 0;
    RigidBody* body2 = 0; // TODO : DELETE THIS

    // Compute the normal vector of the contact
    // TODO : Compute the normal vector of the contact
    Vector3D normalVector(0.0, 1.0, 0.0);

    /*
    switch(intervalLeft.getMaxType()) {
        case VERTEX :   if (intervalRight.getMinType() == VERTEX) {
                            // Construct a new Vertex-Vertex contact
                            *contact = new VertexVertexContact(body1, body2, normalVector, time, intervalLeft.getMaxProjectedPoints()[0]);
                        }
                        else if (intervalRight.getMinType() == EDGE) {
                           // Construct a new Edge-Vertex contact
                           *contact = new EdgeVertexContact(body1, body2, normalVector, time, intervalLeft.getMaxProjectedPoints()[0]);
                        }
                        else if (intervalRight.getMinType() == FACE) {
                            // Construct a new Face-Vertex contact
                           *contact = new FaceVertexContact(body1, body2, normalVector, time, intervalLeft.getMaxProjectedPoints()[0]);
                        }
                        break;

        case EDGE:      if (intervalRight.getMinType() == VERTEX) {
                            // Construct a new Edge-Vertex contact
                           *contact = new EdgeVertexContact(body1, body2, normalVector, time, intervalRight.getMinProjectedPoints()[0]);
                        }
                        else if (intervalRight.getMinType() == EDGE) {
                           // Compute the intersection between the two edges
                           Segment3D edge1(intervalLeft.getMaxProjectedPoints()[0], intervalLeft.getMaxProjectedPoints()[1]);
                           Segment3D edge2(intervalRight.getMinProjectedPoints()[0], intervalRight.getMinProjectedPoints()[1]);
                           Segment3D intersectionSegment = computeSegmentSegmentIntersection(edge1, edge2);

                           // Construct a new Edge-Edge contact
                           *contact = new EdgeEdgeContact(body1, body2, normalVector, time, intersectionSegment);
                        }
                        else if (intervalRight.getMinType() == FACE) {
                            // Compute the intersection between the edge and the face
                            Segment3D edge(intervalLeft.getMaxProjectedPoints()[0], intervalLeft.getMaxProjectedPoints()[1]);
                            Polygon3D face(intervalRight.getMinProjectedPoints());
                            Segment3D intersectionSegment = computeSegmentPolygonIntersection(edge, face);

                            // TODO : Warning : At this moment the set of vertices of the contact is not sorted. We will have to
                            // find a way to sort it because the constructor of the Polygon3D class needs a set where vertices are
                            // sorted in order to have a correct polygon.

                            // Construct a new Face-Edge contact
                            *contact = new FaceEdgeContact(body1, body2, normalVector, time, intersectionSegment);
                        }
                        break;

        case FACE:      if (intervalRight.getMinType() == VERTEX) {
                            // Construct a new Face-Vertex contact
                            *contact = new FaceVertexContact(body1, body2, normalVector, time, intervalRight.getMinProjectedPoints()[0]);
                        }
                        else if (intervalRight.getMinType() == EDGE) {
                            // Compute the intersection between the edge and the face
                            Polygon3D face(intervalLeft.getMaxProjectedPoints());
                            Segment3D edge(intervalRight.getMinProjectedPoints()[0], intervalRight.getMinProjectedPoints()[1]);
                            Segment3D intersectionSegment = computeSegmentPolygonIntersection(edge, face);

                            // TODO : Warning : At this moment the set of vertices of the contact is not sorted. We will have to
                            // find a way to sort it because the constructor of the Polygon3D class needs a set where vertices are
                            // sorted in order to have a correct polygon.

                            // TODO : Here we will have to compute the Segment intersection between the edge and the face
                            *contact = new FaceEdgeContact(body1, body2, normalVector, time, intersectionSegment);
                        }
                        else if (intervalRight.getMinType() == FACE) {
                            // Compute the intersection between the two faces
                            Polygon3D face1(intervalLeft.getMaxProjectedPoints());
                            Polygon3D face2(intervalRight.getMinProjectedPoints());
                            Polygon3D intersectionPolygon = computePolygonPolygonIntersection(face1, face2);

                            // TODO : Warning : At this moment the set of vertices of the contact is not sorted. We will have to
                            // find a way to sort it because the constructor of the Polygon3D class needs a set where vertices are
                            // sorted in order to have a correct polygon.

                            // Construct a new Face-Face contact
                            *contact = new FaceFaceContact(body1, body2, normalVector, time, intersectionPolygon);
                        }
                        break;
    }
    */
}