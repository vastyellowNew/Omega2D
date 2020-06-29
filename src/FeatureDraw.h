/*
 * FeatureDraw.h - Draw Features before they are initialized
 *
 * (c)2020 Applied Scientific Research, Inc.
 *         Mark J Stock <markjstock@gmail.com>
 */

#pragma once

#include "ElementPacket.h"
#include "RenderParams.h"
#include "GlState.h"


// Control storage and drawing of features before Simulation takes over
class FeatureDraw {

public:
  FeatureDraw() { }

  // deleting GlState will destroy buffers
  ~FeatureDraw() { }

  void updateGL();
  void drawGL(std::vector<float>&, RenderParams&);

private:
  //
  // member variables
  //
  void prepare_opengl_buffer(GLuint, GLuint, const GLchar*);
  void initGL(std::vector<float>&, float*, float*, float*);

  // Collected geometry
  ElementPacket<float> m_geom;

  // VAO, VBOs, etc.
  std::shared_ptr<GlState> m_gl;
};
