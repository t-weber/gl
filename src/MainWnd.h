/**
 * GlScene -- main window
 * @author Tobias Weber <tweber@ill.fr>
 * @date February - November 2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GLSCENE_MAINWND_H__
#define __GLSCENE_MAINWND_H__

#include <QtCore/QSettings>
#include <QtCore/QByteArray>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QLabel>

#include <string>
#include <memory>
#include <future>
#include <functional>

#include "tlibs2/libs/maths.h"

#include "types.h"
#include "Scene.h"
#include "settings_variables.h"

#include "common/Resources.h"
#include "common/Recent.h"

#include "renderer/GlRenderer.h"

#include "dialogs/GeoBrowser.h"
#include "dialogs/TextureBrowser.h"
#include "dialogs/About.h"
#include "dialogs/Settings.h"

#include "dock/CamProperties.h"


class PathsTool : public QMainWindow
{ Q_OBJECT
public:
	/**
	 * create UI
	 */
	PathsTool(QWidget* pParent = nullptr);
	virtual ~PathsTool() = default;

	PathsTool(const PathsTool&) = delete;
	const PathsTool& operator=(const PathsTool&) = delete;

	void SetInitialSceneFile(const std::string& file);

	// load file
	bool OpenFile(const QString &file);

	// save file
	bool SaveFile(const QString &file);


private:
	QSettings m_sett{};
	QByteArray m_initial_state{};

	// renderer
	std::shared_ptr<PathsRenderer> m_renderer
		{ std::make_shared<PathsRenderer>(this) };
	int m_multisamples{ 8 };

	// gl info strings
	std::string m_gl_ver{}, m_gl_shader_ver{},
		m_gl_vendor{}, m_gl_renderer{};

	QStatusBar *m_statusbar{ nullptr };
	QLabel *m_labelStatus{ nullptr };

	QMenu *m_menuOpenRecent{ nullptr };
	QMenuBar *m_menubar{ nullptr };

	// context menu for 3d objects
	QMenu *m_contextMenuObj{ nullptr };
	std::string m_curContextObj{};

	// dialogs
	// cannot directly use the type here because this causes a -Wsubobject-linkage warning
	//using t_SettingsDlg = SettingsDlg<g_settingsvariables.size(), &g_settingsvariables>;
	std::shared_ptr<AboutDlg> m_dlgAbout{};
	std::shared_ptr<QDialog> m_dlgSettings{};
	std::shared_ptr<GeometriesBrowser> m_dlgGeoBrowser{};
	std::shared_ptr<TextureBrowser> m_dlgTextureBrowser{};

	// docks
	std::shared_ptr<CamPropertiesDockWidget> m_camProperties{};

	std::string m_initialSceneFile = "startup.glscene";
	bool m_initialSceneFileModified = false;

	// recently opened files
	RecentFiles m_recent{};

	// function to call for the recent file menu items
	std::function<bool(const QString& filename)> m_open_func
		= [this](const QString& filename) -> bool
	{
		return this->OpenFile(filename);
	};

	// scene configuration
	Scene m_scene{};

	// mouse picker
	t_real m_mouseX{}, m_mouseY{};
	std::string m_curObj{};


protected:
	// events
	virtual void showEvent(QShowEvent *) override;
	virtual void hideEvent(QHideEvent *) override;
	virtual void closeEvent(QCloseEvent *) override;
	virtual void dragEnterEvent(QDragEnterEvent *) override;
	virtual void dropEvent(QDropEvent *) override;

	// save a screenshot of the scene 3d view
	bool SaveScreenshot(const QString& file);

	// remember current file and set window title
	void SetCurrentFile(const QString &file);


protected slots:
	// File -> New
	void NewFile();
	bool LoadInitialSceneFile();

	// File -> Open
	void OpenFile();

	// File -> Save
	void SaveFile();

	// File -> Save As
	void SaveFileAs();

	// File -> Save Screenshot
	void SaveScreenshot();

	// called after the plotter has initialised
	void AfterGLInitialisation();

	// mouse coordinates on base plane
	void CursorCoordsChanged(t_real_gl x, t_real_gl y);

	// mouse is over an object
	void PickerIntersection(const t_vec3_gl* pos, std::string obj_name);

	// clicked on an object
	void ObjectClicked(const std::string& obj, bool left, bool middle, bool right);

	// dragging an object
	void ObjectDragged(bool drag_start, const std::string& obj,
		t_real_gl x_start, t_real_gl y_start, t_real_gl x, t_real_gl y);

	// set temporary status message, by default for 2 seconds
	void SetTmpStatus(const std::string& msg, int msg_duration=2000);

	// update permanent status message
	void UpdateStatusLabel();

	// add or delete 3d objects
	void AddCuboid();
	void AddSphere();
	void AddCylinder();
	void AddTetrahedron();
	void AddOctahedron();
	void AddDodecahedron();
	void AddIcosahedron();

	void DeleteCurrentObject();
	void RotateCurrentObject(t_real angle, char axis='z');
	void ShowCurrentObjectProperties();

	void ShowGeometryBrowser();
	void ShowTextureBrowser();

	void CollectGarbage();

	void RotateObject(const std::string& id, t_real angle, char axis='z');
	void DeleteObject(const std::string& id);
	void RenameObject(const std::string& oldid, const std::string& newid);
	void ChangeObjectProperty(const std::string& id, const ObjectProperty& prop);

	// propagate (changed) global settings to each object
	void InitSettings();
};


#endif
