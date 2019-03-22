/*
 * Simulation.cpp - a class to control a 2d vortex particle sim
 *
 * (c)2017-9 Applied Scientific Research, Inc.
 *           Written by Mark J Stock <markjstock@gmail.com>
 */

#include "Simulation.h"
#include "Points.h"
#include "VtkXmlHelper.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <variant>

// constructor
Simulation::Simulation()
  : re(100.0),
    dt(0.01),
    fs{0.0,0.0},
    vort(),
    bdry(),
    fldpt(),
    bem(),
    diff(),
    conv(),
    description(),
    time(0.0),
    output_dt(0.0),
    end_time(0.0),
    use_end_time(false),
    max_steps(0),
    use_max_steps(false),
    sim_is_initialized(false),
    step_has_started(false),
    step_is_finished(false)
  {}

// addresses for use in imgui
float* Simulation::addr_re() { return &re; }
float* Simulation::addr_dt() { return &dt; }
float* Simulation::addr_fs() { return fs; }

// getters
float Simulation::get_hnu() { return std::sqrt(dt/re); }
float Simulation::get_ips() { return diff.get_nom_sep_scaled() * get_hnu(); }
float Simulation::get_vdelta() { return diff.get_particle_overlap() * get_ips(); }
float Simulation::get_time() { return (float)time; }
float Simulation::get_end_time() { return (float)end_time; }
bool Simulation::using_end_time() { return use_end_time; }
size_t Simulation::get_max_steps() { return max_steps; }
bool Simulation::using_max_steps() { return use_max_steps; }
float Simulation::get_output_dt() { return (float)output_dt; }
std::string Simulation::get_description() { return description; }

// setters
void Simulation::set_description(const std::string desc) { description = desc; }
void Simulation::set_end_time(const double net) { end_time = net; use_end_time = true; }
void Simulation::set_max_steps(const size_t nms) { max_steps = nms; use_max_steps = true; }
void Simulation::set_output_dt(const double nodt) { output_dt = nodt; }

// status
size_t Simulation::get_npanels() {
  size_t n = 0;
  for (auto &coll: bdry) {
    //std::visit([&n](auto& elem) { n += elem.get_npanels(); }, coll);
    // only proceed if the last collection is Surfaces
    if (std::holds_alternative<Surfaces<float>>(coll)) {
      Surfaces<float>& surf = std::get<Surfaces<float>>(coll);
      n += surf.get_npanels();
    }
  }
  return n;
}

size_t Simulation::get_nparts() {
  size_t n = 0;
  for (auto &coll: vort) {
    std::visit([&n](auto& elem) { n += elem.get_n(); }, coll);
  }
  return n;
}

size_t Simulation::get_nfldpts() {
  size_t n = 0;
  for (auto &coll : fldpt) {
    std::visit([&n](auto& elem) { n += elem.get_n(); }, coll);
  }
  return n;
}

// like a setter
void Simulation::set_re_for_ips(const float _ips) {
  re = std::pow(diff.get_nom_sep_scaled(), 2) * dt / pow(_ips, 2);
  diff.set_diffuse(false);
}

void Simulation::set_diffuse(const bool _do_diffuse) {
  diff.set_diffuse(_do_diffuse);
}

const bool Simulation::get_diffuse() {
  return diff.get_diffuse();
}

//void Simulation::set_amr(const bool _do_amr) {
//  diff.set_amr(_do_amr);
//  diff.set_diffuse(true);
//}

#ifdef USE_GL
void Simulation::initGL(std::vector<float>& _projmat,
                        float*              _poscolor,
                        float*              _negcolor,
                        float*              _defcolor) {
  for (auto &coll : vort) {
    std::visit([=, &_projmat](auto& elem) { elem.initGL(_projmat, _poscolor, _negcolor, _defcolor); }, coll);
  }
  for (auto &coll : bdry) {
    std::visit([=, &_projmat](auto& elem) { elem.initGL(_projmat, _poscolor, _negcolor, _defcolor); }, coll);
  }
  for (auto &coll : fldpt) {
    std::visit([=, &_projmat](auto& elem) { elem.initGL(_projmat, _poscolor, _negcolor, _defcolor); }, coll);
  }
}

void Simulation::updateGL() {
  for (auto &coll : vort) {
    std::visit([=](auto& elem) { elem.updateGL(); }, coll);
  }
  for (auto &coll : bdry) {
    std::visit([=](auto& elem) { elem.updateGL(); }, coll);
  }
  for (auto &coll : fldpt) {
    std::visit([=](auto& elem) { elem.updateGL(); }, coll);
  }
}

void Simulation::drawGL(std::vector<float>& _projmat,
                        RenderParams&       _rparams) {

  if (step_is_finished) {
    _rparams.tracer_size = get_ips() * _rparams.tracer_scale;
    for (auto &coll : vort) {
      std::visit([&](auto& elem) { elem.drawGL(_projmat, _rparams); }, coll);
    }
    for (auto &coll : bdry) {
      std::visit([&](auto& elem) { elem.drawGL(_projmat, _rparams); }, coll);
    }
    for (auto &coll : fldpt) {
      std::visit([&](auto& elem) { elem.drawGL(_projmat, _rparams); }, coll);
    }
  }
}
#endif

bool Simulation::is_initialized() { return sim_is_initialized; }

void Simulation::set_initialized() { sim_is_initialized = true; }

void Simulation::reset() {

  // must wait for step() to complete, if it's still working
  if (stepfuture.valid()) {
    stepfuture.wait();
    stepfuture.get();
  }

  // now reset everything else
  time = 0.0;
  vort.clear();
  bdry.clear();
  fldpt.clear();
  bem.reset();
  sim_is_initialized = false;
  step_has_started = false;
  step_is_finished = false;
}

void Simulation::clear_bodies() {
  bodies.clear();
}

// Write a set of vtu files for the particles and panels
void Simulation::write_vtk() {
  static size_t frameno = 0;

  size_t idx = 0;
  for (auto &coll : vort) {
    // eventually all collections will support vtk output
    //std::visit([=](auto& elem) { elem.write_vtk(); }, coll);
    // only proceed if the collection is Points
    if (std::holds_alternative<Points<float>>(coll)) {
      Points<float>& pts = std::get<Points<float>>(coll);
      write_vtu_points<float>(pts, idx++, frameno);
    }
  }

  idx = 0;
  for (auto &coll : fldpt) {
    // eventually all collections will support vtk output
    //std::visit([=](auto& elem) { elem.write_vtk(); }, coll);
    // only proceed if the collection is Points
    if (std::holds_alternative<Points<float>>(coll)) {
      Points<float>& pts = std::get<Points<float>>(coll);
      write_vtu_points<float>(pts, idx++, frameno);
    }
  }

  frameno++;
}

//
// Check all aspects of the simulation for conditions that should stop the run
//
std::string Simulation::check_simulation(const size_t _nff, const size_t _nbf) {
  std::string retstr;

  // Check for no bodies and no particles
  if (_nbf == 0 and get_nparts() == 0) {
    retstr.append("No flow features and no bodies. Add one or both, reset, and run.\n");
  }

  // Check for a body and no particles
  if (_nbf > 0 and get_nparts() == 0) {

    const bool zero_freestream = (fs[0]*fs[0]+fs[1]*fs[1] < std::numeric_limits<float>::epsilon());
    const bool no_body_movement = not do_any_bodies_move();

    // AND no freestream
    if (zero_freestream and no_body_movement) {
      retstr.append("No flow features and zero freestream speed - try adding one or both.\n");
      return retstr;
    }

    // AND no viscosity
    if (not diff.get_diffuse()) {
      retstr.append("You have a solid body, but no diffusion. It will not shed vorticity. Turn on viscosity or add a flow feature, reset, and run.\n");
    }
  }

  return retstr;
}

//
// check all bodies for movement
//
bool Simulation::do_any_bodies_move() {
  bool some_move = false;
  for (size_t i=0; i<bodies.size(); ++i) {
    auto thisvel = bodies[i]->get_vel(time);
    auto nextvel = bodies[i]->get_vel(time+dt);
    auto thisrot = bodies[i]->get_rotvel(time);
    auto nextrot = bodies[i]->get_rotvel(time+dt);
    if (std::abs(thisvel[0]) + std::abs(thisvel[1]) + std::abs(thisrot) +
        std::abs(nextvel[0]) + std::abs(nextvel[1]) + std::abs(nextrot) >
        std::numeric_limits<float>::epsilon()) {
      some_move = true;
    }
  }
  return some_move;
}

//
// query and get() the future if possible
//
bool Simulation::test_for_new_results() {

  if (not step_has_started) {
    // if we haven't made an async call yet
    return true;

  } else if (is_future_ready(stepfuture)) {
    // if we did, and it's ready to give us the results
    stepfuture.get();

#ifdef USE_GL
    // tell flow objects to update their values to the GPU
    updateGL();
#endif

    // set flag indicating that at least one step has been solved
    step_is_finished = true;
    step_has_started = false;

    return true;
  }

  // async call is not finished, do not try calling it again
  return false;
}

//
// call this from a real-time GUI - will only run a new step if the last one is done
//
void Simulation::async_step() {
  step_has_started = true;
  stepfuture = std::async(std::launch::async, [this](){step();});
}

//
// here's the vortex method: convection and diffusion with operator splitting
//
void Simulation::step() {
  std::cout << std::endl << "Taking step at t=" << time << " with n=" << get_nparts() << std::endl;

  // we wind up using this a lot
  std::array<double,2> thisfs = {fs[0], fs[1]};

  // for simplicity's sake, just run one full diffusion step here
  diff.step(time, dt, re, get_vdelta(), thisfs, vort, bdry, bem);

  // operator splitting requires one half-step diffuse (use coefficients from previous step, if available)
  //diff.step(time, 0.5*dt, get_vdelta(), get_ips(), thisfs, vort, bdry);

  // advect with no diffusion (must update BEM strengths)
  //conv.advect_1st(time, dt, thisfs, vort, bdry, fldpt, bem);
  conv.advect_2nd(time, dt, thisfs, vort, bdry, fldpt, bem);

  // operator splitting requires another half-step diffuse (must compute new coefficients)
  //diff.step(time, 0.5*dt, get_vdelta(), get_ips(), thisfs, vort, bdry);

  // update strengths for coloring purposes (eventually should be taken care of automatically)
  //vort.update_max_str();

  // update dt and return
  time += (double)dt;
}

// set up some vortex particles
// TODO - accept elem_t and move_t from the caller!
void Simulation::add_particles(std::vector<float> _invec) {

  if (_invec.size() == 0) return;

  // make sure we're getting full particles
  assert(_invec.size() % 4 == 0);

  // add the vdelta to each particle and pass it on
  const float thisvd = get_vdelta();
  for (size_t i=3; i<_invec.size(); i+=4) {
    _invec[i] = thisvd;
  }

  // if no collections exist
  if (vort.size() == 0) {
    // make a new collection
    vort.push_back(Points<float>(_invec, active, lagrangian, nullptr));      // vortons

  } else {
    // THIS MUST USE A VISITOR
    // HACK - add all particles to first collection
    auto& coll = vort.back();
    // only proceed if the last collection is Points
    if (std::holds_alternative<Points<float>>(coll)) {
      Points<float>& pts = std::get<Points<float>>(coll);
      pts.add_new(_invec);
    }
  }
}

// add some tracer particles to new arch
void Simulation::add_fldpts(std::vector<float> _invec, const bool _moves) {

  if (_invec.size() == 0) return;

  // make sure we're getting full points
  assert(_invec.size() % Dimensions == 0);

  const move_t move_type = _moves ? lagrangian : fixed;

  // add to new archtecture

  // if no collections exist
  if (fldpt.size() == 0) {
    // make a new collection
    fldpt.push_back(Points<float>(_invec, inert, move_type, nullptr));

  } else {
    // THIS MUST USE A VISITOR
    // HACK - add all particles to first collection
    //std::visit([&](auto& elem) { elem.add_new(_invec); }, fldpt.back());
    auto& coll = fldpt.back();
    // eventually we will want to check every collection for matching element and move types
    // only proceed if the collection is Points
    if (std::holds_alternative<Points<float>>(coll)) {
      Points<float>& pts = std::get<Points<float>>(coll);
      pts.add_new(_invec);
    }
  }
}

// add geometry
void Simulation::add_boundary(std::shared_ptr<Body> _bptr, ElementPacket<float> _geom) {

  // incoming collections types
  const elem_t this_elem_type = reactive;
  const move_t this_move_type = (_bptr ? bodybound : fixed);

  // search the collections list for a match (same movement type and Body)
  size_t imatch = 0;
  bool no_match = true;
  for (size_t i=0; i<bdry.size(); ++i) {
    // assume match
    bool this_match = true;

    // check element type
    elem_t tet = std::visit([=](auto& elem) { return elem.get_elemt(); }, bdry[i]);
    if (this_elem_type != tet) this_match = false;

    // check movement type
    move_t tmt = std::visit([=](auto& elem) { return elem.get_movet(); }, bdry[i]);
    if (this_move_type != tmt) this_match = false;

    // check body pointer
    if (this_move_type == bodybound and tmt == bodybound) {
      std::shared_ptr<Body> tbp = std::visit([=](auto& elem) { return elem.get_body_ptr(); }, bdry[i]);
      if (_bptr != tbp) this_match = false;
    }

    // check Collections type (add later)
    //if (std::holds_alternative<Surfaces<float>>(coll) not std::holds_alternative<Surfaces<float>>(coll)) this_match = false;

    if (this_match) {
      imatch = i;
      no_match = false;
    }
  }

  // if no match, or no collections exist
  if (no_match) {
    // make a new collection - assume BEM panels
    bdry.push_back(Surfaces<float>(_geom.x,
                                   _geom.idx,
                                   _geom.val,
                                   reactive, this_move_type, _bptr));
  } else {
    // found a match
    auto& coll = bdry[imatch];
    // only proceed if the last collection is Surfaces
    if (std::holds_alternative<Surfaces<float>>(coll)) {
      Surfaces<float>& surf = std::get<Surfaces<float>>(coll);
      surf.add_new(_geom.x,
                   _geom.idx,
                   _geom.val);
    }
  }
}

// add a new Body with the given name
void Simulation::add_body(std::shared_ptr<Body> _body) {
  bodies.emplace_back(_body);
  std::cout << "  added new body (" << _body->get_name() << "), now have " << bodies.size() << std::endl;
}

// return a Body pointer to the last Body in the array
std::shared_ptr<Body> Simulation::get_last_body() {
  std::shared_ptr<Body> bp;

  if (bodies.size() == 0) {
    std::cout << "  no last body found, creating (ground)" << std::endl;
    bp = std::make_shared<Body>();
    bp->set_name("ground");
    add_body(bp);
  } else {
    bp = bodies.back();
    std::cout << "  returning last body (" << bp->get_name() << ")" << std::endl;
  }

  return bp;
}

// return a Body pointer to the body matching the given name
std::shared_ptr<Body> Simulation::get_pointer_to_body(const std::string _name) {
  std::shared_ptr<Body> bp;

  for (auto &bptr : bodies) {
    if (bptr->get_name() == _name) {
      std::cout << "  found body matching name (" << _name << ")" << std::endl;
    }
  }

  // or ground if none match
  if (not bp) {
    std::cout << "  no body matching (" << _name << ") found, creating (ground)" << std::endl;
    bp = std::make_shared<Body>();
    bp->set_name("ground");
    add_body(bp);
  }

  return bp;
}

