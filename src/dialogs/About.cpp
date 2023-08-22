/**
 * about dialog
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date Nov-2021
 * @license see 'LICENSE' file
 */

#include "About.h"
#include "src/types.h"

#include <QtCore/QSettings>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/algorithm/string.hpp>

#include <string>


/**
 * identify the compiler and return its website url
 */
static std::string get_compiler_url(
	[[maybe_unused]] const std::string& name)
{
	//namespace alg = boost::algorithm;

#if defined(BOOST_CLANG)
	//if(alg::contains(name, "clang", alg::is_iequal{}))
		return "https://clang.llvm.org";
#elif defined(BOOST_GCC)
	//else if(alg::contains(name, "gcc", alg::is_iequal{}) ||
	//	alg::contains(name, "gnu", alg::is_iequal{}))
		return "https://gcc.gnu.org";
#endif

	// unknown compiler
	return "";
}


About::About(QWidget *parent, const QIcon* progIcon)
	: QDialog(parent)
{
	namespace alg = boost::algorithm;

	setWindowTitle("About " APPL_TITLE);
	setSizeGripEnabled(true);


	// grid layout
	m_grid = std::make_shared<QGridLayout>(this);
	m_grid->setSpacing(4);
	m_grid->setContentsMargins(8, 8, 8, 8);


	// add description items
	//AddTitle(QApplication::applicationDisplayName().toStdString().c_str());
	AddTitle("Gl Scene Editor", progIcon);

	AddSpacer(15);

	AddItem("Author", "Tobias Weber");
	AddItem("Version", QApplication::applicationVersion(),
		"https://doi.org/10.5281/zenodo.5841951");
	AddItem("Repository", "https://github.com/t-weber/gl",
		"https://github.com/t-weber/gl");

	AddSpacer(10);

	AddItem("Compiled with", BOOST_COMPILER,
		get_compiler_url(BOOST_COMPILER).c_str());
	AddItem("Standard library", BOOST_STDLIB);
	AddItem("Boost version",
		alg::replace_all_copy<std::string>(BOOST_LIB_VERSION, "_", ".").c_str(),
		"https://www.boost.org");
	AddItem("Qt version", QT_VERSION_STR, "https://www.qt.io");

	AddSpacer(10);

	AddItem("Build date", QString("%1, %2").arg(__DATE__).arg(__TIME__));

	AddSpacer();


	// buttons
	QDialogButtonBox *buttonbox = new QDialogButtonBox(this);
	buttonbox->setStandardButtons(QDialogButtonBox::Ok);

	QPushButton *aboutQt = new QPushButton("About Qt...", this);
	buttonbox->addButton(aboutQt, QDialogButtonBox::ActionRole);

	connect(buttonbox, &QDialogButtonBox::clicked,
		[this, buttonbox, aboutQt](QAbstractButton *btn) -> void
	{
		// get button role
		QDialogButtonBox::ButtonRole role = buttonbox->buttonRole(btn);

		if(role == QDialogButtonBox::AcceptRole)
			this->accept();
		else if(role == QDialogButtonBox::RejectRole)
			this->reject();
		else if(btn == static_cast<QAbstractButton*>(aboutQt))
			QApplication::aboutQt();
	});

	aboutQt->setAutoDefault(false);
	aboutQt->setDefault(false);
	buttonbox->button(QDialogButtonBox::Ok)->setAutoDefault(true);
	buttonbox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonbox->button(QDialogButtonBox::Ok)->setFocus();
	m_grid->addWidget(buttonbox, m_grid->rowCount(), 0, 1, 2);


	// restore settings
	QSettings settings{this};
	if(settings.contains("about/geo"))
	{
		QByteArray arr{settings.value("about/geo").toByteArray()};
		this->restoreGeometry(arr);
	}
}


/**
 * adds the program title to the end of the grid layout
 */
void About::AddTitle(const char *title, const QIcon* icon)
{
	QLabel *label = new QLabel(title, this);
	label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QFont font = QApplication::font();
	font.setWeight(QFont::Bold);

	int fontscale = 2;
	if(int fontsize = font.pointSize() * fontscale; fontsize)
		font.setPointSize(fontsize);
	else
		font.setPixelSize(font.pixelSize() * fontscale);

	label->setFont(font);

	// add an icon and a title
	if(icon)
	{
		QWidget *titleWidget = new QWidget(this);

		// title widget grid layout
		QGridLayout *titleGrid = new QGridLayout(titleWidget);
		titleGrid->setSpacing(4);
		titleGrid->setContentsMargins(0, 0, 0, 0);

		titleWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

		QLabel *labelIcon = new QLabel(titleWidget);
		QPixmap pixmap = icon->pixmap(48, 48);
		labelIcon->setPixmap(pixmap);
		labelIcon->setFrameShape(QFrame::StyledPanel);
		labelIcon->setFrameShadow(QFrame::Raised);
		labelIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		QSpacerItem *spacer = new QSpacerItem(16, 1, QSizePolicy::Fixed, QSizePolicy::Fixed);
		titleGrid->addWidget(labelIcon, 0, 0, 1, 1, Qt::AlignVCenter);
		titleGrid->addItem(spacer, 0, 1, 1, 1);
		titleGrid->addWidget(label, 0, 2, 1, 1, Qt::AlignHCenter | Qt::AlignVCenter);

		m_grid->addWidget(titleWidget, m_grid->rowCount(), 0, 1, 2);
	}

	// only add a title
	else
	{
		m_grid->addWidget(label, m_grid->rowCount(), 0,
			1, 2, Qt::AlignHCenter);
	}
}


/**
 * adds a description item to the end of the grid layout
 */
void About::AddItem(const QString& key, const QString& val, const QString& url)
{
	QLabel *labelKey = new QLabel(key + ":", this);
	QLabel *labelVal = new QLabel(this);

	if(url == "")
	{
		labelVal->setText(val + ".");
		labelVal->setOpenExternalLinks(false);
	}
	else
	{
		labelVal->setText("<a href=\"" + url + "\">" + val + "</a>.");
		labelVal->setOpenExternalLinks(true);
	}

	labelKey->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	labelVal->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	int row = m_grid->rowCount();
	m_grid->addWidget(labelKey, row, 0, 1, 1);
	m_grid->addWidget(labelVal, row, 1, 1, 1);
}


/**
 * adds a spacer to the end of the grid layout
 */
void About::AddSpacer(int size_v)
{
	QSizePolicy::Policy policy_h = QSizePolicy::Fixed;
	QSizePolicy::Policy policy_v = QSizePolicy::Fixed;

	// expanding spacer?
	if(size_v < 0)
	{
		policy_v = QSizePolicy::Expanding;
		size_v = 1;
	}

	QSpacerItem *spacer_end = new QSpacerItem(1, size_v, policy_h, policy_v);
	m_grid->addItem(spacer_end, m_grid->rowCount(), 0, 1, 2);
}


void About::accept()
{
	// save settings
	QSettings settings{this};
	QByteArray geo{this->saveGeometry()};
	settings.setValue("about/geo", geo);

	QDialog::accept();
}


void About::reject()
{
	QDialog::reject();
}
