/**
 * global settings variables
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GLSCENE_SETTINGS_VARIABLES__
#define __GLSCENE_SETTINGS_VARIABLES__

#include <QtCore/QString>

#include <string>
#include <array>
#include <variant>
#include <optional>

#include "common/Resources.h"
#include "types.h"
#include "dialogs/Settings.h"
#include "renderer/GlRenderer.h"



// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
// resource manager
extern Resources g_res;

// application binary
extern std::string g_apppath;

// application directory root path (if this exists)
extern std::optional<std::string> g_appdirpath;

// home and desktop directory path
extern std::string g_homepath;
extern std::string g_desktoppath;

// documents and image directory
extern std::string g_docpath;
extern std::string g_imgpath;


// maximum number of recent files
extern unsigned int g_maxnum_recents;


// number precisions
extern int g_prec, g_prec_gui;

// epsilons
extern t_real g_eps, g_eps_angular, g_eps_gui;


// render timer ticks per second
extern unsigned int g_timer_tps;

extern int g_light_follows_cursor;
extern int g_enable_shadow_rendering;

extern int g_enable_portal_rendering;

extern int g_draw_bounding_rectangles;

// camera translation scaling factor
extern t_real_gl g_move_scale;

// camera zoom scaling factors
extern t_real_gl g_zoom_scale;
extern t_real_gl g_wheel_zoom_scale;

// camera rotation scaling factor
extern t_real_gl g_rotation_scale;


// gui theme
extern QString g_theme;

// gui font
extern QString g_font;

// use native menubar?
extern int g_use_native_menubar;

// use native dialogs?
extern int g_use_native_dialogs;

// use gui animations
extern int g_use_animations;

// allow tabbed dock widgets?
extern int g_tabbed_docks;

// allow nested dock widgets?
extern int g_nested_docks;
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// variables register
// ----------------------------------------------------------------------------
constexpr std::array<SettingsVariable, 11> g_settingsvariables
{{
	// epsilons and precisions
	{
		.description = "Calculation epsilon.",
		.key = "settings/eps",
		.value = &g_eps,
	},
	{
		.description = "Angular epsilon.",
		.key = "settings/eps_angular",
		.value = &g_eps_angular,
		.is_angle = true
	},
	{
		.description = "Drawing epsilon.",
		.key = "settings/eps_gui",
		.value = &g_eps_gui,
	},
	{
		.description = "Number precision.",
		.key = "settings/prec",
		.value = &g_prec,
	},
	{
		.description = "GUI number precision.",
		.key = "settings/prec_gui",
		.value = &g_prec_gui,
	},

	// file options
	{
		.description = "Maximum number of recent files.",
		.key = "settings/maxnum_recents",
		.value = &g_maxnum_recents,
	},

	// renderer options
	{
		.description = "Timer ticks per second.",
		.key = "settings/timer_tps",
		.value = &g_timer_tps,
	},
	{
		.description = "Light follows cursor.",
		.key = "settings/light_follows_cursor",
		.value = &g_light_follows_cursor,
		.editor = SettingsVariableEditor::YESNO,
	},
	{
		.description = "Enable shadow rendering.",
		.key = "settings/enable_shadow_rendering",
		.value = &g_enable_shadow_rendering,
		.editor = SettingsVariableEditor::YESNO,
	},
	{
		.description = "Enable portal rendering.",
		.key = "settings/enable_portal_rendering",
		.value = &g_enable_portal_rendering,
		.editor = SettingsVariableEditor::YESNO,
	},
	{
		.description = "Draw bounding rectangles.",
		.key = "settings/draw_bounding_rectangles",
		.value = &g_draw_bounding_rectangles,
		.editor = SettingsVariableEditor::YESNO,
	},
}};
// ----------------------------------------------------------------------------


#endif
