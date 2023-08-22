/**
 * about dialog
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date Nov-2021
 * @license see 'LICENSE' file
 */

#ifndef __GL_ABOUT_H__
#define __GL_ABOUT_H__

#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>

#include <memory>

#include "src/common/Resources.h"


class About : public QDialog
{
public:
	About(QWidget *parent = nullptr, const QIcon* progIcon = nullptr);
	virtual ~About() = default;

	About(const About&) = delete;
	const About& operator=(const About&) = delete;


protected:
	virtual void accept() override;
	virtual void reject() override;

	void AddTitle(const char *title, const QIcon* progIcon = nullptr);
	void AddItem(const QString& key, const QString& val, const QString& url = "");
	void AddSpacer(int size_v = -1);


private:
	std::shared_ptr<QGridLayout> m_grid{};
};

#endif
