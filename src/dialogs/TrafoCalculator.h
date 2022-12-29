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
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QDoubleSpinBox>

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


private:
	const Scene* m_scene{};
	QSettings *m_sett{};

	QTextEdit *m_textRotation{};
	QDoubleSpinBox *m_spinAxis[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_spinAngle{};

	QComboBox *m_comboPortal1{}, *m_comboPortal2{};
	QTextEdit *m_textPortal{};


protected slots:
	virtual void accept() override;

	void CalculateRotation();
	void CalculatePortal();
};


#endif
