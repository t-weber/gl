/**
 * GlScene -- main window
 * @author Tobias Weber <tweber@ill.fr>
 * @date February-November 2021
 * @note some code forked on 15-nov-2021 from my private "qm" project: https://github.com/t-weber/qm
 * @license GPLv3, see 'LICENSE' file
 */

#include "MainWnd.h"

#include <QtCore/QMetaObject>
#include <QtCore/QMimeData>
#include <QtCore/QThread>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtGui/QDesktopServices>

#include <boost/predef.h>
#include <boost/scope_exit.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include <chrono>


// instantiate the settings dialog class
template class SettingsDlg<g_settingsvariables.size(), &g_settingsvariables>;
using t_SettingsDlg = SettingsDlg<g_settingsvariables.size(), &g_settingsvariables>;


/**
 * constructor, create UI
 */
MainWnd::MainWnd(QWidget* pParent) : QMainWindow{pParent}
{
	setWindowTitle(APPL_TITLE);

	// set the program icon
	auto icon_file = g_res.FindFile("glscene.svg");
	if(!icon_file)
		icon_file = g_res.FindFile("glscene.svg");

	if(icon_file && !icon_file->empty())
	{
		QIcon icon{icon_file->c_str()};
		setWindowIcon(icon);
	}

	// set-up common gui variables
	t_SettingsDlg::SetGuiTheme(&g_theme);
	t_SettingsDlg::SetGuiFont(&g_font);
	t_SettingsDlg::SetGuiUseNativeMenubar(&g_use_native_menubar);
	t_SettingsDlg::SetGuiUseNativeDialogs(&g_use_native_dialogs);
	t_SettingsDlg::SetGuiUseAnimations(&g_use_animations);
	t_SettingsDlg::SetGuiTabbedDocks(&g_tabbed_docks);
	t_SettingsDlg::SetGuiNestedDocks(&g_nested_docks);

	// restore settings
	t_SettingsDlg::ReadSettings(&m_sett);


	// --------------------------------------------------------------------
	// rendering widget
	// --------------------------------------------------------------------
	// set gl surface format
	m_renderer->setFormat(gl_format(true, _GL_MAJ_VER, _GL_MIN_VER,
		m_multisamples, m_renderer->format()));

	auto plotpanel = new QWidget(this);

	connect(m_renderer.get(), &GlSceneRenderer::CursorCoordsChanged, this, &MainWnd::CursorCoordsChanged);
	connect(m_renderer.get(), &GlSceneRenderer::PickerIntersection, this, &MainWnd::PickerIntersection);
	connect(m_renderer.get(), &GlSceneRenderer::ObjectClicked, this, &MainWnd::ObjectClicked);
	connect(m_renderer.get(), &GlSceneRenderer::ObjectDragged, this, &MainWnd::ObjectDragged);
	connect(m_renderer.get(), &GlSceneRenderer::AfterGLInitialisation, this, &MainWnd::AfterGLInitialisation);

	// camera position
	connect(m_renderer.get(), &GlSceneRenderer::CamPositionChanged,
		[this](t_real_gl _x, t_real_gl _y, t_real_gl _z) -> void
		{
			t_real x = t_real(_x);
			t_real y = t_real(_y);
			t_real z = t_real(_z);

			if(m_camProperties)
				m_camProperties->GetWidget()->SetPosition(x, y, z);
		});

	// camera rotation
	connect(m_renderer.get(), &GlSceneRenderer::CamRotationChanged,
		[this](t_real_gl _phi, t_real_gl _theta) -> void
		{
			t_real phi = t_real(_phi);
			t_real theta = t_real(_theta);

			if(m_camProperties)
			{
				m_camProperties->GetWidget()->SetRotation(
					phi*t_real{180}/m::pi<t_real>,
					theta*t_real{180}/m::pi<t_real>);
			}
		});

	// camera zoom
	connect(m_renderer.get(), &GlSceneRenderer::CamZoomChanged,
		[this](t_real_gl _zoom) -> void
		{
			t_real zoom = t_real(_zoom);

			if(m_camProperties)
				m_camProperties->GetWidget()->SetZoom(zoom);
		});

	auto pGrid = new QGridLayout(plotpanel);
	pGrid->setSpacing(4);
	pGrid->setContentsMargins(4, 4, 4, 4);

	pGrid->addWidget(m_renderer.get(), 0,0,1,4);

	setCentralWidget(plotpanel);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// dock widgets
	// --------------------------------------------------------------------
	m_camProperties = std::make_shared<CamPropertiesDockWidget>(this);
	m_simProperties = std::make_shared<SimPropertiesDockWidget>(this);
	m_selProperties = std::make_shared<SelectionPropertiesDockWidget>(this);

	for(QDockWidget* dockwidget : std::initializer_list<QDockWidget*>
		{ m_camProperties.get(), m_simProperties.get(), m_selProperties.get() })
	{
		dockwidget->setFeatures(
			QDockWidget::DockWidgetClosable |
			QDockWidget::DockWidgetMovable |
			QDockWidget::DockWidgetFloatable /*|
			QDockWidget::DockWidgetVerticalTitleBar*/);
		dockwidget->setAllowedAreas(Qt::AllDockWidgetAreas);
	}

	addDockWidget(Qt::RightDockWidgetArea, m_camProperties.get());
	addDockWidget(Qt::RightDockWidgetArea, m_simProperties.get());
	addDockWidget(Qt::RightDockWidgetArea, m_selProperties.get());

	auto* camwidget = m_camProperties->GetWidget().get();
	auto* simwidget = m_simProperties->GetWidget().get();
	auto* selwidget = m_selProperties->GetWidget().get();

	// camera viewing angle
	connect(camwidget, &CamPropertiesWidget::ViewingAngleChanged,
		[this](t_real angle) -> void
		{
			if(m_renderer)
			{
				m_renderer->GetCamera().SetFOV(
					angle/t_real{180}*m::pi<t_real>);
				m_renderer->UpdateCam();
			}
		});

	// camera zoom
	connect(camwidget, &CamPropertiesWidget::ZoomChanged,
		[this](t_real zoom) -> void
		{
			if(m_renderer)
			{
				m_renderer->GetCamera().SetZoom(zoom);
				m_renderer->UpdateCam();
			}
		});

	// camera projection
	connect(camwidget, &CamPropertiesWidget::PerspectiveProjChanged,
		[this](bool persp) -> void
		{
			if(m_renderer)
			{
				m_renderer->GetCamera().SetPerspectiveProjection(persp);
				m_renderer->UpdateCam();
			}
		});

	// camera position
	connect(camwidget, &CamPropertiesWidget::PositionChanged,
		[this](t_real _x, t_real _y, t_real _z) -> void
		{
			t_real_gl x = t_real_gl(_x);
			t_real_gl y = t_real_gl(_y);
			t_real_gl z = t_real_gl(_z);

			if(m_renderer)
			{
				m_renderer->GetCamera().SetPosition(
					m::create<t_vec3_gl>({x, y, z}));
				m_renderer->UpdateCam();
			}
		});

	// camera rotation
	connect(camwidget, &CamPropertiesWidget::RotationChanged,
		[this](t_real _phi, t_real _theta) -> void
		{
			t_real_gl phi = t_real_gl(_phi);
			t_real_gl theta = t_real_gl(_theta);

			if(m_renderer)
			{
				m_renderer->GetCamera().SetRotation(
					phi/t_real_gl{180}*m::pi<t_real_gl>,
					theta/t_real_gl{180}*m::pi<t_real_gl>);
				m_renderer->UpdateCam();
			}
		});

	// time scale
	connect(simwidget, &SimPropertiesWidget::TimeScaleChanged,
		[this](t_real t) -> void
		{
			this->m_timescale = t;
		});

	// max. time stepping
	connect(simwidget, &SimPropertiesWidget::MaxTimeStepChanged,
		[this](t_int dt) -> void
		{
			this->m_maxtimestep = dt;
		});

	// selection plane normal
	connect(selwidget, &SelectionPropertiesWidget::PlaneNormChanged,
		[this](t_real _x, t_real _y, t_real _z) -> void
		{
			t_real_gl x = t_real_gl(_x);
			t_real_gl y = t_real_gl(_y);
			t_real_gl z = t_real_gl(_z);

			if(m_renderer)
			{
				m_renderer->SetSelectionPlaneNorm(
					m::create<t_vec3_gl>({x, y, z}));
			}
		});

	// selection plane distance
	connect(selwidget, &SelectionPropertiesWidget::PlaneDistChanged,
		[this](t_real d) -> void
		{
			if(m_renderer)
				m_renderer->SetSelectionPlaneDist(t_real_gl(d));
		});

	// selection plane visibility
	connect(selwidget, &SelectionPropertiesWidget::PlaneVisibilityChanged,
		[this](bool visible) -> void
		{
			if(m_renderer)
				m_renderer->SetSelectionPlaneVisible(visible);
		});

	// mouse drag mode changed
	connect(selwidget, &SelectionPropertiesWidget::MouseDragModeChanged,
		[this](MouseDragMode mode) -> void
		{
			m_mousedragmode = mode;
		});
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// menu bar
	// --------------------------------------------------------------------
	m_menubar = new QMenuBar(this);


	// file menu
	QMenu *menuFile = new QMenu("File", m_menubar);

	QAction *actionNew = new QAction(QIcon::fromTheme("document-new"), "New", menuFile);
	QAction *actionOpen = new QAction(QIcon::fromTheme("document-open"), "Open...", menuFile);
	QAction *actionSave = new QAction(QIcon::fromTheme("document-save"), "Save", menuFile);
	QAction *actionSaveAs = new QAction(QIcon::fromTheme("document-save-as"), "Save As...", menuFile);
	QAction *actionScreenshot = new QAction(QIcon::fromTheme("image-x-generic"), "Save Screenshot...", menuFile);
	QAction *actionQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", menuFile);

	// recent files menu
	m_menuOpenRecent = std::make_shared<QMenu>("Open Recent", menuFile);
	m_menuOpenRecent->setIcon(QIcon::fromTheme("document-open-recent"));

        // restore recent files menu
	m_recent.SetRecentMenu(m_menuOpenRecent);
        m_recent.CreateRecentFileMenu(m_open_func);

#if BOOST_OS_MACOS
	m_recent.AddForbiddenDir("/Applications");
//#elif BOOST_OS_LINUX
//	m_recent.AddForbiddenDir("/usr");
#endif
	if(g_appdirpath)
		m_recent.AddForbiddenDir(g_appdirpath->c_str());

	actionQuit->setMenuRole(QAction::QuitRole);

	// connections
	connect(actionNew, &QAction::triggered, this, [this]()
	{
		//this->NewFile();
		this->LoadInitialSceneFile();
	});

	connect(actionOpen, &QAction::triggered, this, [this]()
	{
		this->OpenFile();
	});

	connect(actionSave, &QAction::triggered, this, [this]()
	{
		this->SaveFile();
	});

	connect(actionSaveAs, &QAction::triggered, this, [this]()
	{
		this->SaveFileAs();
	});

	connect(actionScreenshot, &QAction::triggered, this, [this]()
	{
		this->SaveScreenshot();
	});

	connect(actionQuit, &QAction::triggered, this, &MainWnd::close);


	menuFile->addAction(actionNew);
	menuFile->addSeparator();
	menuFile->addAction(actionOpen);
	menuFile->addMenu(m_recent.GetRecentMenu().get());
	menuFile->addSeparator();
	menuFile->addAction(actionSave);
	menuFile->addAction(actionSaveAs);
	menuFile->addSeparator();
	menuFile->addAction(actionScreenshot);
	menuFile->addSeparator();
	menuFile->addAction(actionQuit);


	// window menu
	QMenu *menuWindow = new QMenu("Window", m_menubar);

	QAction *acHideAllDocks = new QAction("Hide All Dock Widgets", menuFile);
	QAction *acShowAllDocks = new QAction("Show All Dock Widgets", menuFile);
	QAction *acRestoreState = new QAction("Restore Layout", menuFile);

	menuWindow->addAction(m_camProperties->toggleViewAction());
	menuWindow->addAction(m_simProperties->toggleViewAction());
	menuWindow->addAction(m_selProperties->toggleViewAction());
	menuWindow->addSeparator();
	menuWindow->addAction(acHideAllDocks);
	menuWindow->addAction(acShowAllDocks);
	menuWindow->addSeparator();
	menuWindow->addAction(acRestoreState);
	//menuWindow->addAction(acPersp);

	// connections
	connect(acHideAllDocks, &QAction::triggered, [this]() -> void
	{
		for(QDockWidget* dock : std::initializer_list<QDockWidget*>
		{
			m_camProperties.get(), m_simProperties.get(), m_selProperties.get()
		})
		{
			dock->hide();
		}
	});

	connect(acShowAllDocks, &QAction::triggered, [this]() -> void
	{
		for(QDockWidget* dock : std::initializer_list<QDockWidget*>
		{
			m_camProperties.get(), m_simProperties.get(), m_selProperties.get()
		})
		{
			dock->show();
		}
	});

	connect(acRestoreState, &QAction::triggered, [this]() -> void
	{
		this->restoreState(m_initial_state);
	});


	// geometry menu
	QMenu *menuGeo = new QMenu("Geometry", m_menubar);

	QAction *actionAddPlane = new QAction(
		QIcon::fromTheme("insert-object"), "Add Plane", menuGeo);
	QAction *actionAddCuboid = new QAction(
		QIcon::fromTheme("insert-object"), "Add Cube", menuGeo);
	QAction *actionAddSphere = new QAction(
		QIcon::fromTheme("insert-object"), "Add Sphere", menuGeo);
	QAction *actionAddCylinder = new QAction(
		QIcon::fromTheme("insert-object"), "Add Cylinder", menuGeo);
	QAction *actionAddTetrahedron = new QAction(
		QIcon::fromTheme("insert-object"), "Add Tetrahedron", menuGeo);
	QAction *actionAddOctahedron = new QAction(
		QIcon::fromTheme("insert-object"), "Add Octahedron", menuGeo);
	QAction *actionAddDodecahedron = new QAction(
		QIcon::fromTheme("insert-object"), "Add Dodecahedron", menuGeo);
	QAction *actionAddIcosahedron = new QAction(
		QIcon::fromTheme("insert-object"), "Add Icosahedron", menuGeo);

	QAction *actionGeoBrowser = new QAction(
		QIcon::fromTheme("document-properties"), "Object Browser...", menuGeo);
	QAction *actionTextureBrowser = new QAction(
		QIcon::fromTheme("image-x-generic"), "Texture Browser...", menuGeo);

	connect(actionAddPlane, &QAction::triggered, this, &MainWnd::AddPlane);
	connect(actionAddCuboid, &QAction::triggered, this, &MainWnd::AddCuboid);
	connect(actionAddSphere, &QAction::triggered, this, &MainWnd::AddSphere);
	connect(actionAddCylinder, &QAction::triggered, this, &MainWnd::AddCylinder);
	connect(actionAddTetrahedron, &QAction::triggered, this, &MainWnd::AddTetrahedron);
	connect(actionAddOctahedron, &QAction::triggered, this, &MainWnd::AddOctahedron);
	connect(actionAddDodecahedron, &QAction::triggered, this, &MainWnd::AddDodecahedron);
	connect(actionAddIcosahedron, &QAction::triggered, this, &MainWnd::AddIcosahedron);
	connect(actionGeoBrowser, &QAction::triggered, this, &MainWnd::ShowGeometryBrowser);
	connect(actionTextureBrowser, &QAction::triggered, this, &MainWnd::ShowTextureBrowser);

	menuGeo->addAction(actionAddPlane);
	menuGeo->addAction(actionAddCuboid);
	menuGeo->addAction(actionAddSphere);
	menuGeo->addAction(actionAddCylinder);
	menuGeo->addAction(actionAddTetrahedron);
	menuGeo->addAction(actionAddOctahedron);
	//menuGeo->addAction(actionAddDodecahedron);  // TODO
	menuGeo->addAction(actionAddIcosahedron);
	menuGeo->addSeparator();
	menuGeo->addAction(actionGeoBrowser);
	menuGeo->addAction(actionTextureBrowser);


	// tools menu
	QMenu *menuTools = new QMenu("Tools", m_menubar);


	QAction *actionTrafoCalculator = new QAction(
		QIcon::fromTheme("accessories-calculator"),
		"Transformation Calculator...", menuTools);

	connect(actionTrafoCalculator, &QAction::triggered, this, &MainWnd::ShowTrafoCalculator);

	menuTools->addAction(actionTrafoCalculator);


	// settings menu
	QMenu *menuSettings = new QMenu("Settings", m_menubar);

	QAction *actionGarbage = new QAction(QIcon::fromTheme("user-trash-full"), "Collect Garbage", menuSettings);
	QAction *actionClearSettings = new QAction("Clear Settings File", menuSettings);
	QAction *actionSettings = new QAction(QIcon::fromTheme("preferences-system"), "Preferences...", menuSettings);

	actionSettings->setMenuRole(QAction::PreferencesRole);

	// collect garbage
	connect(actionGarbage, &QAction::triggered, this, &MainWnd::CollectGarbage);

	// clear settings
	connect(actionClearSettings, &QAction::triggered,
	[this]()
	{
		//restoreState(m_initial_state);
		m_sett.clear();
	});

	// show settings dialog
	connect(actionSettings, &QAction::triggered, this, [this]()
	{
		if(!this->m_dlgSettings)
		{
			auto settingsDlg = std::make_shared<
				t_SettingsDlg>(this, &m_sett);
			this->m_dlgSettings = settingsDlg;

			settingsDlg->AddChangedSettingsSlot(
				[this](){ MainWnd::InitSettings(); });
		}

		// sequence to show the dialog,
		// see: https://doc.qt.io/qt-5/qdialog.html#code-examples
		m_dlgSettings->show();
		m_dlgSettings->raise();
		m_dlgSettings->activateWindow();
	});

	menuSettings->addAction(actionSettings);
	menuSettings->addSeparator();
	menuSettings->addAction(actionGarbage);
	menuSettings->addAction(actionClearSettings);


	// help menu
	QMenu *menuHelp = new QMenu("Help", m_menubar);

	QAction *actionBug = new QAction("Report Bug...", menuHelp);
	QAction *actionAboutGl = new QAction(QIcon::fromTheme("help-about"), "About Renderer...", menuHelp);
	QAction *actionAbout = new QAction(QIcon::fromTheme("help-about"), "About " APPL_TITLE "...", menuHelp);

	actionAbout->setMenuRole(QAction::AboutRole);

	// show infos about renderer hardware
	connect(actionAboutGl, &QAction::triggered, this, [this]()
	{
		std::ostringstream ostrInfo;
		ostrInfo << "Requested rendering APIs:\n";
		ostrInfo << "    GL Version: " << m_gl_api_ver << "\n";
		ostrInfo << "    GLSL Version: " << m_glsl_api_ver << "\n";

		ostrInfo << "\nRendering using the following device:\n";
		ostrInfo << "    GL Vendor: " << m_gl_vendor << "\n";
		ostrInfo << "    GL Renderer: " << m_gl_renderer << "\n";
		ostrInfo << "    GL Version: " << m_gl_ver << "\n";
		ostrInfo << "    GL Shader Version: " << m_gl_shader_ver << "\n";
		ostrInfo << "    Device Pixel Ratio: " << devicePixelRatio() << "\n";

		QMessageBox::information(this, "About Renderer", ostrInfo.str().c_str());
	});

	// show about dialog
	connect(actionAbout, &QAction::triggered, this, [this]()
	{
		if(!this->m_dlgAbout)
		{
			QIcon icon = windowIcon();
			this->m_dlgAbout = std::make_shared<About>(this, &icon);
		}

		m_dlgAbout->show();
		m_dlgAbout->raise();
		m_dlgAbout->activateWindow();
	});

	// go to bug report url
	connect(actionBug, &QAction::triggered, this, [this]()
	{
		QUrl url("https://github.com/t-weber/gl/issues");
		if(!QDesktopServices::openUrl(url))
			QMessageBox::critical(this, "Error", "Could not open bug report website.");
	});

	menuHelp->addAction(actionBug);
	menuHelp->addSeparator();
	menuHelp->addAction(actionAboutGl);
	menuHelp->addAction(actionAbout);


	// shortcuts
	actionNew->setShortcut(QKeySequence::New);
	actionOpen->setShortcut(QKeySequence::Open);
	actionSave->setShortcut(QKeySequence::Save);
	actionSaveAs->setShortcut(QKeySequence::SaveAs);
	actionSettings->setShortcut(QKeySequence::Preferences);
	actionQuit->setShortcut(QKeySequence::Quit);
	actionGeoBrowser->setShortcut(int(Qt::CTRL) | int(Qt::Key_B));
	actionTextureBrowser->setShortcut(int(Qt::CTRL) | int(Qt::Key_T));


	// menu bar
	m_menubar->addMenu(menuFile);
	m_menubar->addMenu(menuGeo);
	m_menubar->addMenu(menuTools);
	m_menubar->addMenu(menuWindow);
	m_menubar->addMenu(menuSettings);
	m_menubar->addMenu(menuHelp);
	//m_menubar->setNativeMenuBar(false);
	setMenuBar(m_menubar);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// context menu
	// --------------------------------------------------------------------
	m_contextMenuObj = new QMenu(this);

	QAction *actionObjRotXP10 = new QAction(QIcon::fromTheme("object-rotate-left"), "Rotate Object by +10° around x", m_contextMenuObj);
	QAction *actionObjRotXM10 = new QAction(QIcon::fromTheme("object-rotate-right"), "Rotate Object by -10° around x", m_contextMenuObj);
	QAction *actionObjRotYP10 = new QAction(QIcon::fromTheme("object-rotate-left"), "Rotate Object by +10° around y", m_contextMenuObj);
	QAction *actionObjRotYM10 = new QAction(QIcon::fromTheme("object-rotate-right"), "Rotate Object by -10° around y", m_contextMenuObj);
	QAction *actionObjRotZP10 = new QAction(QIcon::fromTheme("object-rotate-left"), "Rotate Object by +10° around z", m_contextMenuObj);
	QAction *actionObjRotZM10 = new QAction(QIcon::fromTheme("object-rotate-right"), "Rotate Object by -10° around z", m_contextMenuObj);
	QAction *actionObjCentreCam = new QAction(QIcon::fromTheme("camera-video"), "Centre Camera on Object", m_contextMenuObj);
	QAction *actionObjDel = new QAction(QIcon::fromTheme("edit-delete"), "Delete Object", m_contextMenuObj);
	QAction *actionObjClone = new QAction(QIcon::fromTheme("edit-copy"), "Clone Object", m_contextMenuObj);
	QAction *actionObjProp = new QAction(QIcon::fromTheme("document-properties"), "Object Properties...", m_contextMenuObj);

	m_contextMenuObj->addAction(actionObjRotXP10);
	m_contextMenuObj->addAction(actionObjRotXM10);
	m_contextMenuObj->addAction(actionObjRotYP10);
	m_contextMenuObj->addAction(actionObjRotYM10);
	m_contextMenuObj->addAction(actionObjRotZP10);
	m_contextMenuObj->addAction(actionObjRotZM10);
	m_contextMenuObj->addSeparator();
	m_contextMenuObj->addAction(actionObjCentreCam);
	m_contextMenuObj->addSeparator();
	m_contextMenuObj->addAction(actionObjDel);
	m_contextMenuObj->addAction(actionObjClone);
	m_contextMenuObj->addAction(actionObjProp);

	connect(actionObjRotXP10, &QAction::triggered,
		[this]() { RotateCurrentObject(10./180.*m::pi<t_real>, 'x'); });
	connect(actionObjRotXM10, &QAction::triggered,
		[this]() { RotateCurrentObject(-10./180.*m::pi<t_real>, 'x'); });
	connect(actionObjRotYP10, &QAction::triggered,
		[this]() { RotateCurrentObject(10./180.*m::pi<t_real>, 'y'); });
	connect(actionObjRotYM10, &QAction::triggered,
		[this]() { RotateCurrentObject(-10./180.*m::pi<t_real>, 'y'); });
	connect(actionObjRotZP10, &QAction::triggered,
		[this]() { RotateCurrentObject(10./180.*m::pi<t_real>, 'z'); });
	connect(actionObjRotZM10, &QAction::triggered,
		[this]() { RotateCurrentObject(-10./180.*m::pi<t_real>, 'z'); });
	connect(actionObjDel, &QAction::triggered, this,
		&MainWnd::DeleteCurrentObject);
	connect(actionObjClone, &QAction::triggered, this,
		&MainWnd::CloneCurrentObject);
	connect(actionObjProp, &QAction::triggered, this,
		&MainWnd::ShowCurrentObjectProperties);
	connect(actionObjCentreCam, &QAction::triggered,
		[this]() { if(m_renderer) m_renderer->CentreCam(m_curContextObj); });
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// status bar
	// --------------------------------------------------------------------
	m_labelStatus = new QLabel(this);
	m_labelStatus->setSizePolicy(QSizePolicy::/*Ignored*/Expanding, QSizePolicy::Fixed);
	m_labelStatus->setFrameStyle(int(QFrame::Sunken) | int(QFrame::Panel));
	m_labelStatus->setLineWidth(1);

	m_statusbar = new QStatusBar(this);
	m_statusbar->setSizeGripEnabled(true);
	m_statusbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_statusbar->addPermanentWidget(m_labelStatus);
	setStatusBar(m_statusbar);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// restore window size, position, and state
	// --------------------------------------------------------------------
	// save initial state
	m_initial_state = saveState();

	if(m_sett.contains("geo"))
		restoreGeometry(m_sett.value("geo").toByteArray());
	else
		resize(1500, 1000);

	if(m_sett.contains("state"))
		restoreState(m_sett.value("state").toByteArray());

	// recent files
	if(m_sett.contains("recent_files"))
		m_recent.SetRecentFiles(m_sett.value("recent_files").toStringList());
	if(m_sett.contains("recent_files_dir"))
		m_recent.SetRecentDir(m_sett.value("recent_files_dir").toString());

	//QFont font = this->font();
	//font.setPointSizeF(14.);
	//this->setFont(font);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// initialisations
	// --------------------------------------------------------------------
	InitSettings();
	// --------------------------------------------------------------------

	setAcceptDrops(true);


	// timer callback function
	connect(&m_timer, &QTimer::timeout, [this]()
	{
		this->tick(std::chrono::milliseconds(1000 / g_timer_tps));
	});

	EnableTimer(true);
}


/**
 * timer ticks
 */
void MainWnd::tick(const std::chrono::milliseconds& ms)
{
	// advance simulation
	using t_val = decltype(ms.count());
	t_val ms_total = t_val(t_real(ms.count()) * m_timescale);

	t_val num_steps = ms_total / m_maxtimestep;
	t_val ms_step = num_steps ? ms_total / num_steps : 0;

	// steps
	t_val ms_cur_val = 0;
	for(t_int step = 0; step < num_steps; ++step)
	{
		std::chrono::milliseconds ms_sim(ms_step);
		m_scene.tick(ms_sim);

		ms_cur_val += ms_step;
	}

	// rest
	if(ms_cur_val < ms_total)
	{
		std::chrono::milliseconds ms_sim(ms_total - ms_cur_val);
		m_scene.tick(ms_sim);
	}

	//std::cout << num_steps << "*" << ms_step << " + " << ms_total - ms_cur_val << std::endl;


	// advance renderer
	if(m_renderer)
		m_renderer->tick(ms);
}


/**
 * enable (or disable) timer ticks
 */
void MainWnd::EnableTimer(bool enabled)
{
	if(enabled)
		m_timer.start(std::chrono::milliseconds(1000 / g_timer_tps));
	else
		m_timer.stop();
}


/**
 * the window is being shown
 */
void MainWnd::showEvent(QShowEvent *evt)
{
	EnableTimer(true);
	QMainWindow::showEvent(evt);
}


/**
 * the window is being hidden
 */
void MainWnd::hideEvent(QHideEvent *evt)
{
	EnableTimer(false);
	QMainWindow::hideEvent(evt);
}


/**
 * the window is being closed
 */
void MainWnd::closeEvent(QCloseEvent *evt)
{
	CollectGarbage();

	// save window size, position, and state
	m_sett.setValue("geo", saveGeometry());
	m_sett.setValue("state", saveState());
	m_sett.setValue("recent_files", m_recent.GetRecentFiles());
	m_sett.setValue("recent_files_dir", m_recent.GetRecentDir());

	QMainWindow::closeEvent(evt);
}


/**
 * clear all allocated objects
 */
void MainWnd::CollectGarbage()
{
	// remove any open dialogs
	if(m_dlgSettings)
		m_dlgSettings.reset();
	if(m_dlgGeoBrowser)
		m_dlgGeoBrowser.reset();
	if(m_dlgTextureBrowser)
		m_dlgTextureBrowser.reset();
	if(m_dlgTrafoCalculator)
		m_dlgTrafoCalculator.reset();
	if(m_dlgAbout)
		m_dlgAbout.reset();
}


/**
 * accept a file dropped onto the main window
 * @see https://doc.qt.io/qt-5/dnd.html
 */
void MainWnd::dragEnterEvent(QDragEnterEvent *evt)
{
	// accept urls
	if(evt->mimeData()->hasUrls())
		evt->accept();

	QMainWindow::dragEnterEvent(evt);
}


/**
 * accept a file dropped onto the main window
 * @see https://doc.qt.io/qt-5/dnd.html
 */
void MainWnd::dropEvent(QDropEvent *evt)
{
	// get mime data dropped on the main window
	if(const QMimeData* dat = evt->mimeData(); dat && dat->hasUrls())
	{
		// get the list of urls dropped on the main window
		if(QList<QUrl> urls = dat->urls(); urls.size() > 0)
		{
			// use the first url for the file name
			QString filename = urls.begin()->path();

			// load the dropped file
			if(QFile::exists(filename))
				OpenFile(filename);
		}
	}

	QMainWindow::dropEvent(evt);
}



/**
 * File -> New
 */
void MainWnd::NewFile()
{
	SetCurrentFile("");
	m_scene.Clear();

	UpdateGeoTrees();

	if(m_dlgTextureBrowser)
		m_dlgTextureBrowser->DeleteTextures();
	if(m_renderer)
		m_renderer->LoadScene(m_scene);
}


/**
 * File -> Open
 */
void MainWnd::OpenFile()
{
	QString dirLast = m_sett.value("cur_dir",
		g_docpath.c_str()).toString();

	QFileDialog filedlg(this, "Open Scene File", dirLast,
		"Gl Scene Files (*.glscene)");
	filedlg.setAcceptMode(QFileDialog::AcceptOpen);
	filedlg.setDefaultSuffix("glscene");
	filedlg.setViewMode(QFileDialog::Detail);
	filedlg.setFileMode(QFileDialog::ExistingFile);
	filedlg.setSidebarUrls(QList<QUrl>({
		QUrl::fromLocalFile(g_homepath.c_str()),
		QUrl::fromLocalFile(g_desktoppath.c_str()),
		QUrl::fromLocalFile(g_docpath.c_str())}));

	if(!filedlg.exec())
		return;

	QStringList files = filedlg.selectedFiles();
	if(!files.size() || files[0]=="" || !QFile::exists(files[0]))
		return;

	if(OpenFile(files[0]))
		m_sett.setValue("cur_dir", QFileInfo(files[0]).path());
}


/**
 * File -> Save
 */
void MainWnd::SaveFile()
{
	if(m_recent.GetOpenFile() == "")
		SaveFileAs();
	else
		SaveFile(m_recent.GetOpenFile());
}


/**
 * File -> Save As
 */
void MainWnd::SaveFileAs()
{
	QString dirLast = m_sett.value("cur_dir",
		g_docpath.c_str()).toString();

	QFileDialog filedlg(this, "Save Scene File", dirLast,
		"Gl Scene Files (*.glscene)");
	filedlg.setAcceptMode(QFileDialog::AcceptSave);
	filedlg.setDefaultSuffix("glscene");
	filedlg.setFileMode(QFileDialog::AnyFile);
	filedlg.setViewMode(QFileDialog::Detail);
	filedlg.selectFile("untitled.glscene");
	filedlg.setSidebarUrls(QList<QUrl>({
		QUrl::fromLocalFile(g_homepath.c_str()),
		QUrl::fromLocalFile(g_desktoppath.c_str()),
		QUrl::fromLocalFile(g_docpath.c_str())}));

	if(!filedlg.exec())
		return;

	QStringList files = filedlg.selectedFiles();
	if(!files.size() || files[0]=="")
		return;

	if(SaveFile(files[0]))
		m_sett.setValue("cur_dir", QFileInfo(files[0]).path());
}


/**
 * File -> Save Screenshot
 */
void MainWnd::SaveScreenshot()
{
	QString dirLast = m_sett.value("cur_image_dir",
		g_imgpath.c_str()).toString();

	QFileDialog filedlg(this, "Save Screenshot", dirLast,
		"PNG Images (*.png);;JPEG Images (*.jpg)");
	filedlg.setAcceptMode(QFileDialog::AcceptSave);
	filedlg.setDefaultSuffix("png");
	filedlg.setViewMode(QFileDialog::Detail);
	filedlg.setFileMode(QFileDialog::AnyFile);
	filedlg.selectFile("glscene.png");
	filedlg.setSidebarUrls(QList<QUrl>({
		QUrl::fromLocalFile(g_homepath.c_str()),
		QUrl::fromLocalFile(g_desktoppath.c_str()),
		QUrl::fromLocalFile(g_imgpath.c_str()),
		QUrl::fromLocalFile(g_docpath.c_str())}));

	if(!filedlg.exec())
		return;

	QStringList files = filedlg.selectedFiles();
	if(!files.size() || files[0]=="")
		return;

	if(SaveScreenshot(files[0]))
		m_sett.setValue("cur_image_dir", QFileInfo(files[0]).path());
}


/**
 * load file
 */
bool MainWnd::OpenFile(const QString& file)
{
	// no file given
	if(file == "")
		return false;

	try
	{
		NewFile();

		// get property tree
		if(file == "" || !QFile::exists(file))
		{
			QMessageBox::critical(this, "Error",
				"Scene file \"" + file + "\" does not exist.");
			return false;
		}

		// open xml
		std::string filename = file.toStdString();
		std::ifstream ifstr{filename};
		if(!ifstr)
		{
			QMessageBox::critical(this, "Error",
				("Could not read scene file \"" + filename + "\".").c_str());
			return false;
		}

		// read xml
		pt::ptree prop;
		pt::read_xml(ifstr, prop);
		// check format and version
		if(auto opt = prop.get_optional<std::string>(FILE_BASENAME "ident");
			!opt || *opt != APPL_IDENT)
		{
			QMessageBox::critical(this, "Error",
				("Scene file \"" + filename +
				"\" has invalid identifier.").c_str());
			return false;
		}

		// load scene definition file
		if(auto [sceneok, msg] =
			Scene::load(prop, m_scene, &filename); !sceneok)
		{
			QMessageBox::critical(this, "Error", msg.c_str());
			return false;
		}
		else
		{
			std::ostringstream ostr;
			ostr << "Loaded \"" << QFileInfo{file}.fileName().toStdString() << "\" "
				<< "dated " << msg << ".";

			SetTmpStatus(ostr.str());
		}


		// load dock window settings
		if(auto prop_dock = prop.get_child_optional(FILE_BASENAME "configuration.camera"); prop_dock)
			m_camProperties->GetWidget()->Load(*prop_dock);
		if(auto prop_dock = prop.get_child_optional(FILE_BASENAME "configuration.simulation"); prop_dock)
			m_simProperties->GetWidget()->Load(*prop_dock);
		if(auto prop_dock = prop.get_child_optional(FILE_BASENAME "configuration.selection_plane"); prop_dock)
			m_selProperties->GetWidget()->Load(*prop_dock);


		SetCurrentFile(file);
		m_recent.AddRecentFile(file, m_open_func);

		UpdateGeoTrees();

		if(m_renderer)
			m_renderer->LoadScene(m_scene);


		// load texture list
		if(m_dlgTextureBrowser)
			m_dlgTextureBrowser->DeleteTextures();

		if(auto textures = prop.get_child_optional(FILE_BASENAME "configuration.textures"); textures)
		{
			// iterate individual texture descriptors
			for(const auto &texture : *textures)
			{
				auto id = texture.second.get<std::string>("<xmlattr>.id", "");
				auto filename = texture.second.get<std::string>("filename", "");
				if(id == "" || filename == "")
					continue;

				if(m_renderer)
					m_renderer->ChangeTextureProperty(
						id.c_str(), filename.c_str());
				if(m_dlgTextureBrowser)
					m_dlgTextureBrowser->ChangeTexture(
						id.c_str(), filename.c_str(), false);
			}
		}

		bool textures_enabled = prop.get<bool>(
			FILE_BASENAME "configuration.textures.<xmlattr>.enabled", false);
		if(m_renderer)
			m_renderer->EnableTextures(textures_enabled);
		if(m_dlgTextureBrowser)
			m_dlgTextureBrowser->EnableTextures(textures_enabled, false);

		// update slot for scene space (e.g. objects) changes
		m_scene.AddUpdateSlot(
			[this](const Scene& scene)
			{
				if(m_renderer)
					m_renderer->UpdateScene(scene);
			});
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Error",
			QString{"Scene configuration error: "} + ex.what() + QString{"."});
		return false;
	}
	return true;
}


/**
 * save file
 */
bool MainWnd::SaveFile(const QString &file)
{
	if(file=="")
		return false;

	// save scene space configuration
	pt::ptree prop = m_scene.Save();

	// save dock window settings
	prop.put_child(FILE_BASENAME "configuration.camera", m_camProperties->GetWidget()->Save());
	prop.put_child(FILE_BASENAME "configuration.simulation", m_simProperties->GetWidget()->Save());
	prop.put_child(FILE_BASENAME "configuration.selection_plane", m_selProperties->GetWidget()->Save());

	// set format and version
	prop.put(FILE_BASENAME "ident", APPL_IDENT);
	prop.put(FILE_BASENAME "doi", "https://doi.org/10.5281/zenodo.5841951");
	prop.put(FILE_BASENAME "timestamp", std::to_string(
		std::chrono::system_clock::now().time_since_epoch().count()));

	// save texture list
	if(m_renderer)
	{
		boost::property_tree::ptree prop_textures;

		const GlSceneRenderer::t_textures& txts = m_renderer->GetTextures();
		for(const auto& pair : txts)
		{
			pt::ptree prop_texture;
			prop_texture.put<std::string>("<xmlattr>.id", pair.first);
			prop_texture.put<std::string>("filename", pair.second.filename);

			pt::ptree prop_texture2;
			prop_texture2.put_child("texture", prop_texture);
			prop_textures.insert(prop_textures.end(), prop_texture2.begin(), prop_texture2.end());
		}

		prop_textures.put<bool>("<xmlattr>.enabled", m_renderer->AreTexturesEnabled());
		prop.put_child(FILE_BASENAME "configuration.textures", prop_textures);
	}

	std::string filename = file.toStdString();
	std::ofstream ofstr{filename};
	ofstr.precision(g_prec);
	if(!ofstr)
	{
		QMessageBox::critical(this, "Error",
			("Could not save scene file \"" + filename + "\".").c_str());
		return false;
	}

	pt::write_xml(ofstr, prop,
		pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));

	SetCurrentFile(file);
	m_recent.AddRecentFile(file, m_open_func);
	return true;
}


/**
 * save screenshot
 */
bool MainWnd::SaveScreenshot(const QString &file)
{
	if(file=="" || !m_renderer)
		return false;

	QImage img = m_renderer->grabFramebuffer();
	return img.save(file, nullptr, 90);
}


void MainWnd::UpdateGeoTrees()
{
	// update object browser tree
	if(m_dlgGeoBrowser)
		m_dlgGeoBrowser->UpdateGeoTree(m_scene);
	if(m_dlgTrafoCalculator)
		m_dlgTrafoCalculator->UpdateGeoTree(m_scene);
}


/**
 * remember current file and set window title
 */
void MainWnd::SetCurrentFile(const QString &file)
{
	static const QString title(APPL_TITLE);

	if(file != "")
	{
		fs::path path{file.toStdString()};
		m_recent.SetRecentDir(path.parent_path().string().c_str());
	}
	m_recent.SetOpenFile(file);

	this->setWindowFilePath(m_recent.GetOpenFile());

	if(m_recent.GetOpenFile() == "")
		this->setWindowTitle(title);
	else
		this->setWindowTitle(title + " \u2014 " + m_recent.GetOpenFile());
}


/**
 * called after the plotter has initialised
 */
void MainWnd::AfterGLInitialisation()
{
	if(!m_renderer)
		return;

	// GL device info
	std::tie(m_gl_api_ver, m_glsl_api_ver,
		m_gl_ver, m_gl_shader_ver,
		m_gl_vendor, m_gl_renderer)
			= m_renderer->GetGlDescription();

	// get camera fov
	t_real viewingAngle = m_renderer->GetCamera().GetFOV();
	m_camProperties->GetWidget()->SetViewingAngle(
		viewingAngle*t_real{180}/m::pi<t_real>);

	// get camera zoom
	t_real zoom = m_renderer->GetCamera().GetZoom();
	m_camProperties->GetWidget()->SetZoom(zoom);

	// get perspective projection flag
	bool persp = m_renderer->GetCamera().GetPerspectiveProjection();
	m_camProperties->GetWidget()->SetPerspectiveProj(persp);

	// get camera position
	t_vec3_gl campos = m_renderer->GetCamera().GetPosition();
	m_camProperties->GetWidget()->SetPosition(campos[0], campos[1], campos[2]);

	// get camera rotation
	auto [phi, theta] = m_renderer->GetCamera().GetRotation();
	t_vec2_gl camrot = m::create<t_vec2_gl>({phi, theta});
	m_camProperties->GetWidget()->SetRotation(
		t_real(camrot[0])*t_real{180}/m::pi<t_real>,
		t_real(camrot[1])*t_real{180}/m::pi<t_real>);

	// get initial selection plane values from the renderer
	const auto& plane_norm = m_renderer->GetSelectionPlaneNorm();

	m_selProperties->GetWidget()->SetPlaneVisibility(m_renderer->GetSelectionPlaneVisible());
	m_selProperties->GetWidget()->SetPlaneDist(m_renderer->GetSelectionPlaneDist());
	m_selProperties->GetWidget()->SetPlaneNorm(plane_norm[0], plane_norm[1], plane_norm[2]);

	LoadInitialSceneFile();
}


/**
 * load an initial scene definition
 */
bool MainWnd::LoadInitialSceneFile()
{
	bool ok = false;

	if(auto scenefile = g_res.FindFile(m_initialSceneFile);
		scenefile && !scenefile->empty())
	{
		if(ok = OpenFile(scenefile->c_str()); ok)
		{
			// don't consider the initial scene configuration as current file
			if(!m_initialSceneFileModified)
				SetCurrentFile("");

			if(m_renderer)
				m_renderer->LoadScene(m_scene);
		}
	}

	return ok;
}


/**
 * mouse coordinates on base plane
 */
void MainWnd::CursorCoordsChanged(t_real_gl x, t_real_gl y, t_real_gl z)
{
	m_mouseX = x;
	m_mouseY = y;
	m_mouseZ = z;
	UpdateStatusLabel();
}


/**
 * mouse is over an object
 */
void MainWnd::PickerIntersection([[maybe_unused]] const t_vec3_gl* pos, std::string obj_name)
{
	if(pos)
		m_curInters = *pos;
	m_curObj = obj_name;
	UpdateStatusLabel();
}


/**
 * clicked on an object in the scene
 */
void MainWnd::ObjectClicked(const std::string& obj,
	[[maybe_unused]] bool left, bool middle, bool right)
{
	if(!m_renderer || obj == "")
		return;

	// show context menu for object
	if(right)
	{
		m_curContextObj = obj;

		QPoint pos = m_renderer->GetMousePosition(true);
		pos.setX(pos.x() + 8);
		pos.setY(pos.y() + 8);
		m_contextMenuObj->popup(pos);
	}

	// centre scene around object
	if(middle)
	{
		m_renderer->CentreCam(obj);
	}
}


/**
 * dragging an object in the scene
 */
void MainWnd::ObjectDragged(bool drag_start, const std::string& objid)
{
	const std::shared_ptr<Geometry> obj = m_scene.FindObject(objid);

	if(!m_renderer || !obj)
		return;

	t_vec3_gl cursor;

	if(drag_start)
	{
		// set the selection plane distance to the dragged object's position
		const t_vec3_gl& plane_norm = m_renderer->GetSelectionPlaneNorm();
		t_vec3_gl proj = m::ortho_project<t_vec3_gl>(
			m_curInters, plane_norm, true);
		t_vec3_gl diff = m_curInters - proj;
		t_real_gl dist = m::norm<t_vec3_gl>(diff);
		if(m::inner<t_vec3_gl>(diff, plane_norm) < 0.)
			dist = -dist;

		//std::cout << "pos : " << m_curInters[0] << " " << m_curInters[1] << " " << m_curInters[2] << std::endl;
		//std::cout << "norm: " << plane_norm[0] << " " << plane_norm[1] << " " << plane_norm[2] << std::endl;
		//std::cout << "proj: " << proj[0] << " " << proj[1] << " " << proj[2] << std::endl;
		//std::cout << std::endl;

		m_renderer->SetSelectionPlaneDist(dist);
		if(m_selProperties)
			m_selProperties->GetWidget()->SetPlaneDist(dist);

		// get mouse cursor position on selection plane
		if(auto[inters_cursor, inters_type] = m_renderer->GetSelectionPlaneCursor(); inters_type)
		{
			cursor = inters_cursor;
			m_drag_start = m::convert<t_vec>(cursor);
		}
	}
	else
	{
		if(auto[inters_cursor, inters_type] = m_renderer->GetSelectionPlaneCursor(); inters_type)
			cursor = inters_cursor;
	}

	m_scene.DragObject(drag_start, objid,
		m_drag_start, m::convert<t_vec>(cursor),
		m_mousedragmode);


	// if the object is a light, set its new position
	if(obj->GetLightId() >= 0)
	{
		const t_vec& pos = obj->GetPosition();
		m_renderer->SetLight(obj->GetLightId(), m::convert<t_vec3_gl>(pos));
	}
}


/**
 * set temporary status message, by default for 2 seconds
 */
void MainWnd::SetTmpStatus(const std::string& msg, int msg_duration)
{
	if(!m_statusbar)
		return;

	if(thread() == QThread::currentThread())
	{
		m_statusbar->showMessage(msg.c_str(), msg_duration);
	}
	else
	{
		// alternate call via meta object when coming from another thread
		QMetaObject::invokeMethod(m_statusbar, "showMessage", Qt::QueuedConnection,
			Q_ARG(QString, msg.c_str()),
			Q_ARG(int, msg_duration)
		);
	}
}


/**
 * update permanent status message
 */
void MainWnd::UpdateStatusLabel()
{
	const t_real maxRange = 1e6;

	if(!std::isfinite(m_mouseX) || !std::isfinite(m_mouseY) || !std::isfinite(m_mouseZ))
		return;
	if(std::abs(m_mouseX) >= maxRange || std::abs(m_mouseY) >= maxRange || std::abs(m_mouseZ) >= maxRange)
		return;

	std::ostringstream ostr;
	ostr.precision(g_prec_gui);
	ostr << std::fixed << std::showpos
		<< "Cursor: (" << m_mouseX << ", " << m_mouseY << ", " << m_mouseZ << ")";

	// show object name
	//if(m_curObj != "")
	//	ostr << ", object: " << m_curObj;

	ostr << ".";
	m_labelStatus->setText(ostr.str().c_str());
}


/**
 * set the scene file to be initially loaded
 */
void MainWnd::SetInitialSceneFile(const std::string& file)
{
	m_initialSceneFile = file;
	m_initialSceneFileModified = true;
}


/**
 * propagate (changed) global settings to each object
 */
void MainWnd::InitSettings()
{
	m_scene.SetEpsilon(g_eps);

	QMainWindow::DockOptions dockoptions{};
	if(g_tabbed_docks)
		dockoptions |= QMainWindow::AllowTabbedDocks | QMainWindow::VerticalTabs;
	if(g_nested_docks)
		dockoptions |= QMainWindow::AllowNestedDocks;
	setDockOptions(dockoptions);

	setAnimated(g_use_animations != 0);

	if(m_renderer)
	{
		m_renderer->SetLightFollowsCursor(g_light_follows_cursor);
		m_renderer->EnableShadowRendering(g_enable_shadow_rendering);
		m_renderer->EnablePortalRendering(g_enable_portal_rendering);
	}
}


/**
 * add a plane to the scene
 */
void MainWnd::AddPlane()
{
	auto plane = std::make_shared<PlaneGeometry>();
	plane->SetWidth(2.);
	plane->SetHeight(2.);
	plane->SetPosition(m::create<t_vec>({0, 0, 0}));

	static std::size_t cnt = 1;
	std::ostringstream ostrId;
	ostrId << "plane " << cnt++;

	// add plane to scene
	m_scene.AddObject(std::vector<std::shared_ptr<Geometry>>{{plane}}, ostrId.str());

	UpdateGeoTrees();

	// add a 3d representation of the plane
	if(m_renderer)
		m_renderer->AddObject(*plane);
}


/**
 * add a cuboid to the scene
 */
void MainWnd::AddCuboid()
{
	auto cuboid = std::make_shared<BoxGeometry>();
	cuboid->SetHeight(2.);
	cuboid->SetDepth(2.);
	cuboid->SetLength(2.);
	cuboid->SetPosition(m::create<t_vec>({0, 0, cuboid->GetHeight()*0.5}));

	static std::size_t cuboidcnt = 1;
	std::ostringstream ostrId;
	ostrId << "cube " << cuboidcnt++;

	// add cuboid to scene
	m_scene.AddObject(std::vector<std::shared_ptr<Geometry>>{{cuboid}}, ostrId.str());

	UpdateGeoTrees();

	// add a 3d representation of the cuboid
	if(m_renderer)
		m_renderer->AddObject(*cuboid);
}


/**
 * add a sphere to the scene
 */
void MainWnd::AddSphere()
{
	auto sphere = std::make_shared<SphereGeometry>();
	sphere->SetRadius(1.);
	sphere->SetPosition(m::create<t_vec>({0, 0, sphere->GetRadius()}));

	static std::size_t sphcnt = 1;
	std::ostringstream ostrId;
	ostrId << "sphere " << sphcnt++;

	// add sphere to scene
	m_scene.AddObject(std::vector<std::shared_ptr<Geometry>>{{sphere}}, ostrId.str());

	UpdateGeoTrees();

	// add a 3d representation of the sphere
	if(m_renderer)
		m_renderer->AddObject(*sphere);
}


/**
 * add a cylinder to the scene
 */
void MainWnd::AddCylinder()
{
	auto cyl = std::make_shared<CylinderGeometry>();
	cyl->SetHeight(4.);
	cyl->SetPosition(m::create<t_vec>({0, 0, cyl->GetHeight()*0.5}));
	cyl->SetRadius(0.5);

	static std::size_t cylcnt = 1;
	std::ostringstream ostrId;
	ostrId << "cylinder " << cylcnt++;

	// add cylinder to scene
	m_scene.AddObject(std::vector<std::shared_ptr<Geometry>>{{cyl}}, ostrId.str());

	UpdateGeoTrees();

	// add a 3d representation of the cylinder
	if(m_renderer)
		m_renderer->AddObject(*cyl);
}


/**
 * add a tetrahedron to the scene
 */
void MainWnd::AddTetrahedron()
{
	auto tetr = std::make_shared<TetrahedronGeometry>();
	tetr->SetRadius(1.);
	tetr->SetPosition(m::create<t_vec>({0, 0, tetr->GetRadius()}));

	static std::size_t tetrcnt = 1;
	std::ostringstream ostrId;
	ostrId << "tetrahedron " << tetrcnt++;

	// add tetrahedron to scene
	m_scene.AddObject(std::vector<std::shared_ptr<Geometry>>{{tetr}}, ostrId.str());

	UpdateGeoTrees();

	// add a 3d representation of the tetrahedron
	if(m_renderer)
		m_renderer->AddObject(*tetr);
}


/**
 * add an octahedron to the scene
 */
void MainWnd::AddOctahedron()
{
	auto octa = std::make_shared<OctahedronGeometry>();
	octa->SetRadius(1.);
	octa->SetPosition(m::create<t_vec>({0, 0, octa->GetRadius()}));

	static std::size_t tetrcnt = 1;
	std::ostringstream ostrId;
	ostrId << "new octahedron " << tetrcnt++;

	// add octahedron to scene
	m_scene.AddObject(std::vector<std::shared_ptr<Geometry>>{{octa}}, ostrId.str());

	UpdateGeoTrees();

	// add a 3d representation of the octahedron
	if(m_renderer)
		m_renderer->AddObject(*octa);
}


/**
 * add an dodecahedron to the scene
 */
void MainWnd::AddDodecahedron()
{
	auto dode = std::make_shared<DodecahedronGeometry>();
	dode->SetRadius(1.);
	dode->SetPosition(m::create<t_vec>({0, 0, dode->GetRadius()}));

	static std::size_t tetrcnt = 1;
	std::ostringstream ostrId;
	ostrId << "dodecahedron " << tetrcnt++;

	// add dodecahedron to scene
	m_scene.AddObject(std::vector<std::shared_ptr<Geometry>>{{dode}}, ostrId.str());

	UpdateGeoTrees();

	// add a 3d representation of the dodecahedron
	if(m_renderer)
		m_renderer->AddObject(*dode);
}


/**
 * add an icosahedron to the scene
 */
void MainWnd::AddIcosahedron()
{
	auto icosa = std::make_shared<IcosahedronGeometry>();
	icosa->SetRadius(1.);
	icosa->SetPosition(m::create<t_vec>({0, 0, icosa->GetRadius()}));

	static std::size_t tetrcnt = 1;
	std::ostringstream ostrId;
	ostrId << "icosahedron " << tetrcnt++;

	// add icosahedron to scene
	m_scene.AddObject(std::vector<std::shared_ptr<Geometry>>{{icosa}}, ostrId.str());

	UpdateGeoTrees();

	// add a 3d representation of the icosahedron
	if(m_renderer)
		m_renderer->AddObject(*icosa);
}



/**
 * delete 3d object under the cursor
 */
void MainWnd::DeleteCurrentObject()
{
	DeleteObject(m_curContextObj);
}


/**
 * clone 3d object under the cursor
 */
void MainWnd::CloneCurrentObject()
{
	CloneObject(m_curContextObj);
}


/**
 * delete the given object from the scene
 */
void MainWnd::DeleteObject(const std::string& obj)
{
	if(obj == "")
		return;

	// remove object from scene
	if(m_scene.DeleteObject(obj))
	{
		UpdateGeoTrees();

		// remove 3d representation of object
		if(m_renderer)
			m_renderer->DeleteObject(obj);
	}
	else
	{
		QMessageBox::warning(this, "Warning",
			QString("Object \"") + obj.c_str() + QString("\" cannot be deleted."));
	}
}


/**
 * clone the given object from the scene
 */
void MainWnd::CloneObject(const std::string& obj)
{
	if(obj == "")
		return;

	// remove object from scene
	if(auto geo = m_scene.CloneObject(obj); geo)
	{
		UpdateGeoTrees();

		// add a 3d representation of the object
		if(m_renderer)
			m_renderer->AddObject(*geo);
	}
	else
	{
		QMessageBox::warning(this, "Warning",
			QString("Object \"") + obj.c_str() + QString("\" cannot be cloned."));
	}
}


/**
 * rotate 3d object under the cursor
 */
void MainWnd::RotateCurrentObject(t_real angle, char axis)
{
	RotateObject(m_curContextObj, angle, axis);
}


/**
 * rotate the given object
 */
void MainWnd::RotateObject(const std::string& objname, t_real angle, char axis)
{
	if(objname == "")
		return;

	// rotate the given object
	if(auto [ok, objgeo] = m_scene.RotateObject(objname, angle, axis); ok)
	{
		UpdateGeoTrees();

		// remove old 3d representation of object and create a new one
		if(m_renderer && objgeo)
		{
			m_renderer->DeleteObject(objname);
			m_renderer->AddObject(*objgeo);
		}
	}
	else
	{
		QMessageBox::warning(this, "Warning",
			QString("Object \"") + objname.c_str() + QString("\" cannot be rotated."));
	}
}


/**
 * open geometries browser and point to currently selected object
 */
void MainWnd::ShowCurrentObjectProperties()
{
	ShowGeometryBrowser();

	if(m_dlgGeoBrowser)
	{
		m_dlgGeoBrowser->SelectObject(m_curContextObj);
	}
}


/**
 * open the geometry browser dialog
 */
void MainWnd::ShowGeometryBrowser()
{
	if(!m_dlgGeoBrowser)
	{
		m_dlgGeoBrowser = std::make_shared<GeometriesBrowser>(this, &m_sett);

		connect(m_dlgGeoBrowser.get(), &GeometriesBrowser::SignalDeleteObject,
			this, &MainWnd::DeleteObject);
		connect(m_dlgGeoBrowser.get(), &GeometriesBrowser::SignalCloneObject,
			this, &MainWnd::CloneObject);
		connect(m_dlgGeoBrowser.get(), &GeometriesBrowser::SignalRenameObject,
			this, &MainWnd::RenameObject);
		connect(m_dlgGeoBrowser.get(), &GeometriesBrowser::SignalChangeObjectProperty,
			this, &MainWnd::ChangeObjectProperty);

		this->m_dlgGeoBrowser->UpdateGeoTree(this->m_scene);
	}

	m_dlgGeoBrowser->show();
	m_dlgGeoBrowser->raise();
	m_dlgGeoBrowser->activateWindow();
}


/**
 * open the texture browser dialog
 */
void MainWnd::ShowTextureBrowser()
{
	if(!m_dlgTextureBrowser)
	{
		m_dlgTextureBrowser = std::make_shared<TextureBrowser>(this, &m_sett);

		// get current texture list from renderer
		if(m_renderer)
		{
			const GlSceneRenderer::t_textures& txts = m_renderer->GetTextures();
			for(const auto& pair : txts)
			{
				m_dlgTextureBrowser->ChangeTexture(
					pair.first.c_str(),
					pair.second.filename.c_str(),
					false);
			}

			m_dlgTextureBrowser->EnableTextures(m_renderer->AreTexturesEnabled(), false);
		}

		connect(m_dlgTextureBrowser.get(), &TextureBrowser::SignalChangeTexture,
			m_renderer.get(), &GlSceneRenderer::ChangeTextureProperty);
		connect(m_dlgTextureBrowser.get(), &TextureBrowser::SignalEnableTextures,
			m_renderer.get(), &GlSceneRenderer::EnableTextures);
	}

	m_dlgTextureBrowser->show();
	m_dlgTextureBrowser->raise();
	m_dlgTextureBrowser->activateWindow();
}


/**
 * open the transformation calculator dialog
 */
void MainWnd::ShowTrafoCalculator()
{
	if(!m_dlgTrafoCalculator)
	{
		m_dlgTrafoCalculator = std::make_shared<TrafoCalculator>(this, &m_sett);

		this->m_dlgTrafoCalculator->UpdateGeoTree(this->m_scene);
	}

	m_dlgTrafoCalculator->show();
	m_dlgTrafoCalculator->raise();
	m_dlgTrafoCalculator->activateWindow();
}


/**
 * rename the given object in the scene
 */
void MainWnd::RenameObject(const std::string& oldid, const std::string& newid)
{
	if(oldid == "" || newid == "" || oldid == newid)
		return;

	if(m_scene.RenameObject(oldid, newid))
	{
		UpdateGeoTrees();

		// rename 3d representation of object
		if(m_renderer)
			m_renderer->RenameObject(oldid, newid);
	}
}


/**
 * change the properties of the given object in scene
 */
void MainWnd::ChangeObjectProperty(const std::string& objname, const ObjectProperty& prop)
{
	if(objname == "")
		return;

	// change object properties
	if(auto [ok, objgeo] = m_scene.SetProperties(objname, { prop} ); ok)
	{
		UpdateGeoTrees();

		// remove old 3d representation of object and create a new one
		if(m_renderer && objgeo)
		{
			m_renderer->DeleteObject(objname);
			m_renderer->AddObject(*objgeo);
		}
	}
	else
	{
		QMessageBox::warning(this, "Warning",
			QString("Properties of object \"") + objname.c_str() +
				QString("\" cannot be changed."));
	}
}
