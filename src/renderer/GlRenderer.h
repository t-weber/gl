/**
 * gl scene renderer
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @note Initially forked from my tlibs2 library (https://code.ill.fr/scientific-software/takin/tlibs2/-/blob/master/libs/qt/glplot.h).
 * @note Further code forked from my privately developed "misc" project (https://github.com/t-weber/misc).
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *   - http://doc.qt.io/qt-5/qopenglwidget.html#details
 *   - http://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/threadedqopenglwidget
 *   - http://doc.qt.io/qt-5/qtgui-openglwindow-example.html
 *   - http://doc.qt.io/qt-5/qopengltexture.html
 *   - (Sellers 2014) G. Sellers et al., ISBN: 978-0-321-90294-8 (2014).
 */

#ifndef __PATHS_RENDERER_H__
#define __PATHS_RENDERER_H__

#include <unordered_map>
#include <optional>

#include <QtCore/QTimer>
#include <QtWidgets/QDialog>
#include <QtGui/QMouseEvent>

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/cam.h"
#include "tlibs2/libs/qt/gl.h"

#include "src/Scene.h"


using t_real_gl = tl2::t_real_gl;
using t_vec2_gl = tl2::t_vec2_gl;
using t_vec3_gl = tl2::t_vec3_gl;
using t_vec_gl = tl2::t_vec_gl;
using t_mat_gl = tl2::t_mat_gl;



/**
 * rendering object structure
 */
struct PathsObj : public tl2::GlRenderObj
{
	t_mat_gl m_mat = tl2::unit<t_mat_gl>();

	bool m_visible = true;		// object shown?
	bool m_cull = true;		// object faces culled?

	t_vec3_gl m_boundingSpherePos = tl2::create<t_vec3_gl>({ 0., 0., 0. });
	t_real_gl m_boundingSphereRad = 0.;

	std::vector<t_vec_gl> m_boundingBox = {};

	std::string m_texture = "";	// texture identifier
};


/**
 * texture descriptor
 */
struct PathsTexture
{
	std::string filename{};
	std::shared_ptr<QOpenGLTexture> texture{};
};


/**
 * rendering widget
 */
class PathsRenderer : public QOpenGLWidget
{ Q_OBJECT
public:
	// camera type
	using t_cam = Camera<t_mat_gl, t_vec_gl, t_vec3_gl, t_real_gl>;

	// 3d object and texture types
	using t_objs = std::unordered_map<std::string, PathsObj>;
	using t_textures = std::unordered_map<std::string, PathsTexture>;


public:
	PathsRenderer(QWidget *pParent = nullptr);
	virtual ~PathsRenderer();

	PathsRenderer(const PathsRenderer&) = delete;
	const PathsRenderer& operator=(const PathsRenderer&) = delete;

	void Clear();
	bool LoadScene(const Scene& scene);
	void AddObject(const Geometry& geo);

	// receivers for scene update signals
	void UpdateScene(const Scene& instr);

	std::tuple<std::string, std::string, std::string, std::string> GetGlDescr() const;
	bool IsInitialised() const { return m_initialised; }

	void DeleteObject(const std::string& obj_name);
	void RenameObject(const std::string& oldname, const std::string& newname);

	t_objs::iterator AddTriangleObject(const std::string& obj_name,
		const std::vector<t_vec3_gl>& triag_verts,
		const std::vector<t_vec3_gl>& triag_norms,
		const std::vector<t_vec3_gl>& triag_uvs,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);

	void SetLight(std::size_t idx, const t_vec3_gl& pos);
	void SetLightFollowsCursor(bool b);
	void EnableShadowRendering(bool b);

	const t_cam& GetCamera() const { return m_cam; }
	t_cam& GetCamera() { return m_cam; }
	void CentreCam(const std::string& obj);

	void SetSelectionPlaneNorm(const t_vec3_gl& vec);
	void SetSelectionPlaneDist(t_real_gl d);
	const t_vec3_gl& GetSelectionPlaneNorm() const { return m_selectionPlaneNorm; }
	t_real_gl GetSelectionPlaneDist() const { return m_selectionPlaneDist; }

	std::tuple<t_vec3_gl, int> GetSelectionPlaneCursor() const;
	QPoint GetMousePosition(bool global_pos = false) const;

	void SaveShadowFramebuffer(const std::string& filename) const;

	bool AreTexturesEnabled() const { return m_textures_active; }
	const t_textures& GetTextures() const { return m_textures; }

	void UpdateCam(bool update_frame = true);


protected:
	virtual void paintEvent(QPaintEvent*) override;
	virtual void initializeGL() override;
	virtual void paintGL() override;
	virtual void resizeGL(int w, int h) override;

	virtual void mouseMoveEvent(QMouseEvent *pEvt) override;
	virtual void mousePressEvent(QMouseEvent *Evt) override;
	virtual void mouseReleaseEvent(QMouseEvent *Evt) override;
	virtual void wheelEvent(QWheelEvent *pEvt) override;
	virtual void keyPressEvent(QKeyEvent *pEvt) override;
	virtual void keyReleaseEvent(QKeyEvent *pEvt) override;

	qgl_funcs* GetGlFunctions();

	void UpdatePicker();
	void UpdateLights();
	void UpdateShadowFramebuffer();

	void DoPaintGL(qgl_funcs *pGL);
	void DoPaintQt(QPainter &painter);

	void tick(const std::chrono::milliseconds& ms);


private:
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	t_qt_mutex m_mutexObj{};
#else
	t_qt_mutex m_mutexObj{QMutex::Recursive};
#endif

	bool m_mouseMovedBetweenDownAndUp = false;
	bool m_mouseDown[3] = { 0, 0, 0 };
	bool m_arrowDown[4] = { 0, 0, 0, 0 };	// l, r, u, d
	bool m_pageDown[2] = { 0, 0 };
	bool m_bracketDown[2] = { 0, 0 };


protected:
	// ------------------------------------------------------------------------
	// shader interface
	// ------------------------------------------------------------------------
	std::shared_ptr<QOpenGLShaderProgram> m_shaders{};
	std::shared_ptr<QOpenGLFramebufferObject> m_fboshadow{};

	// vertex attributes
	GLint m_attrVertex = -1;
	GLint m_attrVertexNorm = -1;
	GLint m_attrVertexCol = -1;
	GLint m_attrTexCoords = -1;

	// texture
	GLint m_uniTextureActive = -1;
	GLint m_uniTexture = -1;

	// lighting
	GLint m_uniConstCol = -1;
	GLint m_uniLightPos = -1;
	GLint m_uniNumActiveLights = -1;
	GLint m_uniShadowMap = -1;
	GLint m_uniShadowRenderingEnabled = -1;
	GLint m_uniShadowRenderPass = -1;

	// matrices
	GLint m_uniMatrixProj = -1;
	GLint m_uniMatrixLightProj = -1;
	GLint m_uniMatrixCam = -1;
	GLint m_uniMatrixCamInv = -1;
	GLint m_uniMatrixLight = -1;
	GLint m_uniMatrixLightInv = -1;
	GLint m_uniMatrixObj = -1;
	// ------------------------------------------------------------------------

	// version identifiers
	std::string m_strGlVer{}, m_strGlShaderVer{},
		m_strGlVendor{}, m_strGlRenderer{};

	// object under cursor
	std::string m_curObj{}, m_draggedObj{};
	bool m_light_follows_cursor = false;

	// textures active?
	bool m_textures_active = false;

	// main camera
	t_cam m_cam{};

	// camera at light position
	t_cam m_lightcam{};

	std::atomic<bool> m_initialised = false;
	std::atomic<bool> m_pickerNeedsUpdate = false;
	std::atomic<bool> m_lightsNeedUpdate = true;
	std::atomic<bool> m_perspectiveNeedsUpdate = true;
	std::atomic<bool> m_viewportNeedsUpdate = true;
	std::atomic<bool> m_shadowFramebufferNeedsUpdate = false;
	std::atomic<bool> m_shadowRenderingEnabled = true;
	std::atomic<bool> m_shadowRenderPass = false;

	// 3d objects
	t_objs m_objs{};

	// lights
	std::vector<t_vec3_gl> m_lights{};

	// texture map
	t_textures m_textures{};

	// cursor
	t_vec3_gl m_selectionPlaneNorm = tl2::create<t_vec3_gl>({0, 0, 1});
	t_real_gl m_selectionPlaneDist = 0;
	QPointF m_posMouse{};
	QPointF m_posMouseRotationStart{}, m_posMouseRotationEnd{};
	bool m_inRotation = false;

	// timer
	QTimer m_timer{};


public slots:
	void EnableTimer(bool enable=true);

	void EnableTextures(bool b);
	bool ChangeTextureProperty(const QString& ident, const QString& filename);


signals:
	void AfterGLInitialisation();

	void ObjectClicked(const std::string& obj, bool left, bool mid, bool right);
	void ObjectDragged(bool drag_start, const std::string& obj);

	void CursorCoordsChanged(t_real_gl x, t_real_gl y, t_real_gl z);
	void PickerIntersection(const t_vec3_gl* pos, std::string obj_name);

	void CamPositionChanged(t_real_gl x, t_real_gl y, t_real_gl z);
	void CamRotationChanged(t_real_gl phi, t_real_gl theta);
	void CamZoomChanged(t_real_gl zoom);
};


#endif
