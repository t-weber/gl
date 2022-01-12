/**
 * texture browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 19-dec-2021
 * @license GPLv3, see 'LICENSE' file
 * @note Forked on 19-dec-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 */

#ifndef __GLSCENE_TEXTURE_BROWSER_H__
#define __GLSCENE_TEXTURE_BROWSER_H__

#include <QtCore/QSettings>
#include <QtGui/QPixmap>
#include <QtWidgets/QDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QCheckBox>

#include <vector>
#include <memory>


class ImageWidget : public QFrame
{
public:
	ImageWidget(QWidget* parent);
	virtual ~ImageWidget() = default;

	void SetImage(const QString& img);

protected:
	virtual void paintEvent(QPaintEvent *evt) override;

private:
	QPixmap m_img{};
};



class TextureBrowser : public QDialog
{ Q_OBJECT
public:
	TextureBrowser(QWidget* pParent = nullptr, QSettings *sett = nullptr);
	virtual ~TextureBrowser();

	TextureBrowser(const TextureBrowser&) = delete;
	const TextureBrowser& operator=(const TextureBrowser&) = delete;

	void DeleteTextures();
	void EnableTextures(bool enable, bool emit_changes = true);
	void ChangeTexture(const QString& ident, const QString& filename,
		bool emit_changes = true);


protected:
	virtual void accept() override;

	void ListItemChanged(QListWidgetItem* cur, QListWidgetItem* prev);
	void BrowseTextureFiles();


private:
	QSettings *m_sett{nullptr};

	QSplitter *m_splitter{nullptr};
	QListWidget *m_list{nullptr};
	QCheckBox *m_checkTextures{nullptr};
	ImageWidget *m_image{nullptr};


signals:
	void SignalEnableTextures(bool);
	void SignalChangeTexture(const QString& ident, const QString& filename);
};


#endif
