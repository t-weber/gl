/**
 * simulation properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date 2-september-2023
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __SIM_PROP_WIDGET_H__
#define __SIM_PROP_WIDGET_H__

#include <memory>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/signals2/signal.hpp>

#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>

#include "src/types.h"


class SimPropertiesWidget : public QWidget
{Q_OBJECT
public:
	SimPropertiesWidget(QWidget *parent=nullptr);
	virtual ~SimPropertiesWidget();

	SimPropertiesWidget(const SimPropertiesWidget&) = delete;
	SimPropertiesWidget& operator=(const SimPropertiesWidget&) = delete;

	// save and load the dock widget's settings
	boost::property_tree::ptree Save() const;
	bool Load(const boost::property_tree::ptree& prop);


public slots:
	void SetTimeScale(t_real t);
	void SetMaxTimeStep(t_int dt);


signals:
	void TimeScaleChanged(t_real t);
	void MaxTimeStepChanged(t_int dt);


private:
	QDoubleSpinBox *m_spinTimeScale{nullptr};
	QSpinBox *m_spinMaxTimeStep{nullptr};
};



class SimPropertiesDockWidget : public QDockWidget
{
public:
	SimPropertiesDockWidget(QWidget *parent = nullptr);
	virtual ~SimPropertiesDockWidget();

	SimPropertiesDockWidget(const SimPropertiesDockWidget&) = delete;
	const SimPropertiesDockWidget& operator=(const SimPropertiesDockWidget&) = delete;

	std::shared_ptr<SimPropertiesWidget> GetWidget() { return m_widget; }


private:
    std::shared_ptr<SimPropertiesWidget> m_widget{};
};


#endif
