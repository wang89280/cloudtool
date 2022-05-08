/**
 * @file cloud.cpp
 * @author hjm (hjmalex@163.com)
 * @version 1.0
 * @date 2022-05-08
 *
 * @copyright Copyright (c) 2022 hjm
 *
 */

#include "base/cloud.h"

#include <pcl/common/common.h>
#include <pcl/common/transforms.h>
#include <pcl/search/kdtree.h>

#include "base/common.h"

namespace ct {

void Cloud::setCloudColor(int r, int g, int b) {
  std::uint32_t rgb =
      ((std::uint32_t)r << 16 | (std::uint32_t)g << 8 | (std::uint32_t)b);
  for (auto& i : points) i.rgb = *reinterpret_cast<float*>(&rgb);
}

void Cloud::setCloudColor(const QString& aixs) {
  float max = -FLT_MAX, min = FLT_MAX;
  float fRed = 0.f, fGreen = 0.f, fBlue = 0.f;
  constexpr float range = 2.f / 3.f;
  constexpr uint8_t colorMax = std::numeric_limits<uint8_t>::max();
  PointXYZRGBN min_pt, max_pt;
  pcl::getMinMax3D(*this, min_pt, max_pt);
  if (aixs == "x") {
    max = max_pt.x;
    min = min_pt.x;
    for (auto& i : points) {
      float hue = (max - i.x) / static_cast<float>(max - min);
      hue *= range;
      hue = range - hue;
      HSVtoRGB(hue, 1.f, 1.f, fRed, fGreen, fBlue);
      i.r = static_cast<uint8_t>(fRed * colorMax);
      i.g = static_cast<uint8_t>(fGreen * colorMax);
      i.b = static_cast<uint8_t>(fBlue * colorMax);
    }
  } else if (aixs == "y") {
    max = max_pt.y;
    min = min_pt.y;
    for (auto& i : points) {
      float hue = (max - i.y) / static_cast<float>(max - min);
      hue *= range;
      hue = range - hue;
      HSVtoRGB(hue, 1.f, 1.f, fRed, fGreen, fBlue);
      i.r = static_cast<uint8_t>(fRed * colorMax);
      i.g = static_cast<uint8_t>(fGreen * colorMax);
      i.b = static_cast<uint8_t>(fBlue * colorMax);
    }
  } else if (aixs == "z") {
    max = max_pt.z;
    min = min_pt.z;
    for (auto& i : points) {
      float hue = (max - i.z) / static_cast<float>(max - min);
      hue *= range;
      hue = range - hue;
      HSVtoRGB(hue, 1.f, 1.f, fRed, fGreen, fBlue);
      i.r = static_cast<uint8_t>(fRed * colorMax);
      i.g = static_cast<uint8_t>(fGreen * colorMax);
      i.b = static_cast<uint8_t>(fBlue * colorMax);
    }
  }
}

void Cloud::scale(double x, double y, double z, bool origin) {
  for (int j = 0; j < points.size(); j++) {
    points[j].x = box_.translation[0] + x * (points[j].x - box_.translation[0]);
    points[j].y = box_.translation[1] + y * (points[j].y - box_.translation[1]);
    points[j].z = box_.translation[2] + z * (points[j].z - box_.translation[2]);
  }
  if (origin) {
    Eigen::Affine3f trans;
    pcl::getTransformation(box_.translation[0] * x - box_.translation[0],
                           box_.translation[1] * y - box_.translation[1],
                           box_.translation[2] * z - box_.translation[2], 0, 0,
                           0, trans);
    pcl::transformPointCloud(*this, *this, trans);
  }
}

void Cloud::update(bool compute_resolution) {
  // update type
  if (points[rand() % size()].normal_x == 0.0f &&
      points[rand() % size()].normal_y == 0.0f &&
      points[rand() % size()].normal_z == 0.0f) {
    has_normals_ = false;
    if (points[rand() % size()].r == 0 && points[rand() % size()].g == 0 &&
        points[rand() % size()].b == 0)
      type_ = "XYZ";
    else
      type_ = "XYZRGB";
  } else {
    has_normals_ = true;
    if (points[rand() % size()].r == 0 && points[rand() % size()].g == 0 &&
        points[rand() % size()].b == 0)
      type_ = "XYZNormal";
    else
      type_ = "XYZRGBNormal";
  }
  // update box
  PointXYZRGBN min, max;
  pcl::getMinMax3D(*this, min, max);
  Eigen::Vector3f center = 0.5f * (min.getVector3fMap() + max.getVector3fMap());
  Eigen::Vector3f whd = max.getVector3fMap() - min.getVector3fMap();
  Eigen::Affine3f affine =
      pcl::getTransformation(center[0], center[1], center[2], 0, 0, 0);
  box_ = {whd(0), whd(1), whd(2),
          affine, center, Eigen::Quaternionf(Eigen::Matrix3f::Identity())};
  // update resolution
  if (compute_resolution) {
    int n_points = 0, nres, size_cloud = size();
    std::vector<int> indices(2);
    std::vector<float> sqr_distances(2);
    pcl::search::KdTree<PointXYZRGBN>::Ptr tree(
        new pcl::search::KdTree<PointXYZRGBN>);
    tree->setInputCloud(this->makeShared());
    for (std::size_t i = 0; i < size_cloud; i++) {
      if (!std::isfinite(points[i].x)) {
        continue;
      }
      nres = tree->nearestKSearch(i, 2, indices, sqr_distances);
      if (nres == 2) {
        resolution_ += sqrt(sqr_distances[1]);
        ++n_points;
      }
    }
    if (n_points != 0) {
      resolution_ /= n_points;
    }
  }
}

}  // namespace ct