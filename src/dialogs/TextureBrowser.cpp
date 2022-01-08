/**
 * texture browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 19-dec-2021
 * @license GPLv3, see 'LICENSE' file
 * @note Forked on 19-dec-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 */

#include "TextureBrowser.h"
#include "src/settings_variables.h"

#include <QtGui/QPainter>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QDialogButtonBox>



// ----------------------------------------------------------------------------
ImageWidget::ImageWidget(QWidget* parent) : QFrame(parent)
{
	setFrameStyle(int(QFrame::Panel) | int(QFrame::Sunken));
}


void ImageWidget::SetImage(const QString& img)
{
	if(img.isEmpty())
		m_img = QPixmap();

	if(!m_img.load(img))
		m_img = QPixmap();

	update();
}


void ImageWidget::paintEvent(QPaintEvent *evt)
{
	QFrame::paintEvent(evt);

	if(!m_img.isNull())
	{
		QPainter painter{};
		painter.begin(this);
		const int pad = 2;
		painter.drawPixmap(pad, pad,
			width()-2*pad, height()-2*pad, m_img);
		painter.end();
	}
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
TextureBrowser::TextureBrowser(QWidget* pParent, QSettings *sett)
	: QDialog{pParent}, m_sett(sett)
{
	setWindowTitle("Texture Browser");
	setSizeGripEnabled(true);

	// list widget
	m_list = new QListWidget(this);
	m_list->setSortingEnabled(true);
	m_list->setMouseTracking(true);

	QPushButton *btnAddImage = new QPushButton("Add Image...", this);
	QPushButton *btnDelImage = new QPushButton("Remove Image", this);

	// list widget grid
	QWidget *widget_list = new QWidget(this);
	auto grid_list = new QGridLayout(widget_list);
	grid_list->setSpacing(4);
	grid_list->setContentsMargins(0,0,0,0);
	grid_list->addWidget(m_list, 0,0,1,1);
	grid_list->addWidget(btnAddImage, 1,0,1,1);
	grid_list->addWidget(btnDelImage, 2,0,1,1);

	// image widget
	m_image = new ImageWidget(this);

	// buttons
	m_checkTextures = new QCheckBox("Enable Texture Mapping", this);
	m_checkTextures->setChecked(false);
	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok);

	// splitter
	m_splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter->addWidget(widget_list);
	m_splitter->addWidget(m_image);
	m_splitter->setStretchFactor(0, 1);
	m_splitter->setStretchFactor(1, 4);

	// grid
	auto grid_dlg = new QGridLayout(this);
	grid_dlg->setSpacing(4);
	grid_dlg->setContentsMargins(12,12,12,12);
	grid_dlg->addWidget(m_splitter, 0,0,1,2);
	grid_dlg->addWidget(m_checkTextures, 1,0,1,1);
	grid_dlg->addWidget(buttons, 1,1,1,1);

	// restore settings
	if(m_sett)
	{
		// restore dialog geometry
		if(m_sett->contains("texturebrowser/geo"))
			restoreGeometry(m_sett->value("texturebrowser/geo").toByteArray());
		else
			resize(600, 400);

		// restore splitter position
		if(m_sett->contains("texturebrowser/splitter"))
			m_splitter->restoreState(m_sett->value("texturebrowser/splitter").toByteArray());
	}


	// connections
	connect(m_list, &QListWidget::currentItemChanged,
		this, &TextureBrowser::ListItemChanged);
	connect(btnAddImage, &QAbstractButton::clicked,
		this, &TextureBrowser::BrowseTextureFiles);
	connect(btnDelImage, &QAbstractButton::clicked,
		this, &TextureBrowser::DeleteTextures);

	connect(m_checkTextures, &QCheckBox::toggled,
		this, &TextureBrowser::SignalEnableTextures);
	connect(buttons, &QDialogButtonBox::accepted,
		this, &TextureBrowser::accept);
	connect(buttons, &QDialogButtonBox::rejected,
		this, &TextureBrowser::reject);
}


TextureBrowser::~TextureBrowser()
{
}


/**
 * change or add a texture image
 */
void TextureBrowser::ChangeTexture(
	const QString& ident, const QString& filename, bool emit_changes)
{
	// TODO: check if another file with the same
	//       identifier is already in the list

	QListWidgetItem *item = new QListWidgetItem(m_list);
	item->setText(QString("[%1] %2")
		.arg(ident)
		.arg(QFileInfo{filename}.fileName()));
	item->setData(Qt::UserRole, filename);
	m_list->addItem(item);

	if(emit_changes)
		emit SignalChangeTexture(ident, filename);
}


void TextureBrowser::EnableTextures(bool enable, bool emit_changes)
{
	m_checkTextures->setChecked(enable);

	if(emit_changes)
		emit SignalEnableTextures(enable);
}


void TextureBrowser::BrowseTextureFiles()
{
	QString dirLast = g_imgpath.c_str();
	if(m_sett)
		dirLast = m_sett->value("cur_texture_dir", dirLast).toString();

	QFileDialog filedlg(this, "Open Image File", dirLast,
		"Images (*.png *.jpg)");
	filedlg.setAcceptMode(QFileDialog::AcceptOpen);
	filedlg.setDefaultSuffix("taspaths");
	filedlg.setViewMode(QFileDialog::Detail);
	filedlg.setFileMode(QFileDialog::ExistingFiles);
	filedlg.setSidebarUrls(QList<QUrl>({
		QUrl::fromLocalFile(g_homepath.c_str()),
		QUrl::fromLocalFile(g_desktoppath.c_str()),
		QUrl::fromLocalFile(g_docpath.c_str())}));

	if(!filedlg.exec())
		return;

	QStringList files = filedlg.selectedFiles();
	for(const QString& file : files)
		ChangeTexture(QFileInfo{file}.baseName(), file, true);

	if(m_sett && files.size())
		m_sett->setValue("cur_texture_dir", QFileInfo(files[0]).path());
}


void TextureBrowser::DeleteTextures()
{
	// if nothing is selected, clear all items
	if(m_list->selectedItems().count() == 0)
		m_list->clear();

	for(QListWidgetItem *item : m_list->selectedItems())
	{
		if(!item)
			continue;
		delete item;
	}
}


void TextureBrowser::ListItemChanged(
	QListWidgetItem* cur, [[maybe_unused]] QListWidgetItem* prev)
{
	if(!cur)
	{
		m_image->SetImage("");
		return;
	}

	m_image->SetImage(cur->data(Qt::UserRole).toString());
}


/**
 * close the dialog
 */
void TextureBrowser::accept()
{
	if(m_sett)
	{
		// save dialog geometry
		m_sett->setValue("texturebrowser/geo", saveGeometry());
		m_sett->setValue("texturebrowser/splitter", m_splitter->saveState());
	}

	QDialog::accept();
}
// ---------------------------------------------------------------------------- 
