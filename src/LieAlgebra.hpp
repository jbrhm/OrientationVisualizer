#pragma once
#include "utils.hpp"
#include <Eigen/Core>
#include <math.h>

class LieAlgebra {
private:
  static constexpr double tolerance = 0.000000000001;

  // This is to prevent floating point errors
  static bool equals(double val1, double val2) {
    return abs(val1 - val2) < tolerance;
  }

  static double trace(const Eigen::Matrix3d &mat) {
    return mat.coeff(0, 0) + mat.coeff(1, 1) + mat.coeff(2, 2);
  }

public:
  static Eigen::Vector3d logarithmicMap(Eigen::Matrix3d SO3) {
    // Compute the magnitude of the mapped vector
    // This is a unique solution between the [0,pi]
    double theta = acos((trace(SO3) - 1) / 2);

    // This if statement prevents unguarded division for when sin(theta) = 0
    if (equals(theta, 0)) {
      // if theta = 0 we just produce the identity element
      return Eigen::Vector3d(0, 0, 0);
    } else if (equals(sin(theta), 0)) {
      // By looking at the off diagonal entries of the orthogonal matrix we can
      // determine what the signs of the entries should be
      double wx = getSign(SO3.coeff(1, 2)) * M_PI *
                  sqrt((1.0 / 2.0) * (SO3.coeff(0, 0) + 1));
      double wy = -1 * getSign(SO3.coeff(0, 2)) * M_PI *
                  sqrt((1.0 / 2.0) * (SO3.coeff(1, 1) + 1));
      double wz = -1 * getSign(SO3.coeff(1, 0)) * M_PI *
                  sqrt((1.0 / 2.0) * (SO3.coeff(2, 2) + 1));
      return Eigen::Vector3d(wx, wy, wz);
    } else {
      // Do the proper logarithmic operation
      Eigen::Matrix3d wx = (theta / (2 * sin(theta))) * (SO3 - SO3.transpose());
      return Eigen::Vector3d(wx.coeff(2, 1), wx.coeff(0, 2), wx.coeff(1, 0));
    }
  }
};
