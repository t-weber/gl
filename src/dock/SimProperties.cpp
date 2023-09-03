/**
 * simulation properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date 2-september-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "SimProperties.h"
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
SimPropertiesWidget::SimPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	m_spinTimeScale = new QDoubleSpinBox(this);
	m_spinTimeScale->setMinimum(-999.);
	m_spinTimeScale->setMaximum(999.);
	m_spinTimeScale->setDecimals(g_prec_gui);
	m_spinTimeScale->setSingleStep(0.1);
	m_spinTimeScale->setValue(1.);
	m_spinTimeScale->setToolTip("Simulation time scale.");

	m_spinMaxTimeStep = new QSpinBox(this);
	m_spinMaxTimeStep->setMinimum(1);
	m_spinMaxTimeStep->setMaximum(9999);
	m_spinMaxTimeStep->setSingleStep(100);
	m_spinMaxTimeStep->setValue(100);
	m_spinMaxTimeStep->setSuffix(" ms");
	m_spinMaxTimeStep->setToolTip("Maximum simulation time per step.");

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(2);
	grid->setVerticalSpacing(2);
	grid->setContentsMargins(4,4,4,4);

	int y = 0;
	grid->addWidget(new QLabel("Time Scale:", this), y, 0, 1, 1);
	grid->addWidget(m_spinTimeScale, y++, 1, 1, 1);
	grid->addWidget(new QLabel("Max. Time Step:", this), y, 0, 1, 1);
	grid->addWidget(m_spinMaxTimeStep, y++, 1, 1, 1);
	grid->addItem(new QSpacerItem(1, 1,
		QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 2);

	// signals
	connect(m_spinTimeScale,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &SimPropertiesWidget::TimeScaleChanged);
	connect(m_spinMaxTimeStep,
		static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		this, &SimPropertiesWidget::MaxTimeStepChanged);
}


SimPropertiesWidget::~SimPropertiesWidget()
{
}


void SimPropertiesWidget::SetTimeScale(t_real t)
{
	this->blockSignals(true);
	m_spinTimeScale->setValue(t);
	this->blockSignals(false);
}


void SimPropertiesWidget::SetMaxTimeStep(int dt)
{
	this->blockSignals(true);
	m_spinMaxTimeStep->setValue(dt);
	this->blockSignals(false);
}


/**
 * save the dock widget's settings
 */
boost::property_tree::ptree SimPropertiesWidget::Save() const
{
	boost::property_tree::ptree prop;

	prop.put<t_real>("time_scale", m_spinTimeScale->value());
	prop.put<t_int>("time_step", m_spinMaxTimeStep->value());

	return prop;
}


/**
 * load the dock widget's settings
 */
bool SimPropertiesWidget::Load(const boost::property_tree::ptree& prop)
{
	// old values
	t_real t = m_spinTimeScale->value();
	t_int dt = m_spinMaxTimeStep->value();

	// new values
	if(auto opt = prop.get_optional<t_real>("time_scale"); opt)
		t = *opt;
	if(auto opt = prop.get_optional<t_int>("time_step"); opt)
		dt = *opt;

	// set new values
	SetTimeScale(t);
	SetMaxTimeStep(dt);

	// emit changes
	emit TimeScaleChanged(t);
	emit MaxTimeStepChanged(t);

	return true;
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
SimPropertiesDockWidget::SimPropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent},
		m_widget{std::make_shared<SimPropertiesWidget>(this)}
{
	setObjectName("SimPropertiesDockWidget");
	setWindowTitle("Simulation Properties");

	setWidget(m_widget.get());
}


SimPropertiesDockWidget::~SimPropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------
