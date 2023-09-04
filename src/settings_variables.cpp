/**
 * global settings variables
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "settings_variables.h"

#include <boost/predef.h>


// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------

// resource manager
Resources g_res{};

// application binary, root, and home directory path
std::string g_apppath = ".";
std::optional<std::string> g_appdirpath;
std::string g_homepath = "~/";
std::string g_desktoppath = g_homepath;
std::string g_docpath = g_homepath;
std::string g_imgpath = g_homepath;


// maximum number of recent files
unsigned int g_maxnum_recents = 16;


// epsilons and precisions
int g_prec = 6;
int g_prec_gui = 3;
t_real g_eps = 1e-6;
t_real g_eps_angular = 0.01 / 180. * m::pi<t_real>;
t_real g_eps_gui = 1e-4;


// mouse dragging
t_real g_drag_scale_force = 10.;
t_real g_drag_scale_momentum = 0.1;


// render timer TPS
unsigned int g_timer_tps = 30;


// renderer options
t_real_gl g_move_scale = t_real_gl(1./75.);
t_real_gl g_zoom_scale = 0.0025;
t_real_gl g_wheel_zoom_scale = t_real_gl(1./64.);
t_real_gl g_rotation_scale = t_real_gl(0.02);

int g_light_follows_cursor = 0;
int g_enable_shadow_rendering = 1;

int g_enable_portal_rendering = 0;

int g_draw_bounding_rectangles = 0;


// gui theme
QString g_theme = "Fusion";

// gui font
QString g_font = "";

// use native menubar?
int g_use_native_menubar = 0;

// use native dialogs?
int g_use_native_dialogs = 0;

// use gui animations?
int g_use_animations = 0;

// allow tabbed dock widgets?
int g_tabbed_docks = 0;

// allow nested dock widgets?
int g_nested_docks = 0;
// ----------------------------------------------------------------------------
