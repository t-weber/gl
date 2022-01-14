/**
 * selection plane properties dock widget
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2022
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __SELECTION_PLANE_WIDGET_H__
#define __SELECTION_PLANE_WIDGET_H__

#include <memory>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/signals2/signal.hpp>

#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>

#include "src/types.h"


class SelectionPropertiesWidget : public QWidget
{Q_OBJECT
public:
	SelectionPropertiesWidget(QWidget *parent=nullptr);
	virtual ~SelectionPropertiesWidget();

	SelectionPropertiesWidget(const SelectionPropertiesWidget&) = delete;
	SelectionPropertiesWidget& operator=(const SelectionPropertiesWidget&) = delete;

	// save and load the dock widget's settings
	boost::property_tree::ptree Save() const;
	bool Load(const boost::property_tree::ptree& prop);


public slots:
	void SetPlaneDist(t_real dist);
	void SetPlaneNorm(t_real x, t_real y, t_real z);
	void SetPlaneVisibility(bool);


signals:
	void PlaneDistChanged(t_real angle);
	void PlaneNormChanged(t_real x, t_real y, t_real z);
	void PlaneVisibilityChanged(bool visible);


private:
	QDoubleSpinBox *m_spinPlaneDist{nullptr};
	QDoubleSpinBox *m_spinPlaneNorm[3]{nullptr, nullptr, nullptr};
	QCheckBox *m_checkPlaneVisible{nullptr};
};



class SelectionPropertiesDockWidget : public QDockWidget
{
public:
	SelectionPropertiesDockWidget(QWidget *parent=nullptr);
	virtual ~SelectionPropertiesDockWidget();

	SelectionPropertiesDockWidget(const SelectionPropertiesDockWidget&) = delete;
	const SelectionPropertiesDockWidget& operator=(const SelectionPropertiesDockWidget&) = delete;

	std::shared_ptr<SelectionPropertiesWidget> GetWidget() { return m_widget; }


private:
    std::shared_ptr<SelectionPropertiesWidget> m_widget;
};


#endif
