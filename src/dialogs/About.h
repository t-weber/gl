/**
 * about dialog
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TAKIN_PATHS_ABOUT_H__
#define __TAKIN_PATHS_ABOUT_H__


#include <QtWidgets/QDialog>
#include <QtCore/QSettings>


class AboutDlg : public QDialog
{
public:
	AboutDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~AboutDlg();

	AboutDlg(const AboutDlg& other);
	const AboutDlg& operator=(const AboutDlg&);


protected:
	virtual void accept() override;


private:
	QSettings *m_sett{nullptr};
};


#endif
