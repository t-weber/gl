/**
 * selection plane properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2022
 * @license GPLv3, see 'LICENSE' file
s */

#include "SelectionProperties.h"
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
SelectionPropertiesWidget::SelectionPropertiesWidget(QWidget *parent)
	: QWidget{parent}
{
	const char* pos_comp[] = {"x", "y", "z"};
	for(int pos=0; pos<3; ++pos)
	{
		m_spinPlaneNorm[pos] = new QDoubleSpinBox(this);
		m_spinPlaneNorm[pos]->setMinimum(-999);
		m_spinPlaneNorm[pos]->setMaximum(+999);
		m_spinPlaneNorm[pos]->setDecimals(g_prec_gui);
		m_spinPlaneNorm[pos]->setSingleStep(1);
		m_spinPlaneNorm[pos]->setValue(pos == 2 ? 1. : 0.);
		m_spinPlaneNorm[pos]->setToolTip(
			QString("Selection plane normal %1 component.").arg(pos_comp[pos]));
	}

	m_spinPlaneDist = new QDoubleSpinBox(this);
	m_spinPlaneDist->setMinimum(-999);
	m_spinPlaneDist->setMaximum(999);
	m_spinPlaneDist->setDecimals(g_prec_gui);
	m_spinPlaneDist->setSingleStep(1);
	m_spinPlaneDist->setValue(0.);
	m_spinPlaneDist->setToolTip("Selection plane distance.");

	QPushButton* btnPlaneNorm[3];
	const char* pos_btn[] = {"[100]", "[010]", "[001]"};
	for(int pos=0; pos<3; ++pos)
	{
		btnPlaneNorm[pos] = new QPushButton(this);
		btnPlaneNorm[pos]->setText(pos_btn[pos]);
		btnPlaneNorm[pos]->setToolTip(
			QString("Set selection plane normal to %1.").arg(pos_btn[pos]));
	}

	auto *groupVecs = new QGroupBox("Selection Plane", this);
	{
		auto *layoutVecs = new QGridLayout(groupVecs);
		layoutVecs->setHorizontalSpacing(2);
		layoutVecs->setVerticalSpacing(2);
		layoutVecs->setContentsMargins(4,4,4,4);

		int y = 0;
		layoutVecs->addWidget(new QLabel("Normal (x, y, z):", this),
			y++, 0, 1, 6);
		layoutVecs->addWidget(m_spinPlaneNorm[0], y, 0, 1, 2);
		layoutVecs->addWidget(m_spinPlaneNorm[1], y, 2, 1, 2);
		layoutVecs->addWidget(m_spinPlaneNorm[2], y++, 4, 1, 2);

		layoutVecs->addWidget(new QLabel("Distance:", this),
			y, 0, 1, 3);
		layoutVecs->addWidget(m_spinPlaneDist, y++, 3, 1, 3);

		QFrame *separator = new QFrame(this);
		separator->setFrameStyle(QFrame::HLine);
		layoutVecs->addWidget(separator, y++, 0, 1, 6);

		layoutVecs->addWidget(btnPlaneNorm[0], y, 0, 1, 2);
		layoutVecs->addWidget(btnPlaneNorm[1], y, 2, 1, 2);
		layoutVecs->addWidget(btnPlaneNorm[2], y++, 4, 1, 2);
	}

	auto *grid = new QGridLayout(this);
	grid->setHorizontalSpacing(2);
	grid->setVerticalSpacing(2);
	grid->setContentsMargins(4,4,4,4);

	int y = 0;
	grid->addWidget(groupVecs, y++, 0, 1, 1);
	grid->addItem(new QSpacerItem(1, 1, 
		QSizePolicy::Minimum, QSizePolicy::Expanding), y++, 0, 1, 1);

	// plane distance
	connect(m_spinPlaneDist,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &SelectionPropertiesWidget::PlaneDistChanged);

	// plane normal
	for(int i=0; i<3; ++i)
	{
		connect(m_spinPlaneNorm[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this, i](t_real val) -> void
			{
				t_real pos[3];
				for(int j=0; j<3; ++j)
				{
					if(j == i)
						pos[j] = val;
					else
						pos[j] = m_spinPlaneNorm[j]->value();
				}

				emit PlaneNormChanged(pos[0], pos[1], pos[2]);
			});

		connect(btnPlaneNorm[i], &QAbstractButton::clicked,
			[this, i]() -> void
			{
				this->blockSignals(true);
				m_spinPlaneDist->setValue(0.);

				switch(i)
				{
					case 0:
						m_spinPlaneNorm[0]->setValue(1.);
						m_spinPlaneNorm[1]->setValue(0.);
						m_spinPlaneNorm[2]->setValue(0.);
						break;
					case 1:
						m_spinPlaneNorm[0]->setValue(0.);
						m_spinPlaneNorm[1]->setValue(1.);
						m_spinPlaneNorm[2]->setValue(0.);
						break;
					case 2:
						m_spinPlaneNorm[0]->setValue(0.);
						m_spinPlaneNorm[1]->setValue(0.);
						m_spinPlaneNorm[2]->setValue(1.);
						break;
				}

				this->blockSignals(false);
				emit PlaneNormChanged(
					m_spinPlaneNorm[0]->value(),
					m_spinPlaneNorm[1]->value(),
					m_spinPlaneNorm[2]->value());
				emit PlaneDistChanged(m_spinPlaneDist->value());
			});
	}
}


SelectionPropertiesWidget::~SelectionPropertiesWidget()
{
}


void SelectionPropertiesWidget::SetPlaneDist(t_real d)
{
	this->blockSignals(true);
	m_spinPlaneDist->setValue(d);
	this->blockSignals(false);
}


void SelectionPropertiesWidget::SetPlaneNorm(t_real x, t_real y, t_real z)
{
	this->blockSignals(true);
	if(m_spinPlaneNorm[0]) m_spinPlaneNorm[0]->setValue(x);
	if(m_spinPlaneNorm[1]) m_spinPlaneNorm[1]->setValue(y);
	if(m_spinPlaneNorm[2]) m_spinPlaneNorm[2]->setValue(z);
	this->blockSignals(false);
}


/**
 * save the dock widget's settings
 */
boost::property_tree::ptree SelectionPropertiesWidget::Save() const
{
	boost::property_tree::ptree prop;

	// selection plane normal
	prop.put<t_real>("x", m_spinPlaneNorm[0]->value());
	prop.put<t_real>("y", m_spinPlaneNorm[1]->value());
	prop.put<t_real>("z", m_spinPlaneNorm[2]->value());

	// selection plane distance
	prop.put<t_real>("d", m_spinPlaneDist->value());

	return prop;
}


/**
 * load the dock widget's settings
 */
bool SelectionPropertiesWidget::Load(const boost::property_tree::ptree& prop)
{
	// old selection plane normal and distance
	t_real norm0 = m_spinPlaneNorm[0]->value();
	t_real norm1 = m_spinPlaneNorm[1]->value();
	t_real norm2 = m_spinPlaneNorm[2]->value();
	t_real dist = m_spinPlaneDist->value();

	// new selection plane normal and distance
	if(auto opt = prop.get_optional<t_real>("x"); opt)
		norm0 = *opt;
	if(auto opt = prop.get_optional<t_real>("y"); opt)
		norm1 = *opt;
	if(auto opt = prop.get_optional<t_real>("z"); opt)
		norm2 = *opt;
	if(auto opt = prop.get_optional<t_real>("d"); opt)
		dist = *opt;

	// set new values
	SetPlaneNorm(norm0, norm1, norm2);
	SetPlaneDist(dist);

	// emit changes
	emit PlaneNormChanged(norm0, norm1, norm2);
	emit PlaneDistChanged(dist);

	return true;
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// properties dock widget
// --------------------------------------------------------------------------------
SelectionPropertiesDockWidget::SelectionPropertiesDockWidget(QWidget *parent)
	: QDockWidget{parent},
		m_widget{std::make_shared<SelectionPropertiesWidget>(this)}
{
	setObjectName("SelectionPropertiesDockWidget");
	setWindowTitle("Selection Properties");

	setWidget(m_widget.get());
}


SelectionPropertiesDockWidget::~SelectionPropertiesDockWidget()
{
}
// --------------------------------------------------------------------------------
