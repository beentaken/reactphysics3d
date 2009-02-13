/****************************************************************************
 * Copyright (C) 2009      Daniel Chappuis                                  *
 ****************************************************************************
 * This file is part of ReactPhysics3D.                                     *
 *                                                                          *
 * ReactPhysics3D is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation, either version 3 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * ReactPhysics3D is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with ReactPhysics3D. If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// Libraries
#include "Quaternion.h"
#include "constants.h"
#include <cassert>

// Namespaces
using namespace reactphysics3d;

// Constructor of the class
Quaternion::Quaternion()
           :x(0.0), y(0.0), z(0.0), w(0.0) {

}

// Constructor with arguments
Quaternion::Quaternion(double x, double y, double z, double w)
           :x(x), y(y), z(z), w(w) {

}

// Constructor with the component w and the vector v=(x y z)
Quaternion::Quaternion(double w, const Vector3D& v)
           :x(v.getX()), y(v.getY()), z(v.getZ()), w(w) {

}

// Copy-constructor
Quaternion::Quaternion(const Quaternion& quaternion)
           :x(quaternion.x), y(quaternion.y), z(quaternion.z), w(quaternion.w) {

}

// Destructor
Quaternion::~Quaternion() {

}

// Compute the rotation angle (in radians) and the 3D rotation axis
// This method is used to get the rotation angle and axis of an
// orientation quaternion.
// TODO : Test this method
void Quaternion::getRotationAngleAxis(double& angle, Vector3D& axis) const {
    Quaternion quaternion;

    // If the quaternion is unit
    if (length() == 1.0) {
        quaternion = *this;
    }
    else {
        // We compute the unit quaternion
        quaternion = getUnit();
    }

    // Compute the roation angle
    angle = acos(quaternion.w) * 2.0;

    // Compute the 3D rotation axis
    Vector3D rotationAxis(quaternion.x, quaternion.y, quaternion.z);

    // Normalize the rotation axis
    rotationAxis = rotationAxis.getUnit();

    // Set the rotation axis values
    axis.setAllValues(rotationAxis.getX(), rotationAxis.getY(), rotationAxis.getZ());
}

// Compute the spherical linear interpolation between two quaternions.
// The t argument has to be such that 0 <= t <= 1.
// TODO : Test this method
Quaternion Quaternion::slerp(const Quaternion& quaternion1, const Quaternion& quaternion2, double t) {
    assert(t >= 0 && t <= 1);

    double invert = 1;

    // Compute cos(theta) using the quaternion scalar product
    double cosineTheta = quaternion1.scalarProduct(quaternion2);

    // Take care of the sign of cosineTheta
    if (cosineTheta < 0) {
			cosineTheta = -cosineTheta;
			invert = -1;
    }

    // Because of precision, if cos(theta) is nearly 1, therefore theta is nearly 0 and we can write
    // sin((1-t)*theta) as (1-t) and sin(t*theta) as t
    if ((1.0-cosineTheta) < epsilon) {
        return quaternion1 * (1-t) + quaternion2 * (t * invert);
    }

    // Compute the theta angle
    double theta = acos(cosineTheta);

    // Compute sin(theta)
    double sineTheta = sin(theta);

    // Compute the two coefficients that are in the spherical linear interpolation formula
    double coeff1 = sin((1-t)*theta) / sineTheta;
    double coeff2 = sin(t*theta) / sineTheta * invert;

    // Compute and return the interpolated quaternion
    return quaternion1 * coeff1 + quaternion2 * coeff2;
}


