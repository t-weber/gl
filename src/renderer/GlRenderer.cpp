/**
 * gl scene renderer
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @note Initially forked from my tlibs2 library (https://code.ill.fr/scientific-software/takin/tlibs2/-/blob/master/libs/qt/glplot.cpp).
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

#include "GlRenderer.h"
#include "src/common/Resources.h"
#include "src/settings_variables.h"

#include <QtCore/QtGlobal>
#include <QtCore/QThread>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QPainter>
#include <QtGui/QGuiApplication>

#include <iostream>
#include <boost/range/combine.hpp>
#include <boost/scope_exit.hpp>

#include "tlibs2/libs/str.h"
#include "tlibs2/libs/file.h"


#define MAX_LIGHTS 4  // max. number allowed in shader


/**
 * initialise the renderer
 */
GlSceneRenderer::GlSceneRenderer(QWidget *pParent) : QOpenGLWidget(pParent)
{
	m_cam.SetDist(15.);

	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
}


/**
 * uninitialise the renderer clean up
 */
GlSceneRenderer::~GlSceneRenderer()
{
	setMouseTracking(false);
	Clear();

	// remove selection plane
	tl2::delete_render_object(m_selectionPlane);

	// delete gl objects within current gl context
	m_shaders.reset();
}


/**
 * renderer versions and driver descriptions
 */
std::tuple<std::string, std::string, std::string, std::string>
GlSceneRenderer::GetGlDescr() const
{
	return std::make_tuple(
		m_strGlVer, m_strGlShaderVer,
		m_strGlVendor, m_strGlRenderer);
}


/**
 * clear scene
 */
void GlSceneRenderer::Clear()
{
	BOOST_SCOPE_EXIT(this_)
	{
		this_->doneCurrent();
	} BOOST_SCOPE_EXIT_END
	makeCurrent();

	m_lights.clear();

	// clear objects
	QMutexLocker _locker{&m_mutexObj};
	for(auto &[obj_name, obj] : m_objs)
		tl2::delete_render_object(obj);
	m_objs.clear();

	// clear textures
	for(auto& txt : m_textures)
	{
		if(txt.second.texture)
		{
			txt.second.texture->destroy();
			txt.second.texture = nullptr;
		}
	}
	m_textures.clear();
}


/**
 * enable or disable texture mapping
 */
void GlSceneRenderer::EnableTextures(bool b)
{
	m_textures_active = b;
	update();
}


/**
 * add a texture image
 */
bool GlSceneRenderer::ChangeTextureProperty(
	const QString& ident, const QString& filename)
{
	if(!m_initialised)
		return false;

	BOOST_SCOPE_EXIT(this_)
	{
		this_->doneCurrent();
	} BOOST_SCOPE_EXIT_END
	makeCurrent();

	QMutexLocker _locker{&m_mutexObj};

	auto iter = m_textures.find(ident.toStdString());

	// remove texture
	if(filename == "")
	{
		if(iter != m_textures.end())
		{
			if(iter->second.texture)
				iter->second.texture->destroy();
			m_textures.erase(iter);

			return true;
		}
	}

	// add or replace texture
	else if(QImage image(filename); !image.isNull())
	{
		// insert new texture
		if(iter == m_textures.end())
		{
			GlSceneTexture txt
			{
				.filename = filename.toStdString(),
				.texture = std::make_shared<QOpenGLTexture>(image),
			};

			m_textures.emplace(std::make_pair(ident.toStdString(), txt));
		}

		// replace old texture
		else
		{
			if(iter->second.texture)
				iter->second.texture->destroy();

			iter->second.filename = filename.toStdString();
			iter->second.texture = std::make_shared<QOpenGLTexture>(image);
		}

		return true;
	}

	return false;
}


/**
 * create a 3d representation of the scene's objects
 */
bool GlSceneRenderer::LoadScene(const Scene& scene)
{
	if(!m_initialised)
		return false;

	Clear();

	// objects
	for(const auto& obj : scene.GetObjects())
	{
		if(!obj)
			continue;
		AddObject(*obj);
	}

	update();
	return true;
}


/**
 * calculate the bounding box and sphere
 */
template<class t_vec, template<class...> class t_cont = std::vector>
requires tl2::is_vec<t_vec>
static void create_bounding_objects(GlSceneObj& obj, const t_cont<t_vec>& triag_verts)
{
	// bounding sphere
	auto [boundingSpherePos, boundingSphereRad] =
		tl2::bounding_sphere<t_vec3_gl>(triag_verts);

	// bounding box
	auto [bbMin, bbMax] =
		tl2::bounding_box<t_vec3_gl>(triag_verts);

	// object bounding sphere
	obj.m_boundingSpherePos = std::move(boundingSpherePos);
	obj.m_boundingSphereRad = boundingSphereRad;

	// object bounding box
	obj.m_boundingBox.reserve(8);
	obj.m_boundingBox.push_back(tl2::create<t_vec_gl>({bbMin[0], bbMin[1], bbMin[2], 1.}));
	obj.m_boundingBox.push_back(tl2::create<t_vec_gl>({bbMin[0], bbMin[1], bbMax[2], 1.}));
	obj.m_boundingBox.push_back(tl2::create<t_vec_gl>({bbMin[0], bbMax[1], bbMin[2], 1.}));
	obj.m_boundingBox.push_back(tl2::create<t_vec_gl>({bbMin[0], bbMax[1], bbMax[2], 1.}));
	obj.m_boundingBox.push_back(tl2::create<t_vec_gl>({bbMax[0], bbMin[1], bbMin[2], 1.}));
	obj.m_boundingBox.push_back(tl2::create<t_vec_gl>({bbMax[0], bbMin[1], bbMax[2], 1.}));
	obj.m_boundingBox.push_back(tl2::create<t_vec_gl>({bbMax[0], bbMax[1], bbMin[2], 1.}));
	obj.m_boundingBox.push_back(tl2::create<t_vec_gl>({bbMax[0], bbMax[1], bbMax[2], 1.}));
}


/**
 * insert an object into the scene
 */
void GlSceneRenderer::AddObject(const Geometry& obj)
{
	if(!m_initialised)
		return;

	QMutexLocker _locker{&m_mutexObj};
	auto [_verts, _norms, _uvs] = obj.GetTriangles();

	auto verts = tl2::convert<t_vec3_gl>(_verts);
	auto norms = tl2::convert<t_vec3_gl>(_norms);
	auto uvs = tl2::convert<t_vec3_gl>(_uvs);
	auto cols = tl2::convert<t_vec3_gl>(obj.GetColour());

	auto obj_iter = AddTriangleObject(
		obj.GetId(), verts, norms, uvs,
		cols[0], cols[1], cols[2], 1);

	const t_mat& _mat = obj.GetTrafo();
	t_mat_gl mat = tl2::convert<t_mat_gl>(_mat);
	obj_iter->second.m_mat = mat;
	obj_iter->second.m_texture = obj.GetTexture();
	obj_iter->second.m_lighting = obj.IsLightingEnabled();
	obj_iter->second.m_portal_id = obj.GetPortalId();
	obj_iter->second.m_portal_mat = obj.GetPortalTrafo();

	if(obj.GetLightId() >= 0)
	{
		const t_vec& pos = obj.GetPosition();
		SetLight(obj.GetLightId(), tl2::convert<t_vec3_gl>(pos));
	}

	update();
}


/**
 * scene has been changed (e.g. objects have been moved)
 */
void GlSceneRenderer::UpdateScene(const Scene& scene)
{
	if(!m_initialised)
		return;

	//QMutexLocker _locker{&m_mutexObj};

	// update object matrices
	for(const auto& obj : scene.GetObjects())
	{
		m_objs[obj->GetId()].m_mat = obj->GetTrafo();
	}

	update();
}


/**
 * delete an object by name
 */
void GlSceneRenderer::DeleteObject(const std::string& obj_name)
{
	QMutexLocker _locker{&m_mutexObj};
	auto iter = m_objs.find(obj_name);

	if(iter != m_objs.end())
	{
		tl2::delete_render_object(iter->second);
		m_objs.erase(iter);

		update();
	}
}


/**
 * rename an object
 */
void GlSceneRenderer::RenameObject(const std::string& oldname, const std::string& newname)
{
	QMutexLocker _locker{&m_mutexObj};
	auto iter = m_objs.find(oldname);

	if(iter != m_objs.end())
	{
		// get node handle to rename key
		decltype(m_objs)::node_type node = m_objs.extract(iter);
		node.key() = newname;
		m_objs.insert(std::move(node));

		update();
	}
}


/**
 * add a polygon-based object
 */
GlSceneRenderer::t_objs::iterator
GlSceneRenderer::AddTriangleObject(
	const std::string& obj_name,
	const std::vector<t_vec3_gl>& triag_verts,
	const std::vector<t_vec3_gl>& triag_norms,
	const std::vector<t_vec3_gl>& triag_uvs,
	t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a)
{
	// colour
	auto col = tl2::create<t_vec_gl>({ r, g, b, a });

	GlSceneObj obj;
	create_bounding_objects<t_vec3_gl>(obj, triag_verts);

	QMutexLocker _locker{&m_mutexObj};

	create_triangle_object(this, obj,
		triag_verts, triag_verts, triag_norms,
		triag_uvs, col,
		false, m_attrVertex, m_attrVertexNorm,
		m_attrVertexCol, m_attrTexCoords);

	// object transformation matrix
	obj.m_mat = tl2::hom_translation<t_mat_gl, t_real_gl>(0., 0., 0.);

	return m_objs.emplace(std::make_pair(obj_name, std::move(obj))).first;
}


/**
 * centre camera around a given object
 */
void GlSceneRenderer::CentreCam(const std::string& objid)
{
	if(auto iter = m_objs.find(objid); iter!=m_objs.end())
	{
		GlSceneObj& obj = iter->second;

		// add object centre to transformation matrix
		const t_mat_gl& matObj = obj.m_mat;
		//matObj(0,3) += obj.m_boundingSpherePos[0];
		//matObj(1,3) += obj.m_boundingSpherePos[1];
		//matObj(2,3) += obj.m_boundingSpherePos[2];

		m_cam.Centre(matObj);
		UpdateCam();
	}
}


/**
 * create the geometry object of the selection plane
 */
void GlSceneRenderer::CreateSelectionPlane()
{
	t_vec3_gl norm = tl2::create<t_vec3_gl>({ 0, 0, 1 });
	t_real_gl len = 20.;
	auto solid = tl2::create_plane<t_mat_gl, t_vec3_gl>(norm, len, len);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec3_gl>(solid);
	auto col = tl2::create<t_vec_gl>({ 0.5, 0.5, 1., 0.1 });

	create_bounding_objects<t_vec3_gl>(m_selectionPlane, verts);

	create_triangle_object(this, m_selectionPlane,
		verts, verts, norms, uvs, col,
		false, m_attrVertex, m_attrVertexNorm,
		m_attrVertexCol, m_attrTexCoords);

	m_selectionPlane.m_visible = false;
	m_selectionPlane.m_cull = false;
	m_selectionPlane.m_lighting = false;

	m_selectionPlane.m_mat = tl2::hom_translation<t_mat_gl, t_real_gl>(0., 0., 0.);
}


/**
 * orient the selection plane and move it to the cursor position
 */
void GlSceneRenderer::CalcSelectionPlaneMatrix()
{
	// rotate plane object's [001] normal into this vector
	static const t_vec3_gl obj_norm = tl2::create<t_vec3_gl>({ 0, 0, 1 });
	t_mat_gl rot = tl2::rotation<t_mat_gl, t_vec3_gl>(obj_norm, m_selectionPlaneNorm);

	t_vec3_gl pos = m_selectionPlaneNorm * m_selectionPlaneDist;
	t_mat_gl trans = tl2::hom_translation<t_mat_gl>(pos[0], pos[1], pos[2]);

	m_selectionPlane.m_mat = trans * rot;
}


/**
 * set the normal vector of the selection plane
 */
void GlSceneRenderer::SetSelectionPlaneNorm(const t_vec3_gl& vec)
{
	const t_real_gl len = tl2::norm<t_vec3_gl>(vec);

	if(!tl2::equals_0<t_real_gl>(len, t_real_gl(g_eps)))
	{
		m_selectionPlaneNorm = vec;
		m_selectionPlaneNorm /= len;

		CalcSelectionPlaneMatrix();
		update();
	}
}


/**
 * set the distance of the selection plane
 */
void GlSceneRenderer::SetSelectionPlaneDist(t_real_gl d)
{
	m_selectionPlaneDist = d;

	CalcSelectionPlaneMatrix();
	update();
}


/**
 * set light position
 * light 0 is the principal light casting a shadow
 */
void GlSceneRenderer::SetLight(std::size_t idx, const t_vec3_gl& pos)
{
	if(m_lights.size() < idx+1)
		m_lights.resize(idx+1);

	m_lights[idx] = pos;
	m_lightsNeedUpdate = true;

	// target vector
	//t_vec3_gl target = tl2::create<t_vec3_gl>({ 0, 0, 0 });
	t_vec3_gl target = pos;
	target[2] = 0;

	// up vector
	t_vec3_gl up = tl2::create<t_vec3_gl>({ 0, 1, 0 });

	// light 0 is the principal light
	if(idx == 0)
		m_lightcam.SetLookAt(pos, target, up);
}


/**
 * light (and shadows) follow the cursor position
 */
void GlSceneRenderer::SetLightFollowsCursor(bool b)
{
	m_light_follows_cursor = b;
	update();
}


/**
 * enable the rendering of shadows
 */
void GlSceneRenderer::EnableShadowRendering(bool b)
{
	m_shadowRenderingEnabled = b;
	update();
}


/**
 * enable the stencik rendering passes
 */
void GlSceneRenderer::EnablePortalRendering(bool b)
{
	m_portalRenderingEnabled = b;
	update();
}


/**
 * update the light positions and the light camera for shadow rendering
 */
void GlSceneRenderer::UpdateLights()
{
	if(!m_initialised)
		return;

	auto *pGl = GetGlFunctions();
	if(!pGl)
		return;

	int num_lights = std::min(MAX_LIGHTS, static_cast<int>(m_lights.size()));
	t_real_gl pos[MAX_LIGHTS * 3];

	for(int i=0; i<num_lights; ++i)
	{
		pos[i*3 + 0] = m_lights[i][0];
		pos[i*3 + 1] = m_lights[i][1];
		pos[i*3 + 2] = m_lights[i][2];
	}

	// bind shaders
	BOOST_SCOPE_EXIT(m_shaders)
	{
		m_shaders->release();
	} BOOST_SCOPE_EXIT_END
	m_shaders->bind();
	LOGGLERR(pGl);

	m_shaders->setUniformValueArray(m_uniLightPos, pos, num_lights, 3);
	m_shaders->setUniformValue(m_uniNumActiveLights, num_lights);

	// update light perspective
	t_real ratio = 1;
	if(m_fboshadow)
	{
		ratio = t_real_gl(m_fboshadow->height()) /
			t_real_gl(m_fboshadow->width());
	}

	bool persp_proj = m_cam.GetPerspectiveProjection();
	m_lightcam.SetPerspectiveProjection(persp_proj);

	if(persp_proj)
	{
		// viewing angle has to be large enough so that the
		// shadow map covers the entire scene
		m_lightcam.SetFOV(tl2::pi<t_real_gl> * 0.75);
		m_lightcam.SetPerspectiveProjection(true);
		m_lightcam.SetAspectRatio(ratio);
		m_lightcam.UpdatePerspective();
	}

	m_lightcam.UpdatePerspective();

	// set matrices
	m_shaders->setUniformValue(
		m_uniMatrixLightProj, m_lightcam.GetPerspective());
	LOGGLERR(pGl);

	m_lightsNeedUpdate = false;
}


/**
 * get the position of the mouse cursor on the selection plane
 */
std::tuple<t_vec3_gl, int> GlSceneRenderer::GetSelectionPlaneCursor() const
{
	auto [org3, dir3] = m_cam.GetPickerRay(m_posMouse.x(), m_posMouse.y());

	auto[inters, inters_type, inters_lam] =
		tl2::intersect_line_plane<t_vec3_gl>(
			org3, dir3, m_selectionPlaneNorm, m_selectionPlaneDist);

	return std::make_tuple(inters, inters_type);
}


/**
 * calculate the object the mouse cursor is currently on
 */
void GlSceneRenderer::UpdatePicker()
{
	if(!m_initialised)
		return;

	// picker ray
	auto [org3, dir3] = m_cam.GetPickerRay(m_posMouse.x(), m_posMouse.y());


	// cursor coordinates (intersection with selection plane)
	if(auto [cursor_pos, inters_type] = GetSelectionPlaneCursor(); inters_type)
	{
		// save intersections with selection plane for drawing objects
		emit CursorCoordsChanged(cursor_pos[0], cursor_pos[1], cursor_pos[2]);

		if(m_light_follows_cursor)
			SetLight(0, tl2::create<t_vec3_gl>({ cursor_pos[0], cursor_pos[1], 10 }));
	}


	// intersection with geometry
	bool hasInters = false;
	m_curObj = "";
	t_vec_gl closest_inters = tl2::create<t_vec_gl>({ 0, 0, 0, 0 });

	QMutexLocker _locker{&m_mutexObj};

	for(const auto& [obj_name, obj] : m_objs)
	{
		if(obj.m_type != tl2::GlRenderObjType::TRIANGLES || !obj.m_visible)
			continue;

		const t_mat_gl& matObj = obj.m_mat;

		// scaling factor, TODO: maximum factor for non-uniform scaling
		auto scale = std::cbrt(std::abs(tl2::det(matObj)));

		// intersection with bounding sphere?
		auto boundingInters =
			tl2::intersect_line_sphere<t_vec3_gl, std::vector>(
				org3, dir3,
				matObj * obj.m_boundingSpherePos,
				scale * obj.m_boundingSphereRad);
		if(boundingInters.size() == 0)
			continue;

		// test actual polygons for intersection
		for(std::size_t startidx=0; startidx+2<obj.m_triangles.size(); startidx+=3)
		{
			std::vector<t_vec3_gl> poly{ {
				obj.m_triangles[startidx+0],
				obj.m_triangles[startidx+1],
				obj.m_triangles[startidx+2]
			} };

			std::vector<t_vec3_gl> polyuv{ {
				obj.m_uvs[startidx+0],
				obj.m_uvs[startidx+1],
				obj.m_uvs[startidx+2]
			} };

			auto [inters, does_intersect, inters_lam] =
				tl2::intersect_line_poly<t_vec3_gl, t_mat_gl>(
					org3, dir3, poly, matObj);

			if(!does_intersect)
				continue;

			t_vec_gl inters4 = tl2::create<t_vec_gl>({
				inters[0], inters[1], inters[2], 1});

			// intersection with other objects
			bool updateUV = false;

			if(!hasInters)
			{	// first intersection
				closest_inters = inters4;
				m_curObj = obj_name;
				hasInters = true;
				updateUV = true;
			}
			else
			{	// test if next intersection is closer...
				t_vec_gl oldPosTrafo = m_cam.GetTransformation() * closest_inters;
				t_vec_gl newPosTrafo = m_cam.GetTransformation() * inters4;

				if(tl2::norm(newPosTrafo) < tl2::norm(oldPosTrafo))
				{	// ...it is closer
					closest_inters = inters4;
					m_curObj = obj_name;

					updateUV = true;
				}
			}

			if(updateUV)
			{
				// TODO
			}
		}
	}

	m_pickerNeedsUpdate = false;
	t_vec3_gl closest_inters3 = tl2::create<t_vec3_gl>({
		closest_inters[0], closest_inters[1], closest_inters[2]});

	emit PickerIntersection(hasInters ? &closest_inters3 : nullptr, m_curObj);
}


/**
 * timer tick
 * this is currently only used for keyboard input
 * (new frames are rendered as needed, not on timer ticks)
 */
void GlSceneRenderer::tick(const std::chrono::milliseconds& ms)
{
	if(!m_initialised)
		return;

	bool needs_update = false;

	// if a key is pressed, move and update the camera
	if(m_arrowDown[0] || m_arrowDown[1] || m_arrowDown[2] || m_arrowDown[3]
		|| m_pageDown[0] || m_pageDown[1])
	{
		t_real_gl move_scale = t_real_gl(ms.count()) * g_move_scale;

		m_cam.Translate(
			move_scale * (t_real(m_arrowDown[0]) - t_real(m_arrowDown[1])),
			move_scale * (t_real(m_pageDown[0]) - t_real(m_pageDown[1])),
			move_scale * (t_real(m_arrowDown[2]) - t_real(m_arrowDown[3])));

		needs_update = true;
	}

	// zoom the view
	if(m_bracketDown[0] || m_bracketDown[1])
	{
		t_real zoom_dir = -1;
		if(m_bracketDown[1])
			zoom_dir = 1;

		t_real zoom_scale = t_real_gl(ms.count()) * g_zoom_scale;
		m_cam.Zoom(zoom_dir * zoom_scale);

		needs_update = true;
	}

	// update camera and frame
	if(needs_update)
		UpdateCam();

#ifdef USE_BULLET
	update();
#endif
}


/**
 * update the camera matrices and redraw the frame
 */
void GlSceneRenderer::UpdateCam(bool update_frame)
{
	if(m_cam.TransformationNeedsUpdate())
	{
		m_cam.UpdateTransformation();
		m_pickerNeedsUpdate = true;

		// emit changed camera position and rotation
		t_vec3_gl pos = m_cam.GetPosition();
		auto [phi, theta] = m_cam.GetRotation();

		emit CamPositionChanged(pos[0], pos[1], pos[2]);
		emit CamRotationChanged(phi, theta);
		emit CamZoomChanged(m_cam.GetZoom());
	}

	if(m_cam.PerspectiveNeedsUpdate())
	{
		m_cam.UpdatePerspective();
		m_perspectiveNeedsUpdate = true;
		m_pickerNeedsUpdate = true;
	}

	if(m_cam.ViewportNeedsUpdate())
	{
		m_cam.UpdateViewport();
		m_viewportNeedsUpdate = true;
	}

	// redraw frame
	if(update_frame)
		update();
}


/**
 * initialise renderer widget
 */
void GlSceneRenderer::initializeGL()
{
	m_initialised = false;

	// --------------------------------------------------------------------
	// shaders
	// --------------------------------------------------------------------
	std::string fragfile = g_res.FindFile("frag.shader");
	std::string vertexfile = g_res.FindFile("vertex.shader");

	auto [frag_ok, strFragShader] = tl2::load_file<std::string>(fragfile);
	auto [vertex_ok, strVertexShader] = tl2::load_file<std::string>(vertexfile);

	if(!frag_ok || !vertex_ok)
	{
		std::cerr << "Fragment or vertex shader could not be loaded." << std::endl;
		return;
	}
	// --------------------------------------------------------------------


	// set glsl version and constants
	const std::string strGlsl = tl2::var_to_str<t_real_gl>(_GLSL_MAJ_VER*100 + _GLSL_MIN_VER*10);
	std::string strPi = tl2::var_to_str<t_real_gl>(tl2::pi<t_real_gl>);
	algo::replace_all(strPi, std::string(","), std::string("."));	// ensure decimal point

	for(std::string* strSrc : { &strFragShader, &strVertexShader })
	{
		algo::replace_all(*strSrc, std::string("${GLSL_VERSION}"), strGlsl);
		algo::replace_all(*strSrc, std::string("${PI}"), strPi);
		algo::replace_all(*strSrc, std::string("${MAX_LIGHTS}"), tl2::var_to_str<unsigned int>(MAX_LIGHTS));
	}


	// get gl functions
	auto *pGl = tl2::get_gl_functions(this);
	if(!pGl) return;

	m_strGlVer = (char*)pGl->glGetString(GL_VERSION);
	m_strGlShaderVer = (char*)pGl->glGetString(GL_SHADING_LANGUAGE_VERSION);
	m_strGlVendor = (char*)pGl->glGetString(GL_VENDOR);
	m_strGlRenderer = (char*)pGl->glGetString(GL_RENDERER);
	LOGGLERR(pGl);


	static QMutex shadermutex;
	BOOST_SCOPE_EXIT(&shadermutex)
	{
		shadermutex.unlock();
	} BOOST_SCOPE_EXIT_END
	shadermutex.lock();

	// shader compiler/linker error handler
	auto shader_err = [this](const char* err) -> void
	{
		std::cerr << err << std::endl;

		std::string strLog = m_shaders->log().toStdString();
		if(strLog.size())
			std::cerr << "Shader log: " << strLog << std::endl;
	};

	// compile & link shaders
	m_shaders = std::make_shared<QOpenGLShaderProgram>(this);

	if(!m_shaders->addShaderFromSourceCode(QOpenGLShader::Fragment, strFragShader.c_str()))
	{
		shader_err("Cannot compile fragment shader.");
		return;
	}

	if(!m_shaders->addShaderFromSourceCode(QOpenGLShader::Vertex, strVertexShader.c_str()))
	{
		shader_err("Cannot compile vertex shader.");
		return;
	}

	if(!m_shaders->link())
	{
		shader_err("Cannot link shaders.");
		return;
	}


	// get attribute handles from shaders
	m_attrVertex = m_shaders->attributeLocation("vertex");
	m_attrVertexNorm = m_shaders->attributeLocation("normal");
	m_attrVertexCol = m_shaders->attributeLocation("vertex_col");
	m_attrTexCoords = m_shaders->attributeLocation("tex_coords");

	// get uniform handles from shaders
	m_uniMatrixCam = m_shaders->uniformLocation("trafos_cam");
	m_uniMatrixCamInv = m_shaders->uniformLocation("trafos_cam_inv");
	m_uniMatrixLight = m_shaders->uniformLocation("trafos_light");
	m_uniMatrixLightInv = m_shaders->uniformLocation("trafos_light_inv");
	m_uniMatrixProj = m_shaders->uniformLocation("trafos_proj");
	m_uniMatrixLightProj = m_shaders->uniformLocation("trafos_light_proj");
	m_uniMatrixObj = m_shaders->uniformLocation("trafos_obj");

	m_uniTextureActive = m_shaders->uniformLocation("texture_active");
	m_uniTexture = m_shaders->uniformLocation("texture_image");

	m_uniConstCol = m_shaders->uniformLocation("lights_const_col");
	m_uniLightPos = m_shaders->uniformLocation("lights_pos");
	m_uniNumActiveLights = m_shaders->uniformLocation("lights_numactive");
	m_uniLightingEnabled = m_shaders->uniformLocation("lights_enabled");

	m_uniShadowRenderingEnabled = m_shaders->uniformLocation("shadow_enabled");
	m_uniShadowRenderPass = m_shaders->uniformLocation("shadow_renderpass");
	m_uniShadowMap = m_shaders->uniformLocation("shadow_map");

	LOGGLERR(pGl);

	CreateSelectionPlane();
	SetLight(0, tl2::create<t_vec3_gl>({ 0, 0, 10 }));

	m_initialised = true;
	emit AfterGLInitialisation();
}


/**
 * renderer widget is being resized
 */
void GlSceneRenderer::resizeGL(int w, int h)
{
	m_cam.SetScreenDimensions(w, h);

	m_viewportNeedsUpdate = true;
	m_shadowFramebufferNeedsUpdate = true;
	m_lightsNeedUpdate = true;

	UpdateCam();
}


/**
 * get the gl functions corresponding to the selected version
 */
qgl_funcs* GlSceneRenderer::GetGlFunctions()
{
	if(!m_initialised)
		return nullptr;
	if(auto *pContext = ((QOpenGLWidget*)this)->context(); !pContext)
		return nullptr;
	return tl2::get_gl_functions(this);
}


/**
 * framebuffer for shadow rendering
 * @see (Sellers 2014) pp. 534-540
 */
void GlSceneRenderer::UpdateShadowFramebuffer()
{
	if(!m_initialised)
		return;

	auto *pGl = GetGlFunctions();
	if(!pGl)
		return;

	t_real scale = devicePixelRatioF();
	int w = m_cam.GetScreenDimensions()[0] * scale;
	int h = m_cam.GetScreenDimensions()[1] * scale;

	QOpenGLFramebufferObjectFormat fbformat;
	fbformat.setTextureTarget(GL_TEXTURE_2D);
	fbformat.setInternalTextureFormat(GL_RGBA32F);
	fbformat.setAttachment(QOpenGLFramebufferObject::Depth);
	m_fboshadow = std::make_shared<QOpenGLFramebufferObject>(
		w, h, fbformat);
	LOGGLERR(pGl);

	BOOST_SCOPE_EXIT(pGl, m_fboshadow)
	{
		pGl->glActiveTexture(GL_TEXTURE0);
		pGl->glBindTexture(GL_TEXTURE_2D, 0);
		if(m_fboshadow)
			m_fboshadow->release();
	} BOOST_SCOPE_EXIT_END

	if(m_fboshadow)
	{
		pGl->glActiveTexture(GL_TEXTURE0);

		m_fboshadow->bind();
		LOGGLERR(pGl);

		pGl->glBindTexture(GL_TEXTURE_2D, m_fboshadow->texture());
		LOGGLERR(pGl);

		// shadow texture parameters
		// see: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
		pGl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		pGl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		pGl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		pGl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}

	m_shadowFramebufferNeedsUpdate = false;
}


/**
 * find objects that are marked as portals
 */
void GlSceneRenderer::CreateActivePortals()
{
	m_active_portal = nullptr;
	m_active_portals.clear();

	for(const auto& [obj_name, obj] : m_objs)
	{
		if(bool obj_is_portal = (obj.m_portal_id >= 0); obj_is_portal)
		{
			// remember current portal for subsequent rendering passes
			ActivePortal active_portal{
				.id = obj.m_portal_id,
				.mat = obj.m_portal_mat };

			//std::cout << "portal " << active_portal.id << ":\n";
			//tl2::niceprint(std::cout, active_portal.mat);

			m_active_portals.emplace_back(std::move(active_portal));
		}
	}
}


/**
 * draw the scene
 */
void GlSceneRenderer::paintGL()
{
	if(!m_initialised || thread() != QThread::currentThread())
		return;

	QMutexLocker _locker{&m_mutexObj};

	if(auto *pContext = context(); !pContext) return;
	auto *pGl = tl2::get_gl_functions(this);

	// shadow framebuffer render pass
	if(m_shadowRenderingEnabled)
	{
		m_portalRenderPass = PortalRenderPass::IGNORE;
		m_shadowRenderPass = true;
		DoPaintGL(pGl);
		m_shadowRenderPass = false;
	}

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// gl main render pass
	{
		if(m_pickerNeedsUpdate)
			UpdatePicker();

		BOOST_SCOPE_EXIT(&painter)
		{
			painter.endNativePainting();
		} BOOST_SCOPE_EXIT_END
		painter.beginNativePainting();

		pGl->glClearColor(1., 1., 1., 1.);
		pGl->glClearStencil(0);

		if(m_portalRenderingEnabled)
		{
			CreateActivePortals();

			m_firstpass = true;
			for(const ActivePortal& active_portal : m_active_portals)
			{
				m_active_portal = &active_portal;

				// pass 0: draw portal masks into stencil buffer
				m_portalRenderPass = PortalRenderPass::CREATE_STENCIL;
				DoPaintGL(pGl);

				// pass 1: draw scene that is only visible through portals
				m_portalRenderPass = PortalRenderPass::RENDER_PORTALS;
				DoPaintGL(pGl);

				// pass 2: render only the z-values of the portals
				m_portalRenderPass = PortalRenderPass::CREATE_Z;
				DoPaintGL(pGl);

				m_firstpass = false;
			}

			// pass 3: draw the rest of the scene without portals
			m_portalRenderPass = PortalRenderPass::RENDER_NONPORTALS;
			m_active_portal = nullptr;
			DoPaintGL(pGl);
		}
		else
		{
			m_portalRenderPass = PortalRenderPass::IGNORE;
			m_firstpass = true;
			DoPaintGL(pGl);
		}
	}

	// qt painting pass
	{
		DoPaintQt(painter);
	}
}


/**
 * pure gl drawing
 */
void GlSceneRenderer::DoPaintGL(qgl_funcs *pGl)
{
	// remove shadow texture
	BOOST_SCOPE_EXIT(m_fboshadow, pGl)
	{
		pGl->glActiveTexture(GL_TEXTURE0);
		pGl->glBindTexture(GL_TEXTURE_2D, 0);
		if(m_fboshadow)
			m_fboshadow->release();
	} BOOST_SCOPE_EXIT_END

	bool portal_shadows =
		//m_portalRenderPass == PortalRenderPass::RENDER_PORTALS ||
		m_portalRenderPass == PortalRenderPass::RENDER_NONPORTALS ||
		m_portalRenderPass == PortalRenderPass::IGNORE;

	if(m_shadowRenderingEnabled && portal_shadows)
	{
		if(m_shadowRenderPass)
		{
			// render into the shadow framebuffer
			if(m_shadowFramebufferNeedsUpdate)
				UpdateShadowFramebuffer();

			if(m_fboshadow)
				m_fboshadow->bind();
		}
		else
		{
			// bind shadow texture
			if(m_fboshadow)
			{
				pGl->glActiveTexture(GL_TEXTURE0);
				pGl->glBindTexture(GL_TEXTURE_2D, m_fboshadow->texture());
				LOGGLERR(pGl);

				// see: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
				pGl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				pGl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
		}
	}


	// default options
	pGl->glCullFace(GL_BACK);
	pGl->glFrontFace(GL_CCW);
	pGl->glEnable(GL_CULL_FACE);

	pGl->glDisable(GL_BLEND);
	pGl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(m_shadowRenderPass)
		pGl->glDisable(GL_MULTISAMPLE);
	else
		pGl->glEnable(GL_MULTISAMPLE);
	pGl->glEnable(GL_LINE_SMOOTH);
	pGl->glEnable(GL_POLYGON_SMOOTH);
	pGl->glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	pGl->glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	// clear
	pGl->glDisable(GL_DEPTH_TEST);
	pGl->glDisable(GL_STENCIL_TEST);

	pGl->glDepthMask(GL_TRUE);
	pGl->glDepthFunc(GL_LESS);

	if(m_portalRenderPass == PortalRenderPass::CREATE_STENCIL)
	{
		// don't write colours when creating portal stencil maps
		pGl->glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		pGl->glStencilMask(~0);

		pGl->glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		pGl->glStencilOp(
			GL_KEEP,     // stencil test failed
			GL_KEEP,     // stencil test passed, depth test failed
			GL_REPLACE); // both tests passed
		pGl->glEnable(GL_STENCIL_TEST);
	}
	else if(m_portalRenderPass == PortalRenderPass::RENDER_PORTALS)
	{
		pGl->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// TODO: recover depth mask for multiple portals...
		// TODO: prevent (via z-buffer) the rendering of objects
		//       through a portal that are in front of the portal
		GLuint clear_bits = GL_DEPTH_BUFFER_BIT;
		if(m_firstpass)
			clear_bits |= GL_COLOR_BUFFER_BIT;
		pGl->glClear(clear_bits);

		pGl->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		pGl->glEnable(GL_STENCIL_TEST);
	}
	else if(m_portalRenderPass == PortalRenderPass::CREATE_Z)
	{
		// don't write colours when creating portal z maps
		pGl->glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		// prevent rendering scene in portals in front of portals
		// TODO: z-buffer for overlapping portals
		pGl->glDepthFunc(GL_ALWAYS);
	}
	else if(m_portalRenderPass == PortalRenderPass::RENDER_NONPORTALS)
	{
		pGl->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
	else if(m_portalRenderPass == PortalRenderPass::IGNORE)
	{
		pGl->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		pGl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	pGl->glEnable(GL_DEPTH_TEST);
	pGl->glStencilMask(0);

	if(m_viewportNeedsUpdate)
	{
		const auto& dims = m_cam.GetScreenDimensions();
		auto [z_near, z_far] = m_cam.GetDepthRange();

		pGl->glViewport(0, 0, dims[0], dims[1]);
		pGl->glDepthRange(z_near, z_far);
		LOGGLERR(pGl);

		m_viewportNeedsUpdate = false;
	}

	if(m_lightsNeedUpdate)
		UpdateLights();

	// bind shaders
	BOOST_SCOPE_EXIT(m_shaders)
	{
		m_shaders->release();
	} BOOST_SCOPE_EXIT_END
	m_shaders->bind();
	LOGGLERR(pGl);

	// lighting not needed for creation of shadow map
	if(m_shadowRenderPass)
		m_shaders->setUniformValue(m_uniLightingEnabled, false);

	m_shaders->setUniformValue(m_uniShadowRenderingEnabled, m_shadowRenderingEnabled);
	m_shaders->setUniformValue(m_uniShadowRenderPass, m_shadowRenderPass);

	// set camera transformation matrix
	m_shaders->setUniformValue(
		m_uniMatrixCam, m_cam.GetTransformation());
	m_shaders->setUniformValue(
		m_uniMatrixCamInv, m_cam.GetInverseTransformation());

	// set perspective matrix
	if(m_perspectiveNeedsUpdate)
	{
		m_shaders->setUniformValue(m_uniMatrixProj, m_cam.GetPerspective());
		m_perspectiveNeedsUpdate = false;
	}

	// set light matrix
	m_shaders->setUniformValue(
		m_uniMatrixLight, m_lightcam.GetTransformation());
	m_shaders->setUniformValue(
		m_uniMatrixLightInv, m_lightcam.GetInverseTransformation());

	m_shaders->setUniformValue(m_uniShadowMap, 0);

	//m_shaders->setUniformValue(m_uniTextureActive, m_textures_active);
	m_shaders->setUniformValue(m_uniTexture, 1);

	auto colOverride = tl2::create<t_vec_gl>({ 1, 1, 1, 1 });

	// render object
	auto render_triangle_geometry = 
		[this, pGl, &colOverride, &boost_scope_exit_aux_args](
			const GlSceneObj& obj)
	{
		if(!obj.m_visible)
			return;

		const bool obj_is_portal = (obj.m_portal_id >= 0 &&
			m_portalRenderPass != PortalRenderPass::IGNORE);
		t_mat_gl matObj = obj.m_mat;

		// draw portal surfaces into stencil buffer
		if(m_portalRenderPass == PortalRenderPass::CREATE_STENCIL)
		{
			if(m_active_portal && obj_is_portal && obj.m_portal_id == m_active_portal->id)
			{
				pGl->glStencilMask(~0);
				pGl->glStencilFunc(GL_ALWAYS, 1, ~0);
			}
			else
			{
				// don't write non-portals to stencil buffer
				pGl->glStencilMask(0);
			}
		}
		else if(m_portalRenderPass == PortalRenderPass::CREATE_Z)
		{
			// ignore non-portals
			if(!obj_is_portal)
				return;
		}
		else if(m_portalRenderPass == PortalRenderPass::RENDER_NONPORTALS)
		{
			// ignore portals
			if(obj_is_portal)
				return;
		}
		else if(m_portalRenderPass == PortalRenderPass::RENDER_PORTALS)
		{
			// ignore portals themselves (only render view through portals)
			if(obj_is_portal)
				return;

			if(m_active_portal)
				matObj = m_active_portal->mat * matObj;
		}

		// check if object is in camera frustum
		if(m_shadowRenderPass)
		{
			if(obj_is_portal)
				return;

			if(m_lightcam.IsBoundingBoxOutsideFrustum(
				matObj, obj.m_boundingBox))
				return;
		}
		else
		{
			m_shaders->setUniformValue(m_uniLightingEnabled, obj.m_lighting);

			if(m_cam.IsBoundingBoxOutsideFrustum(
				matObj, obj.m_boundingBox))
				return;
		}

		// textures
		std::shared_ptr<QOpenGLTexture> texture;
		if(m_textures_active && !m_shadowRenderPass)
		{
			if(auto iter = m_textures.find(obj.m_texture);
				iter!=m_textures.end())
			{
				texture = iter->second.texture;
				/*if(texture)
				{
					texture->setMinMagFilters(
						QOpenGLTexture::Linear, QOpenGLTexture::Linear);
				}*/
			}
		}

		BOOST_SCOPE_EXIT(texture, pGl)
		{
			if(texture)
			{
				pGl->glActiveTexture(GL_TEXTURE1);
				pGl->glBindTexture(GL_TEXTURE_2D, 0);
				texture->release();
			}
		} BOOST_SCOPE_EXIT_END

		m_shaders->setUniformValue(m_uniTextureActive, !!texture);

		if(texture)
		{
			pGl->glActiveTexture(GL_TEXTURE1);
			texture->bind();
			LOGGLERR(pGl);

			// see: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
			pGl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			pGl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}


		// set override color to white
		m_shaders->setUniformValue(m_uniConstCol, colOverride);

		if(obj.m_cull)
			pGl->glEnable(GL_CULL_FACE);
		else
			pGl->glDisable(GL_CULL_FACE);

		// draw scene that is only visible through portals
		if(m_portalRenderPass == PortalRenderPass::RENDER_PORTALS && m_active_portal)
		{
			pGl->glStencilFunc(GL_EQUAL, 1, ~0);
		}

		m_shaders->setUniformValue(m_uniMatrixObj, matObj);

		// main vertex array object
		obj.m_vertex_array->bind();

		// bind vertex attribute arrays
		BOOST_SCOPE_EXIT(pGl, &obj, &m_attrVertex, &m_attrVertexNorm, &m_attrVertexCol, &m_attrTexCoords)
		{
			pGl->glDisableVertexAttribArray(m_attrVertexCol);
			if(obj.m_type == tl2::GlRenderObjType::TRIANGLES)
			{
				pGl->glDisableVertexAttribArray(m_attrTexCoords);
				pGl->glDisableVertexAttribArray(m_attrVertexNorm);
			}
			pGl->glDisableVertexAttribArray(m_attrVertex);
		}
		BOOST_SCOPE_EXIT_END

		pGl->glEnableVertexAttribArray(m_attrVertex);
		if(obj.m_type == tl2::GlRenderObjType::TRIANGLES)
		{
			pGl->glEnableVertexAttribArray(m_attrVertexNorm);
			pGl->glEnableVertexAttribArray(m_attrTexCoords);
		}
		pGl->glEnableVertexAttribArray(m_attrVertexCol);
		LOGGLERR(pGl);


		// render the object
		if(obj.m_type == tl2::GlRenderObjType::TRIANGLES)
			pGl->glDrawArrays(GL_TRIANGLES, 0, obj.m_triangles.size());
		else if(obj.m_type == tl2::GlRenderObjType::LINES)
			pGl->glDrawArrays(GL_LINES, 0, obj.m_vertices.size());
		else
			std::cerr << "Unknown plot object type." << std::endl;
		LOGGLERR(pGl);
	};

	for(const auto& [obj_name, obj] : m_objs)
		render_triangle_geometry(obj);

	// render the selection plane
	if(!m_shadowRenderPass)
	{
		m_shaders->setUniformValue(m_uniShadowRenderingEnabled, false);
		pGl->glEnable(GL_BLEND);
		render_triangle_geometry(m_selectionPlane);
	}

	pGl->glDisable(GL_BLEND);
	pGl->glDisable(GL_CULL_FACE);
	pGl->glDisable(GL_DEPTH_TEST);
	pGl->glDisable(GL_STENCIL_TEST);
}


/**
 * directly draw on a qpainter
 */
void GlSceneRenderer::DoPaintQt(QPainter &painter)
{
	QFont fontOrig = painter.font();
	QPen penOrig = painter.pen();
	QBrush brushOrig = painter.brush();

	// draw tooltips and bounding rectangles
	if(auto curObj = m_objs.find(m_curObj); curObj != m_objs.end())
	{
		const auto& obj = curObj->second;

		if(obj.m_visible && obj.m_type == tl2::GlRenderObjType::TRIANGLES)
		{
			// draw a bounding rectangle over the currently selected item
			if(g_draw_bounding_rectangles)
			{
				auto boundingRect = m_cam.GetBoundingRect(
					obj.m_mat, obj.m_boundingBox /*obj.m_vertices*/);

				QPolygonF polyBounds;
				for(const t_vec_gl& _vertex : boundingRect)
				{
					const t_mat_gl& matViewport = m_cam.GetViewport();
					t_vec_gl vertex = matViewport * _vertex;
					vertex[1] = matViewport(1,1)*2. - vertex[1];
					polyBounds << QPointF(vertex[0], vertex[1]);
				}

				QPen penBounds = penOrig;
				penBounds.setColor(QColor(0xff, 0xff, 0xff, 0xe0));
				penBounds.setWidth(4.);
				painter.setPen(penBounds);
				painter.drawPolygon(polyBounds);
				penBounds.setColor(QColor(0x00, 0x00, 0x00, 0xe0));
				penBounds.setWidth(2.);
				painter.setPen(penBounds);
				painter.drawPolygon(polyBounds);
			}

			// draw tooltip
			QString label = curObj->first.c_str();

			QFont fontLabel = fontOrig;
			QPen penLabel = penOrig;
			QBrush brushLabel = brushOrig;

			fontLabel.setStyleStrategy(
				QFont::StyleStrategy(
					QFont::PreferAntialias |
					QFont::PreferQuality));
			fontLabel.setWeight(QFont::Normal);
			penLabel.setColor(QColor(0, 0, 0, 0xff));
			brushLabel.setColor(QColor(0xff, 0xff, 0xff, 0x7f));
			brushLabel.setStyle(Qt::SolidPattern);
			painter.setFont(fontLabel);
			painter.setPen(penLabel);
			painter.setBrush(brushLabel);

			QRect textBoundingRect = painter.fontMetrics().boundingRect(label);
			textBoundingRect.setWidth(textBoundingRect.width() * 1.5);
			textBoundingRect.setHeight(textBoundingRect.height() * 2);
			textBoundingRect.translate(m_posMouse.x()+16, m_posMouse.y()+24);

			painter.drawRoundedRect(textBoundingRect, 8., 8.);
			painter.drawText(textBoundingRect,
				Qt::AlignCenter | Qt::AlignVCenter,
				label);
		}
	}

	// restore original styles
	painter.setFont(fontOrig);
	painter.setPen(penOrig);
	painter.setBrush(brushOrig);
}


/**
 * save the shadow frame buffer to a file
 */
void GlSceneRenderer::SaveShadowFramebuffer(const std::string& filename) const
{
	auto img = m_fboshadow->toImage(true, 0);
	img.save(filename.c_str());
}


/**
 * paint event
 */
void GlSceneRenderer::paintEvent(QPaintEvent* pEvt)
{
	QOpenGLWidget::paintEvent(pEvt);
}
