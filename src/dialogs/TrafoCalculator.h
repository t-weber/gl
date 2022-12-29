/**
 * tranformation calculator
 * @author Tobias Weber <tweber@ill.fr>
 * @date 29-dec-2022
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GLSCENE_TRAFOCALC_H__
#define __GLSCENE_TRAFOCALC_H__

#include <QtCore/QSettings>
#include <QtWidgets/QDialog>

#include <vector>
#include <memory>

#include "src/Scene.h"


class TrafoCalculator : public QDialog
{ Q_OBJECT
public:
	TrafoCalculator(QWidget* pParent = nullptr, QSettings *sett = nullptr);
	virtual ~TrafoCalculator();

	TrafoCalculator(const TrafoCalculator&) = delete;
	const TrafoCalculator& operator=(const TrafoCalculator&) = delete;

	void UpdateGeoTree(const Scene& scene);


protected:
	virtual void accept() override;


private:
	QSettings *m_sett{nullptr};
};


#endif
