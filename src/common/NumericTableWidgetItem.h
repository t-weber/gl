/**
 * numeric table widget item
 * @author Tobias Weber
 * @date apr-2021
 * @license see 'LICENSE' file
 * @note forked from https://github.com/t-weber/geo
 */

#ifndef __GEO_NUMTABWIDGETITEM_H__
#define __GEO_NUMTABWIDGETITEM_H__


#include <QTableWidgetItem>
#include <sstream>


template<class T = double>
class NumericTableWidgetItem : public QTableWidgetItem
{
public:
	NumericTableWidgetItem(const T& val)
		: QTableWidgetItem(std::to_string(val).c_str()),
		  m_val{val}
	{ }


	virtual ~NumericTableWidgetItem() = default;


	virtual bool operator<(const QTableWidgetItem& item) const override
	{
		T val1{}, val2{};

		std::istringstream{this->text().toStdString()} >> val1;
		std::istringstream{item.text().toStdString()} >> val2;

		return val1 < val2;
        }


	virtual void setData(int itemdatarole, const QVariant& var) override
	{
		if(itemdatarole == Qt::EditRole)
			m_val = var.value<T>();
		QTableWidgetItem::setData(itemdatarole, std::to_string(m_val).c_str());
	}


	virtual QTableWidgetItem* clone() const override
	{
		return new NumericTableWidgetItem<T>(m_val);
	}


	T GetValue() const
	{
		return m_val;
	}


private:
	T m_val{};
};


#endif
