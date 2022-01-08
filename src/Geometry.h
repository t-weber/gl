/**
 * geometry objects
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GEO_OBJ_H__
#define __GEO_OBJ_H__

#include <tuple>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <boost/property_tree/ptree.hpp>

#include "types.h"


// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

// convert a vector to a serialisable string
extern std::string geo_vec_to_str(const t_vec& vec);

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// geometry base class
// ----------------------------------------------------------------------------
/**
 * geometric primitive types
 */
enum class GeometryType
{
	BOX,
	CYLINDER,
	SPHERE
};


/**
 * representation of a property of an object in the scene
 * space for easy data exchange
 */
struct ObjectProperty
{
	std::string key{};
	std::variant<t_real, t_int, t_vec, t_mat, std::string> value{};
};


/**
 * geometry base class
 */
class Geometry
{
public:
	Geometry();
	virtual ~Geometry();

	virtual GeometryType GetType() const = 0;

	virtual void Clear() = 0;
	virtual bool Load(const boost::property_tree::ptree& prop);
	virtual boost::property_tree::ptree Save() const;

	virtual void UpdateTrafo() const;
	virtual const t_mat& GetTrafo() const;
	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
		GetTriangles() const = 0;

	virtual const std::string& GetId() const { return m_id; }
	virtual void SetId(const std::string& id) { m_id = id; }

	virtual t_vec GetCentre() const = 0;
	virtual void SetCentre(const t_vec& vec) = 0;

	virtual const t_vec& GetColour() const { return m_colour; }
	virtual void SetColour(const t_vec& col) { m_colour = col; }

	virtual const std::string& GetTexture() const { return m_texture; }
	virtual void SetTexture(std::string ident) { m_texture = ident; }

	virtual void Rotate(t_real angle);

	virtual std::vector<ObjectProperty> GetProperties() const = 0;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) = 0;

	static std::tuple<bool, std::vector<std::shared_ptr<Geometry>>>
		load(const boost::property_tree::ptree& prop);

protected:
	std::string m_id{};
	t_vec m_colour = tl2::create<t_vec>({1, 0, 0});
	std::string m_texture{};

	t_mat m_rot = tl2::unit<t_mat>(4);
	t_vec m_pos = tl2::create<t_vec>({0, 0, 0});

	mutable bool m_trafo_needs_update = true;
	mutable t_mat m_trafo = tl2::unit<t_mat>(4);
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// box
// ----------------------------------------------------------------------------
class BoxGeometry : public Geometry
{
public:
	BoxGeometry();
	virtual ~BoxGeometry();

	virtual GeometryType GetType() const override { return GeometryType::BOX; }

	virtual void Clear() override;
	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
	GetTriangles() const override;

	virtual t_vec GetCentre() const override;
	virtual void SetCentre(const t_vec& vec) override;

	t_real GetHeight() const { return m_height; }
	t_real GetDepth() const { return m_depth; }
	t_real GetLength() const { return m_length; }

	void SetHeight(t_real h) { m_height = h; m_trafo_needs_update = true; }
	void SetDepth(t_real d) { m_depth = d; m_trafo_needs_update = true; }
	void SetLength(t_real l)  { m_length = l; m_trafo_needs_update = true; }

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;

private:
	t_real m_height = 0, m_depth = 0, m_length = 0;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// cylinder
// ----------------------------------------------------------------------------
class CylinderGeometry : public Geometry
{
public:
	CylinderGeometry();
	virtual ~CylinderGeometry();

	virtual GeometryType GetType() const override { return GeometryType::CYLINDER; }

	virtual void Clear() override;
	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual t_vec GetCentre() const override;
	virtual void SetCentre(const t_vec& vec) override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
		GetTriangles() const override;

	const t_vec& GetPos() const { return m_pos; }
	void SetPos(const t_vec& vec) { m_pos = vec; m_trafo_needs_update = true; }

	t_real GetHeight() const { return m_height; }
	void SetHeight(t_real h) { m_height = h; m_trafo_needs_update = true; }

	t_real GetRadius() const { return m_radius; }
	void SetRadius(t_real rad) { m_radius = rad; m_trafo_needs_update = true; }

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;

private:
	t_real m_height = 0, m_radius = 0;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// sphere
// ----------------------------------------------------------------------------
class SphereGeometry : public Geometry
{
public:
	SphereGeometry();
	virtual ~SphereGeometry();

	virtual GeometryType GetType() const override { return GeometryType::SPHERE; }

	virtual void Clear() override;
	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual t_vec GetCentre() const override;
	virtual void SetCentre(const t_vec& vec) override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
	GetTriangles() const override;

	const t_vec& GetPos() const { return m_pos; }
	void SetPos(const t_vec& vec) { m_pos = vec; m_trafo_needs_update = true; }

	t_real GetRadius() const { return m_radius; }
	void SetRadius(t_real rad) { m_radius = rad; m_trafo_needs_update = true; }

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;

private:
	t_real m_radius = 0;
};
// ----------------------------------------------------------------------------

#endif
