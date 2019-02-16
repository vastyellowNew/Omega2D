/*
 * Body.cpp - class for an independent solid boundary
 *
 * (c)2017-9 Applied Scientific Research, Inc.
 *           Written by Mark J Stock <markjstock@gmail.com>
 */

#include "Body.h"

#include <cassert>
#include <iostream>

//
// A single rigid body
//
// Note that Vec = std::array<double,Dimensions>
//

// delegating ctor
Body::Body() :
  Body(0.0, 0.0)
  {}

// primary constructor
Body::Body(const double _x, const double _y) :
  this_time(0.0),
  pos(Vec({{_x, _y}})),
  vel(Vec({{0.0, 0.0}})),
  apos(0.0),
  avel(0.0)
{
  // time (t) is the only variable allowed in the equations
  func_vars.push_back({"t", &this_time});
  // ane make space for the compiled functions
  pos_func.resize(Dimensions);
}

Body::~Body() {
  // free the internal memory used by tinyexpr
  for (size_t i=0; i<pos_func.size(); ++i) te_free(pos_func[i]);
}

// getters/setters

void Body::set_name(const std::string _name) { name = _name; }
void Body::set_parent_name(const std::string _name) { parent = _name; }
std::string Body::get_name() { return name; }

void Body::set_pos(const size_t _i, const double _val) {
  assert(_i>=0 and _i<Dimensions);
  pos[_i] = _val;
}

void Body::set_pos(const size_t _i, const std::string _val) {
  assert(_i>=0 and _i<Dimensions);
  // store the expression locally
  pos_expr[_i] = _val;
  // compile it
  int ierr = 0;
  pos_func[_i] = te_compile(_val.c_str(), func_vars.data(), 1, &ierr);
  if (pos_func[_i]) {
    std::cout << "  testing parsed expression, with t=0, value is " << te_eval(pos_func[_i]) << std::endl;
  } else {
    std::cout << "  Error parsing expression (" << _val << "), near character " << ierr << std::endl;
  }
}

Vec Body::get_pos(const double _time) {
  // for testing, return a loop
  //pos[0] = 0.5 * sin(2.0*_time);
  //pos[1] = 0.5 * (1.0-cos(2.0*_time));

  // for realsies, get the value or evaluate the expression
  this_time = _time;
  for (size_t i=0; i<Dimensions; ++i) {
    if (pos_func[i]) {
      pos[i] = te_eval(pos_func[i]);
      //std::cout << "IDEAL POS " << (0.5 * (1.0-cos(2.0*_time))) << "  AND ACTUAL " << pos[i] << std::endl;
    }
  }

  return pos;
}

Vec Body::get_vel(const double _time) {
  // for testing, return a loop
  //vel[0] = cos(2.0*_time);
  //vel[1] = 0.5 * 2.0*sin(2.0*_time);

  // 2-point first derivative estimate
  const double dt = 1.e-5;
  for (size_t i=0; i<Dimensions; ++i) {
    if (pos_func[i]) {
      this_time = _time + dt;
      const double pplus = te_eval(pos_func[i]);
      this_time = _time - dt;
      const double pminus = te_eval(pos_func[i]);
      vel[i] = (pplus - pminus) / (2.0*dt);
      //std::cout << "      VEL " << (0.5 * 2.0*sin(2.0*_time)) << "  AND ACTUAL " << vel[i] << std::endl;
      //std::cout << "        used " << pplus << " and " << pminus << std::endl;
    }
  }

  return vel;
}

double Body::get_orient(const double _time) {
  return apos;
}

double Body::get_rotvel(const double _time) {
  return avel;
}

