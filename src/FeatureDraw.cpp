/*
 * FeatureDraw.cpp - Draw Features before they are initialized
 *
 * (c)2020 Applied Scientific Research, Inc.
 *         Mark J Stock <markjstock@gmail.com>
 */

#include "FeatureDraw.h"
#include "ShaderHelper.h"
#include "OglHelper.h"

#include <iostream>
#include <vector>
#include <cassert>

const std::string vert_shader_source =
#include "shaders/elempack.vert"
;
const std::string frag_shader_source =
#include "shaders/elempack.frag"
;

// Control storage and drawing of features before Simulation takes over

// helper function to clean up initGL
void FeatureDraw::prepare_opengl_buffer(GLuint _prog, GLuint _idx, const GLchar* _name) {
  glBindBuffer(GL_ARRAY_BUFFER, m_gl->vbo[_idx]);
  const GLint position_attribute = glGetAttribLocation(_prog, _name);
  // Specify how the data for position can be accessed
  glVertexAttribPointer(position_attribute, 1, get_gl_type<float>, GL_FALSE, 0, 0);
  // Enable the attribute
  glEnableVertexAttribArray(position_attribute);
}

void FeatureDraw::initGL(std::vector<float>& _projmat,
                         float*              _poscolor,
                         float*              _negcolor,
                         float*              _defcolor) {

  std::cout << "inside FeatureDraw::initGL" << std::endl;

  // generate the opengl state object and bind the vao
  m_gl = std::make_shared<GlState>(2,1);

  // Allocate space, but don't upload the data from CPU to GPU yet
  glBindBuffer(GL_ARRAY_BUFFER, m_gl->vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, 0, m_geom.x.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gl->vbo[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, m_geom.idx.data(), GL_STATIC_DRAW);

  // Load and create the line-drawing shader program
  //m_gl->spo[0] = create_draw_surface_line_prog();
  m_gl->spo[0] = create_vertfrag_prog(vert_shader_source, frag_shader_source);

  // Now do the two arrays
  prepare_opengl_buffer(m_gl->spo[0], 0, "pos");
  prepare_opengl_buffer(m_gl->spo[0], 1, "str");

  // Get the location of the attributes that enters in the vertex shader
  m_gl->projmat_attribute = glGetUniformLocation(m_gl->spo[0], "Projection");

  // upload the projection matrix
  glUniformMatrix4fv(m_gl->projmat_attribute, 1, GL_FALSE, _projmat.data());

  // locate where the colors and color scales go
  m_gl->pos_color_attribute = glGetUniformLocation(m_gl->spo[0], "pos_color");
  m_gl->neg_color_attribute = glGetUniformLocation(m_gl->spo[0], "neg_color");
  m_gl->def_color_attribute = glGetUniformLocation(m_gl->spo[0], "def_color");

  // send the current values
  glUniform4fv(m_gl->pos_color_attribute, 1, (const GLfloat *)_poscolor);
  glUniform4fv(m_gl->neg_color_attribute, 1, (const GLfloat *)_negcolor);
  glUniform4fv(m_gl->def_color_attribute, 1, (const GLfloat *)_defcolor);
  //std::cout << "init pos color as " << _poscolor[0] << " " << _poscolor[1] << " " << _poscolor[2] << " " << _poscolor[3] << std::endl;

  // and indicate the fragment color output
  glBindFragDataLocation(m_gl->spo[0], 0, "frag_color");

  glBindVertexArray(0);
}

void FeatureDraw::updateGL() {

  // has this been init'd yet?
  if (not m_gl) return;
  if (glIsVertexArray(m_gl->vao) == GL_FALSE) return;

  // has the number of elements changed?
  if (m_gl->num_uploaded != (GLsizei)m_geom.idx.size()) {
    glBindVertexArray(m_gl->vao);

    // Indicate and upload the data from CPU to GPU

    // the positions
    glBindBuffer(GL_ARRAY_BUFFER, m_gl->vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*m_geom.x.size(), m_geom.x.data(), GL_DYNAMIC_DRAW);

    // and element indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gl->vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Int)*m_geom.idx.size(), m_geom.idx.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);

    // must tell draw call how many elements are there - or, really, how many indices
    m_gl->num_uploaded = m_geom.idx.size();
  }
}

void FeatureDraw::drawGL(std::vector<float>& _projmat,
                         RenderParams&       _rparams) {

  // has this been init'd yet?
  if (not m_gl) {
    initGL(_projmat, _rparams.pos_circ_color,
                     _rparams.neg_circ_color,
                     _rparams.default_color);
    updateGL();
  }

  // draw if there are any elems to draw
  if (m_gl->num_uploaded > 0) {
    glBindVertexArray(m_gl->vao);

    // get blending ready
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // draw as lines
    glUseProgram(m_gl->spo[0]);

    // upload the current projection matrix
    glUniformMatrix4fv(m_gl->projmat_attribute, 1, GL_FALSE, _projmat.data());

    // upload the current color values
    glUniform4fv(m_gl->pos_color_attribute, 1, (const GLfloat *)_rparams.pos_circ_color);
    glUniform4fv(m_gl->neg_color_attribute, 1, (const GLfloat *)_rparams.neg_circ_color);
    glUniform4fv(m_gl->def_color_attribute, 1, (const GLfloat *)_rparams.default_color);

    // the one draw call here
    glDrawElements(GL_LINES, m_gl->num_uploaded, get_gl_type<Int>, 0);

    // return state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindVertexArray(0);
  }
}
