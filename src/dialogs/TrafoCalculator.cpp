/**
 * transformation calculator
 * @author Tobias Weber <tweber@ill.fr>
 * @date 29-dec-2022
 * @license GPLv3, see 'LICENSE' file
 */

#include "TrafoCalculator.h"

#include <QtGui/QPainter>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QTabWidget>

#include "src/settings_variables.h"
#include "src/Geometry.h"
#include "src/types.h"


TrafoCalculator::TrafoCalculator(QWidget* pParent, QSettings *sett)
	: QDialog{pParent}, m_sett(sett)
{
	setWindowTitle("Transformation Calculator");
	setSizeGripEnabled(true);

	// tabs
	QTabWidget *tabs = new QTabWidget(this);
	QWidget *rotationPanel = new QWidget(tabs);
	QWidget *portalPanel = new QWidget(tabs);

	// buttons
	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok);

	// tab panels
	tabs->addTab(rotationPanel, "Rotation");
	tabs->addTab(portalPanel, "Portal");

	// rotation tab
	QLabel *labelAxis = new QLabel("Axis: ");
	QLabel *labelAngle = new QLabel("Angle (deg.): ");
	m_spinAxis[0] = new QDoubleSpinBox(rotationPanel);
	m_spinAxis[1] = new QDoubleSpinBox(rotationPanel);
	m_spinAxis[2] = new QDoubleSpinBox(rotationPanel);
	m_spinAxis[2]->setValue(1);
	m_spinAngle = new QDoubleSpinBox(rotationPanel);
	m_spinAngle->setMinimum(-180);
	m_spinAngle->setMaximum(180);
	labelAxis->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});
	labelAngle->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});
	m_textRotation = new QTextEdit(rotationPanel);
	m_textRotation->setReadOnly(true);

	// rotation grid
	auto grid_rotation = new QGridLayout(rotationPanel);
	grid_rotation->setSpacing(4);
	grid_rotation->setContentsMargins(8, 8, 8, 8);
	grid_rotation->addWidget(labelAxis, 0, 0, 1, 1);
	grid_rotation->addWidget(m_spinAxis[0], 0, 1, 1, 1);
	grid_rotation->addWidget(m_spinAxis[1], 0, 2, 1, 1);
	grid_rotation->addWidget(m_spinAxis[2], 0, 3, 1, 1);
	grid_rotation->addWidget(labelAngle, 1, 0, 1, 1);
	grid_rotation->addWidget(m_spinAngle, 1, 1, 1, 1);
	grid_rotation->addWidget(m_textRotation, 2, 0, 1, 4);

	// portal tab
	QLabel *labelPortal1 = new QLabel("Start: ");
	QLabel *labelPortal2 = new QLabel("Target: ");
	labelPortal1->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});
	labelPortal2->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});
	m_comboPortal1 = new QComboBox(portalPanel);
	m_comboPortal2 = new QComboBox(portalPanel);
	m_checkPortalTranslation = new QCheckBox("Only Translation", portalPanel);
	m_textPortal = new QTextEdit(portalPanel);
	m_textPortal->setReadOnly(true);

	// portal grid
	auto grid_portal = new QGridLayout(portalPanel);
	grid_portal->setSpacing(4);
	grid_portal->setContentsMargins(8, 8, 8, 8);
	grid_portal->addWidget(labelPortal1, 0, 0, 1, 1);
	grid_portal->addWidget(m_comboPortal1, 0, 1, 1, 1);
	grid_portal->addWidget(labelPortal2, 1, 0, 1, 1);
	grid_portal->addWidget(m_comboPortal2, 1, 1, 1, 1);
	grid_portal->addWidget(m_checkPortalTranslation, 2, 0, 1, 2);
	grid_portal->addWidget(m_textPortal, 3, 0, 1, 2);

	// main grid
	auto grid_dlg = new QGridLayout(this);
	grid_dlg->setSpacing(4);
	grid_dlg->setContentsMargins(12, 12, 12, 12);
	grid_dlg->addWidget(tabs, 0, 0, 1, 1);
	grid_dlg->addWidget(buttons, 1, 0, 1, 1);

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
	for(QDoubleSpinBox* spin : {m_spinAxis[0], m_spinAxis[1], m_spinAxis[2], m_spinAngle})
	{
		connect(spin,
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &TrafoCalculator::CalculateRotation);
	}
	for(QComboBox* combo : {m_comboPortal1, m_comboPortal2})
	{
		connect(combo,
			static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &TrafoCalculator::CalculatePortal);
	}
	connect(m_checkPortalTranslation, &QCheckBox::toggled,
		this, &TrafoCalculator::CalculatePortal);

	CalculateRotation();
	CalculatePortal();
}


TrafoCalculator::~TrafoCalculator()
{
}


/**
 * get objects in scene
 */
void TrafoCalculator::UpdateGeoTree(const Scene& scene)
{
	// remember the current instrument space
	m_scene = &scene;

	if(!m_comboPortal1 || !m_comboPortal2)
		return;

	m_comboPortal1->clear();
	m_comboPortal2->clear();

	for(const auto& obj : scene.GetObjects())
	{
		const std::string& name = obj->GetId();

		m_comboPortal1->addItem(name.c_str());
		m_comboPortal2->addItem(name.c_str());
	}
}


void set_result(QTextEdit* edit, const t_mat& mat)
{
	std::ostringstream ostrResult;
	ostrResult.precision(g_prec);

	ostrResult << "<p>Transformation Matrix:\n";
	ostrResult << "<table style=\"border:0px\">\n";
	for(std::size_t i=0; i<mat.size1(); ++i)
	{
		ostrResult << "\t<tr>\n";
		for(std::size_t j=0; j<mat.size2(); ++j)
		{
			ostrResult << "\t\t<td style=\"padding-right:8px\">";

			const t_real val = mat(i, j);
			const t_real rounded = std::round(val); 
			if(std::abs(rounded - val) <= g_eps)
				ostrResult << rounded;
			else
				ostrResult << val;

			ostrResult << "</td>\n";
		}
		ostrResult << "\t</tr>\n";
	}

	ostrResult << "</table>";
	ostrResult << "</p>\n";

	ostrResult << "<p>Single-Line String:<br>";
	ostrResult << geo_mat_to_str(mat);
	ostrResult << "</p>\n";

	if(auto [matInv, okInv] = m::inv<t_mat, t_vec>(mat); okInv)
	{
		ostrResult << "<br><p>Inverse Transformation Matrix:\n";
		ostrResult << "<table style=\"border:0px\">\n";
		for(std::size_t i=0; i<matInv.size1(); ++i)
		{
			ostrResult << "\t<tr>\n";
			for(std::size_t j=0; j<matInv.size2(); ++j)
			{
				ostrResult << "\t\t<td style=\"padding-right:8px\">";

				const t_real val = matInv(i, j);
				const t_real rounded = std::round(val); 
				if(std::abs(rounded - val) <= g_eps)
					ostrResult << rounded;
				else
					ostrResult << val;

				ostrResult << "</td>\n";
			}
			ostrResult << "\t</tr>\n";
		}

		ostrResult << "</table>";
		ostrResult << "</p>\n";

		ostrResult << "<p>Single-Line String:<br>";
		ostrResult << geo_mat_to_str(matInv);
		ostrResult << "</p>\n";
	}

	edit->setHtml(ostrResult.str().c_str());
}


void TrafoCalculator::CalculateRotation()
{
	if(!m_spinAngle || !m_textRotation)
		return;

	t_vec axis = m::create<t_vec>({
		m_spinAxis[0]->value(),
		m_spinAxis[1]->value(),
		m_spinAxis[2]->value() });
	t_real angle = m_spinAngle->value() / 180. * m::pi<t_real>;

	m_textRotation->clear();

	t_mat mat = m::rotation<t_mat, t_vec>(axis, angle, false);
	set_result(m_textRotation, mat);
}


void TrafoCalculator::CalculatePortal()
{
	if(!m_comboPortal1 || !m_comboPortal2 || !m_textPortal || !m_scene)
		return;

	m_textPortal->clear();

	bool only_trans = m_checkPortalTranslation->isChecked();

	std::string start_name = m_comboPortal1->currentText().toStdString();
	std::string target_name = m_comboPortal2->currentText().toStdString();

	auto start = m_scene->FindObject(start_name);
	auto target = m_scene->FindObject(target_name);
	if(!start || !target)
	{
		m_textPortal->setText("Invalid start or target object.");
		return;
	}

	if(only_trans)
	{
		t_mat matStart = m::unit<t_mat>(4);
		t_mat matTarget = m::unit<t_mat>(4);
		m::set_col<t_mat, t_vec>(matStart, start->GetPosition(), 3);
		m::set_col<t_mat, t_vec>(matTarget, -target->GetPosition(), 3);

		t_mat mat = matTarget * matStart;
		set_result(m_textPortal, mat);
	}
	else
	{
		const auto& matStart = start->GetTrafo();
		const auto& matTarget = target->GetTrafo();

		auto [mat, mat_ok] = m::inv<t_mat, t_vec>(matTarget);
		if(!mat_ok)
		{
			m_textPortal->setText("Cannot invert target matrix.");
			return;
		}

		mat = mat * matStart;
		set_result(m_textPortal, mat);
	}
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
