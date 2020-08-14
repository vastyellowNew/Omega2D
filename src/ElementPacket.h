/*
 * ElementPacket.h - Pass around fundamental geometry
 *
 * (c)2018-20 Applied Scientific Research, Inc.
 *            Mark J Stock <markjstock@gmail.com>
 *            Blake B Hillier <blakehillier@mac.com>
 */

#pragma once

#include "Omega2D.h"

#include <vector>

// Helper class for passing arbitrary elements around
template<class S>
struct ElementPacket {
  ElementPacket<S>() = default;
  ~ElementPacket<S>() = default;

  ElementPacket<S>(ElementPacket<S> const&) = default; //allow copy
  ElementPacket<S>(ElementPacket<S>&&) = default; //allow move
  ElementPacket<S>& operator=(ElementPacket<S> const&) = default; //allow copy
  ElementPacket<S>& operator=(ElementPacket<S>&&) = default; //allow move

  // Ensure the element packet is correct
  // This will probaly need some parameters and some if statements
  // Check the sim.add_* functions
  bool verify(int attribute, int check) { return (attribute % check == 0); }

  std::vector<S> x;
  std::vector<Int> idx;
  std::vector<S> val;
  size_t nelem;
  uint8_t ndim;	// 0=points, 1=surfaces, 2=volumes for 2D
		// 0=points, 1=lines, 2=surfaces, 3=volumes for 3D
};

