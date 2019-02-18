/*
 * BoundaryFeature.cpp - GUI-side descriptions of boundary features
 *
 * (c)2017-9 Applied Scientific Research, Inc.
 *           Written by Mark J Stock <markjstock@gmail.com>
 */

#include "BoundaryFeature.h"

#include <cmath>
#include <iostream>
#include <sstream>

// write out any object of parent type BoundaryFeature by dispatching to appropriate "debug" method
std::ostream& operator<<(std::ostream& os, BoundaryFeature const& ff) { 
  ff.debug(os);
  return os;
}


//
// Create a circle (fluid is outside circle)
//
ElementPacket<float>
SolidCircle::init_elements(const float _ips) const {

  // how many panels?
  const size_t num_panels = std::min(10000, std::max(5, (int)(m_diam * M_PI / _ips)));

  std::cout << "Creating circle with " << num_panels << " panels" << std::endl;

  // created once
  std::vector<float>   x(num_panels*2);
  std::vector<Int>   idx(num_panels*2);
  std::vector<float> val(num_panels);

  // outside is to the left walking from one point to the next
  // so go CW around the circle starting at theta=0 (+x axis)
  for (size_t i=0; i<num_panels; i++) {
    x[2*i]     = m_x + 0.5*m_diam * std::cos(2.0 * M_PI * (float)i / (float)num_panels);
    x[2*i+1]   = m_y - 0.5*m_diam * std::sin(2.0 * M_PI * (float)i / (float)num_panels);
    idx[2*i]   = i;
    idx[2*i+1] = i+1;
    val[i]     = 0.0;
  }

  // correct the final index
  idx[2*num_panels-1] = 0;

  return ElementPacket<float>({x, idx, val});
}

void
SolidCircle::debug(std::ostream& os) const {
  os << to_string();
}

std::string
SolidCircle::to_string() const {
  std::stringstream ss;
  ss << "solid circle at " << m_x << " " << m_y << " with diameter " << m_diam;
  return ss.str();
}

nlohmann::json
SolidCircle::to_json() const {
  // make an object for the mesh
  nlohmann::json mesh = nlohmann::json::object();
  mesh["geometry"] = "circle";
  mesh["translation"] = {m_x, m_y};
  mesh["scale"] = m_diam;
  return mesh;
}


//
// Create an oval (fluid is outside circle)
//
ElementPacket<float>
SolidOval::init_elements(const float _ips) const {

  // how many panels?
  const size_t num_panels = std::min(10000, std::max(5, (int)(m_diam * M_PI / _ips)));

  std::cout << "Creating oval with " << num_panels << " panels" << std::endl;

  const float st = std::sin(M_PI * m_theta / 180.0);
  const float ct = std::cos(M_PI * m_theta / 180.0);

  // created once
  std::vector<float>   x(num_panels*2);
  std::vector<Int>   idx(num_panels*2);
  std::vector<float> val(num_panels);

  // outside is to the left walking from one point to the next
  // so go CW around the circle starting at theta=0 (+x axis)
  for (size_t i=0; i<num_panels; i++) {
    const float theta = 2.0 * M_PI * (float)i / (float)num_panels;
    const float dx =  0.5*m_diam * std::cos(theta);
    const float dy = -0.5*m_dmin * std::sin(theta);
    x[2*i]     = m_x + dx*ct - dy*st;
    x[2*i+1]   = m_y + dx*st + dy*ct;
    idx[2*i]   = i;
    idx[2*i+1] = i+1;
    val[i]     = 0.0;
  }

  // correct the final index
  idx[2*num_panels-1] = 0;

  return ElementPacket<float>({x, idx, val});
}

void
SolidOval::debug(std::ostream& os) const {
  os << to_string();
}

std::string
SolidOval::to_string() const {
  std::stringstream ss;
  ss << "solid oval at " << m_x << " " << m_y << " with diameters " << m_diam << " " << m_dmin << " rotated " << m_theta << " deg";
  return ss.str();
}

nlohmann::json
SolidOval::to_json() const {
  // make an object for the mesh
  nlohmann::json mesh = nlohmann::json::object();
  mesh["geometry"] = "oval";
  mesh["translation"] = {m_x, m_y};
  mesh["scale"] = {m_diam, m_dmin};;
  mesh["rotation"] = m_theta;
  return mesh;
}


//
// Create a square (fluid is outside)
//
ElementPacket<float>
SolidSquare::init_elements(const float _ips) const {

  // how many panels?
  const size_t num_panels = 4 * std::min(2500, std::max(1, (int)(m_side / _ips)));

  std::cout << "Creating square with " << num_panels << " panels" << std::endl;

  // created once
  std::vector<float>   x(num_panels*2);
  std::vector<Int>   idx(num_panels*2);
  std::vector<float> val(num_panels);

  const float st = std::sin(M_PI * m_theta / 180.0);
  const float ct = std::cos(M_PI * m_theta / 180.0);

  // outside is to the left walking from one point to the next
  // so go CW around the body
  size_t icnt = 0;
  for (size_t i=0; i<num_panels/4; i++) {
    const float px = m_side * -0.5;
    const float py = m_side * (-0.5 + (float)i / (float)(num_panels/4));
    x[icnt++] = m_x + px*ct - py*st;
    x[icnt++] = m_y + px*st + py*ct;
  }
  for (size_t i=0; i<num_panels/4; i++) {
    const float px = m_side * (-0.5 + (float)i / (float)(num_panels/4));
    const float py = m_side * 0.5;
    x[icnt++] = m_x + px*ct - py*st;
    x[icnt++] = m_y + px*st + py*ct;
  }
  for (size_t i=0; i<num_panels/4; i++) {
    const float px = m_side * 0.5;
    const float py = m_side * (0.5 - (float)i / (float)(num_panels/4));
    x[icnt++] = m_x + px*ct - py*st;
    x[icnt++] = m_y + px*st + py*ct;
  }
  for (size_t i=0; i<num_panels/4; i++) {
    const float px = m_side * (0.5 - (float)i / (float)(num_panels/4));
    const float py = m_side * -0.5;
    x[icnt++] = m_x + px*ct - py*st;
    x[icnt++] = m_y + px*st + py*ct;
  }

  // outside is to the left walking from one point to the next
  for (size_t i=0; i<num_panels; i++) {
    idx[2*i]   = i;
    idx[2*i+1] = i+1;
    val[i]     = 0.0;
  }

  // correct the final index
  idx[2*num_panels-1] = 0;

  return ElementPacket<float>({x, idx, val});
}

void
SolidSquare::debug(std::ostream& os) const {
  os << to_string();
}

std::string
SolidSquare::to_string() const {
  std::stringstream ss;
  ss << "solid square at " << m_x << " " << m_y << " with side " << m_side << " rotated " << m_theta << " deg";
  return ss.str();
}

nlohmann::json
SolidSquare::to_json() const {
  // make an object for the mesh
  nlohmann::json mesh = nlohmann::json::object();
  mesh["geometry"] = "square";
  mesh["translation"] = {m_x, m_y};
  mesh["scale"] = m_side;
  mesh["rotation"] = m_theta;
  return mesh;
}

