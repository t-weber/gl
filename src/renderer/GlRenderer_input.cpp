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


/**
 * a key has been pressed
 */
void GlSceneRenderer::keyPressEvent(QKeyEvent *pEvt)
{
	switch(pEvt->key())
	{
		case Qt::Key_Left:
			m_arrowDown[0] = 1;
			pEvt->accept();
			break;
		case Qt::Key_Right:
			m_arrowDown[1] = 1;
			pEvt->accept();
			break;
		case Qt::Key_Up:
			m_arrowDown[2] = 1;
			pEvt->accept();
			break;
		case Qt::Key_Down:
			m_arrowDown[3] = 1;
			pEvt->accept();
			break;
		case Qt::Key_PageUp:
		case Qt::Key_Comma:
			m_pageDown[0] = 1;
			pEvt->accept();
			break;
		case Qt::Key_PageDown:
		case Qt::Key_Period:
			m_pageDown[1] = 1;
			pEvt->accept();
			break;
		case Qt::Key_BracketLeft:
			m_bracketDown[0] = 1;
			pEvt->accept();
			break;
		case Qt::Key_BracketRight:
			m_bracketDown[1] = 1;
			pEvt->accept();
			break;
		/*case Qt::Key_S:
			SaveShadowFramebuffer("shadow.png");
			break;*/
		default:
			QOpenGLWidget::keyPressEvent(pEvt);
			break;
	}
}


/**
 * a key has been released
 */
void GlSceneRenderer::keyReleaseEvent(QKeyEvent *pEvt)
{
	switch(pEvt->key())
	{
		case Qt::Key_Left:
			m_arrowDown[0] = 0;
			pEvt->accept();
			break;
		case Qt::Key_Right:
			m_arrowDown[1] = 0;
			pEvt->accept();
			break;
		case Qt::Key_Up:
			m_arrowDown[2] = 0;
			pEvt->accept();
			break;
		case Qt::Key_Down:
			m_arrowDown[3] = 0;
			pEvt->accept();
			break;
		case Qt::Key_PageUp:
		case Qt::Key_Comma:
			m_pageDown[0] = 0;
			pEvt->accept();
			break;
		case Qt::Key_PageDown:
		case Qt::Key_Period:
			m_pageDown[1] = 0;
			pEvt->accept();
			break;
		case Qt::Key_BracketLeft:
			m_bracketDown[0] = 0;
			pEvt->accept();
			break;
		case Qt::Key_BracketRight:
			m_bracketDown[1] = 0;
			pEvt->accept();
			break;
		default:
			QOpenGLWidget::keyReleaseEvent(pEvt);
			break;
	}
}


/**
 * the mouse cursor is being moved
 */
void GlSceneRenderer::mouseMoveEvent(QMouseEvent *pEvt)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	m_posMouse = pEvt->position();
#else
	m_posMouse = pEvt->localPos();
#endif

	if(m_inRotation)
	{
		auto diff = (m_posMouse - m_posMouseRotationStart)
			* g_rotation_scale;

		m_cam.Rotate(diff.x(), diff.y());
		UpdateCam(false);
	}

	UpdatePicker();

	// an object is being dragged
	if(m_draggedObj != "")
	{
		emit ObjectDragged(false, m_draggedObj);
	}

	m_mouseMovedBetweenDownAndUp = true;

	update();
	pEvt->accept();
}


/**
 * get the mouse position on the screen (not in the scene)
 */
QPoint GlSceneRenderer::GetMousePosition(bool global_pos) const
{
	QPoint pos = m_posMouse.toPoint();
	if(global_pos)
		pos = mapToGlobal(pos);
	return pos;
}


/**
 * mouse button is being pressed
 */
void GlSceneRenderer::mousePressEvent(QMouseEvent *pEvt)
{
	m_mouseMovedBetweenDownAndUp = false;

	if(pEvt->buttons() & Qt::LeftButton) m_mouseDown[0] = 1;
	if(pEvt->buttons() & Qt::MiddleButton) m_mouseDown[1] = 1;
	if(pEvt->buttons() & Qt::RightButton) m_mouseDown[2] = 1;

	// left mouse button pressed
	if(m_mouseDown[0] && m_draggedObj == "")
	{
		m_draggedObj = m_curObj;
		emit ObjectDragged(true, m_draggedObj);
	}

	// middle mouse button pressed
	if(m_mouseDown[1])
	{
		// reset zoom
		m_cam.SetZoom(1);
		UpdateCam();
	}

	// right mouse button pressed
	if(m_mouseDown[2])
	{
		// begin rotation
		if(!m_inRotation)
		{
			m_posMouseRotationStart = m_posMouse;
			m_inRotation = true;
		}
	}

	pEvt->accept();
}


/**
 * mouse button is released
 */
void GlSceneRenderer::mouseReleaseEvent(QMouseEvent *pEvt)
{
	bool mouseDownOld[] = { m_mouseDown[0], m_mouseDown[1], m_mouseDown[2] };

	if((pEvt->buttons() & Qt::LeftButton) == 0) m_mouseDown[0] = 0;
	if((pEvt->buttons() & Qt::MiddleButton) == 0) m_mouseDown[1] = 0;
	if((pEvt->buttons() & Qt::RightButton) == 0) m_mouseDown[2] = 0;

	// left mouse button released
	if(!m_mouseDown[0])
	{
		m_draggedObj = "";
	}

	// right mouse button released
	if(!m_mouseDown[2])
	{
		// end rotation
		if(m_inRotation)
		{
			m_cam.SaveRotation();
			m_inRotation = false;
		}
	}

	pEvt->accept();

	// only emit click if moving the mouse (i.e. rotationg the scene) was not the primary intent
	if(!m_mouseMovedBetweenDownAndUp)
	{
		bool mouseClicked[] = {
			!m_mouseDown[0] && mouseDownOld[0],
			!m_mouseDown[1] && mouseDownOld[1],
			!m_mouseDown[2] && mouseDownOld[2] };
		if(mouseClicked[0] || mouseClicked[1] || mouseClicked[2])
			emit ObjectClicked(m_curObj, mouseClicked[0], mouseClicked[1], mouseClicked[2]);
	}
}


/**
 * mouse wheel movement
 */
void GlSceneRenderer::wheelEvent(QWheelEvent *pEvt)
{
	const t_real_gl degrees = pEvt->angleDelta().y() / 8.;
	if(m::equals_0<t_real_gl>(degrees, t_real_gl(g_eps)))
	{
		pEvt->ignore();
		return;
	}

	m_cam.Zoom(degrees * g_wheel_zoom_scale);
	UpdateCam();

	pEvt->accept();
}
