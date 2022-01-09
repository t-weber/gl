/**
 * camera properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "CamProperties.h"
#include "src/settings_variables.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFrame>


// --------------------------------------------------------------------------------
// properties widget
// --------------------------------------------------------------------------------
CamPropertiesWidget::CamPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	m_spinViewingAngle = new QDoubleSpinBox(this);
	m_spinViewingAngle->setMinimum(1);
	m_spinViewingAngle->setMaximum(179);
	m_spinViewingAngle->setDecimals(g_prec_gui);
	m_spinViewingAngle->setSingleStep(1);
	m_spinViewingAngle->setSuffix("°");
	m_spinViewingAngle->setToolTip("Camera field of view [deg].");

	m_spinZoom = new QDoubleSpinBox(this);
	m_spinZoom->setMinimum(0.001);
	m_spinZoom->setMaximum(999);
	m_spinZoom->setDecimals(g_prec_gui);
	m_spinZoom->setSingleStep(0.1);
	m_spinZoom->setToolTip("Camera zoom.");

	m_checkPerspectiveProj = new QCheckBox(this);
	m_checkPerspectiveProj->setText("Perspective Projection");
	m_checkPerspectiveProj->setToolTip("Choose perspective or parallel projection.");
	m_checkPerspectiveProj->setChecked(true);

	const char* pos_comp[] = {"x", "y", "z"};
	for(int pos=0; pos<3; ++pos)
	{
		m_spinPos[pos] = new QDoubleSpinBox(this);
		m_spinPos[pos]->setMinimum(-100);
		m_spinPos[pos]->setMaximum(+100);
		m_spinPos[pos]->setDecimals(g_prec_gui);
		m_spinPos[pos]->setSingleStep(1);
		m_spinPos[pos]->setToolTip(QString("Camera %1 position.").arg(pos_comp[pos]));
	}

	m_spinRot[0] = new QDoubleSpinBox(this);
	m_spinRot[0]->setMinimum(0);
	m_spinRot[0]->setMaximum(360);
	m_spinRot[0]->setDecimals(g_prec_gui);
	m_spinRot[0]->setSingleStep(1);
	m_spinRot[0]->setSuffix("°");
	m_spinRot[0]->setToolTip("Camera φ rotation [deg].");

	m_spinRot[1] = new QDoubleSpinBox(this);
	m_spinRot[1]->setMinimum(-90);
	m_spinRot[1]->setMaximum(0);
	m_spinRot[1]->setDecimals(g_prec_gui);
	m_spinRot[1]->setSingleStep(1);
	m_spinRot[1]->setSuffix("°");
	m_spinRot[1]->setToolTip("Camera θ rotation [deg].");

	auto *groupProj = new QGroupBox("Projection", this);
	{
		auto *layoutProj = new QGridLayout(groupProj);
		layoutProj->setHorizontalSpacing(2);
		layoutProj->setVerticalSpacing(2);
		layoutProj->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutProj->addWidget(new QLabel("Field of View and Zoom:", this),
			y++, 0, 1, 2);
		layoutProj->addWidget(m_spinViewingAngle, y, 0, 1, 1);
		layoutProj->addWidget(m_spinZoom, y++, 1, 1, 1);
		layoutProj->addWidget(m_checkPerspectiveProj, y++, 0, 1, 2);
	}

	auto *groupVecs = new QGroupBox("Vectors", this);
	{
		auto *layoutVecs = new QGridLayout(groupVecs);
		layoutVecs->setHorizontalSpacing(2);
		layoutVecs->setVerticalSpacing(2);
		layoutVecs->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutVecs->addWidget(new QLabel("Position (x, y, z):", this),
			y++, 0, 1, 6);
		layoutVecs->addWidget(m_spinPos[0], y, 0, 1, 2);
		layoutVecs->addWidget(m_spinPos[1], y, 2, 1, 2);
		layoutVecs->addWidget(m_spinPos[2], y++, 4, 1, 2);

		//QFrame *separator = new QFrame(this);
		//separator->setFrameStyle(QFrame::HLine);
		//layoutVecs->addWidget(separator, y++, 0, 1, 6);

		layoutVecs->addWidget(new QLabel("Rotation (φ, θ):", this),
			y++, 0, 1, 6);
		layoutVecs->addWidget(m_spinRot[0], y, 0, 1, 3);
		layoutVecs->addWidget(m_spinRot[1], y++, 3, 1, 3);
	}

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(2);
	grid->setVerticalSpacing(2);
	grid->setContentsMargins(4,4,4,4);

	int y = 0;
	grid->addWidget(groupProj, y++, 0, 1, 1);
	grid->addWidget(groupVecs, y++, 0, 1, 1);
	grid->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 1);

	// viewing angle
	connect(m_spinViewingAngle,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &CamPropertiesWidget::ViewingAngleChanged);

	// zoom
	connect(m_spinZoom,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &CamPropertiesWidget::ZoomChanged);

	// perspective projection flag
	connect(m_checkPerspectiveProj, &QCheckBox::stateChanged,
		[this](int state) -> void
		{
			emit PerspectiveProjChanged(state == Qt::Checked);
		});

	// position
	for(int i=0; i<3; ++i)
	{
		connect(m_spinPos[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real val) -> void
			{
				t_real pos[3];
				for(int j=0; j<3; ++j)
				{
					if(j == i)
						pos[j] = val;
					else
						pos[j] = m_spinPos[j]->value();
				}

				emit PositionChanged(pos[0], pos[1], pos[2]);
			});
	}

	// rotation
	for(int i=0; i<2; ++i)
	{
		connect(m_spinRot[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real angle) -> void
			{
				t_real angles[2];
				for(int j=0; j<2; ++j)
				{
					if(j == i)
						angles[j] = angle;
					else
						angles[j] = m_spinRot[j]->value();
				}

				emit RotationChanged(angles[0], angles[1]);
			});
	}
}


CamPropertiesWidget::~CamPropertiesWidget()
{
}


void CamPropertiesWidget::SetViewingAngle(t_real angle)
{
	this->blockSignals(true);
	m_spinViewingAngle->setValue(angle);
	this->blockSignals(false);
}


void CamPropertiesWidget::SetZoom(t_real zoom)
{
	this->blockSignals(true);
	m_spinZoom->setValue(zoom);
	this->blockSignals(false);
}


void CamPropertiesWidget::SetPerspectiveProj(bool proj)
{
	m_checkPerspectiveProj->setChecked(proj);
}


void CamPropertiesWidget::SetPosition(t_real x, t_real y, t_real z)
{
	this->blockSignals(true);
	if(m_spinPos[0]) m_spinPos[0]->setValue(x);
	if(m_spinPos[1]) m_spinPos[1]->setValue(y);
	if(m_spinPos[2]) m_spinPos[2]->setValue(z);
	this->blockSignals(false);
}


void CamPropertiesWidget::SetRotation(t_real phi, t_real theta)
{
	this->blockSignals(true);
	if(m_spinRot[0]) m_spinRot[0]->setValue(phi);
	if(m_spinRot[1]) m_spinRot[1]->setValue(theta);
	this->blockSignals(false);
}


/**
 * save the dock widget's settings
 */
boost::property_tree::ptree CamPropertiesWidget::Save() const
{
	boost::property_tree::ptree prop;

	// camera position
	prop.put<t_real>("x", m_spinPos[0]->value());
	prop.put<t_real>("y", m_spinPos[1]->value());
	prop.put<t_real>("z", m_spinPos[2]->value());

	// camera rotation
	prop.put<t_real>("phi", m_spinRot[0]->value());
	prop.put<t_real>("theta", m_spinRot[1]->value());

	// viewing angle and projection
	prop.put<t_real>("viewing_angle", m_spinViewingAngle->value());
	prop.put<t_real>("zoom", m_spinZoom->value());
	prop.put<int>("perspective_proj", m_checkPerspectiveProj->isChecked());

	return prop;
}


/**
 * load the dock widget's settings
 */
bool CamPropertiesWidget::Load(const boost::property_tree::ptree& prop)
{
	// old camera position
	t_real pos0 = m_spinPos[0]->value();
	t_real pos1 = m_spinPos[1]->value();
	t_real pos2 = m_spinPos[2]->value();

	// old camera rotation
	t_real rot0 = m_spinRot[0]->value();
	t_real rot1 = m_spinRot[1]->value();

	// camera position
	if(auto opt = prop.get_optional<t_real>("x"); opt)
		pos0 = *opt;
	if(auto opt = prop.get_optional<t_real>("y"); opt)
		pos1 = *opt;
	if(auto opt = prop.get_optional<t_real>("z"); opt)
		pos2 = *opt;

	// camera rotation
	if(auto opt = prop.get_optional<t_real>("phi"); opt)
		rot0 = *opt;
	if(auto opt = prop.get_optional<t_real>("theta"); opt)
		rot1 = *opt;

	// viewing angle and projection
	if(auto opt = prop.get_optional<t_real>("viewing_angle"); opt)
		m_spinViewingAngle->setValue(*opt);
	if(auto opt = prop.get_optional<t_real>("zoom"); opt)
		m_spinZoom->setValue(*opt);
	if(auto opt = prop.get_optional<int>("perspective_proj"); opt)
		m_checkPerspectiveProj->setChecked(*opt != 0);

	// set new values
	SetPosition(pos0, pos1, pos2);
	SetRotation(rot0, rot1);

	// emit changes
	emit PositionChanged(pos0, pos1, pos2);
	emit RotationChanged(rot0, rot1);

	return true;
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
CamPropertiesDockWidget::CamPropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent},
		m_widget{std::make_shared<CamPropertiesWidget>(this)}
{
	setObjectName("CamPropertiesDockWidget");
	setWindowTitle("Camera Properties");

	setWidget(m_widget.get());
}


CamPropertiesDockWidget::~CamPropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------
