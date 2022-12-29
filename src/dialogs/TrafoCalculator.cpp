/**
 * transformation calculator
 * @author Tobias Weber <tweber@ill.fr>
 * @date 29-dec-2022
 * @license GPLv3, see 'LICENSE' file
 */

#include "TrafoCalculator.h"
#include "src/settings_variables.h"

#include <QtGui/QPainter>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QDialogButtonBox>


TrafoCalculator::TrafoCalculator(QWidget* pParent, QSettings *sett)
	: QDialog{pParent}, m_sett(sett)
{
	setWindowTitle("Transformation Calculator");
	setSizeGripEnabled(true);

	// buttons
	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok);

	// grid
	auto grid_dlg = new QGridLayout(this);
	grid_dlg->setSpacing(4);
	grid_dlg->setContentsMargins(12,12,12,12);
	grid_dlg->addWidget(buttons, 1,1,1,1);

	// restore settings
	if(m_sett)
	{
		// restore dialog geometry
		if(m_sett->contains("trafocalc/geo"))
			restoreGeometry(m_sett->value("trafocalc/geo").toByteArray());
		else
			resize(500, 500);
	}


	// connections
	connect(buttons, &QDialogButtonBox::accepted,
		this, &TrafoCalculator::accept);
	connect(buttons, &QDialogButtonBox::rejected,
		this, &TrafoCalculator::reject);
}


TrafoCalculator::~TrafoCalculator()
{
}


void TrafoCalculator::UpdateGeoTree(const Scene& scene)
{
}


/**
 * close the dialog
 */
void TrafoCalculator::accept()
{
	if(m_sett)
	{
		// save dialog geometry
		m_sett->setValue("trafocalc/geo", saveGeometry());
	}

	QDialog::accept();
}
// ---------------------------------------------------------------------------- 
