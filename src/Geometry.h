/**
 * geometry objects
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @note some code forked from my private "misc" project: https://github.com/t-weber/misc
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GEO_OBJ_H__
#define __GEO_OBJ_H__

#include <tuple>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <chrono>

#include <boost/property_tree/ptree.hpp>

#ifdef USE_BULLET
	#include <BulletDynamics/btBulletDynamicsCommon.h>
	#include <BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h>
#endif

#include "types.h"


// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

// convert a vector to a serialisable string
extern std::string geo_vec_to_str(const t_vec& vec,
	const char* sep = "; ");

// convert a serialised string to a vector
extern t_vec geo_str_to_vec(const std::string& str,
	const char* seps = "|;,");

// convert a matrix to a serialisable string
extern std::string geo_mat_to_str(const t_mat& mat,
	const char* seprow = "| ", const char* sepcol = "; ");

// convert a serialised string to a matrix
extern t_mat geo_str_to_mat(const std::string& str,
	const char* seprow = "|", const char* sepcol = ";");
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// geometry base class
// ----------------------------------------------------------------------------
/**
 * representation of a property of an object in the scene
 * space for easy data exchange
 */
struct ObjectProperty
{
	std::string key{};
	std::variant<t_real, t_int, bool, t_vec, t_mat, std::string> value{};
};


/**
 * geometry base class
 */
class Geometry
{
public:
	Geometry();
	virtual ~Geometry();

	virtual bool Load(const boost::property_tree::ptree& prop);
	virtual boost::property_tree::ptree Save() const;

	virtual const t_mat& GetTrafo() const { return m_trafo; }
	virtual void SetTrafo(const t_mat& trafo) { m_trafo = trafo; }

	virtual t_vec GetPosition() const;
	virtual void SetPosition(const t_vec& vec);

	virtual t_mat GetRotation() const;
	virtual void SetRotation(const t_mat& rot);

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
		GetTriangles() const = 0;

	virtual const std::string& GetId() const { return m_id; }
	virtual void SetId(const std::string& id) { m_id = id; }

	virtual bool IsFixed() const { return m_fixed; }
	virtual void SetFixed(bool b) { m_fixed = b; }

	virtual bool IsLightingEnabled() const { return m_lighting; }
	virtual void SetLighting(bool b) { m_lighting = b; }

	virtual const t_vec& GetColour() const { return m_colour; }
	virtual void SetColour(const t_vec& col) { m_colour = col; }

	virtual const std::string& GetTexture() const { return m_texture; }
	virtual void SetTexture(std::string ident) { m_texture = ident; }

	virtual void Rotate(t_real angle, char axis='z');
	virtual void Rotate(t_real angle, const t_vec& axis);

	virtual std::vector<ObjectProperty> GetProperties() const ;
	virtual void SetProperties(const std::vector<ObjectProperty>& props);

	virtual void tick(const std::chrono::milliseconds& ms);

	static std::tuple<bool, std::vector<std::shared_ptr<Geometry>>>
		load(const boost::property_tree::ptree& prop);

#ifdef USE_BULLET
	virtual void SetMatrixFromState();
	virtual void SetStateFromMatrix();
	virtual void CreateRigidBody();
	virtual void UpdateRigidBody();
	virtual std::shared_ptr<btRigidBody> GetRigidBody() { return m_rigid_body; }
#endif


protected:
	std::string m_id{};
	t_vec m_colour = tl2::create<t_vec>({1, 0, 0});
	bool m_lighting = true;
	std::string m_texture{};

	bool m_fixed = false;
	t_mat m_trafo = tl2::unit<t_mat>(4);

#ifdef USE_BULLET
	std::shared_ptr<btPolyhedralConvexShape> m_shape{};
	std::shared_ptr<btDefaultMotionState> m_state{};
	std::shared_ptr<btRigidBody> m_rigid_body{};

	t_real m_mass = 1.;
#endif
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// plane
// ----------------------------------------------------------------------------
class PlaneGeometry : public Geometry
{
public:
	PlaneGeometry();
	virtual ~PlaneGeometry();

	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
	GetTriangles() const override;

	const t_vec& GetNormal() const { return m_norm; }
	t_real GetWidth() const { return m_width; }
	t_real GetHeight() const { return m_height; }

	void SetNormal(const t_vec& n);
	void SetWidth(t_real w);
	void SetHeight(t_real h);

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;

#ifdef USE_BULLET
	virtual void CreateRigidBody() override;
	virtual void UpdateRigidBody() override;
#endif

private:
	t_vec m_norm = tl2::create<t_vec>({0, 0, 1});
	t_real m_width = 1., m_height = 1.;
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

	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
	GetTriangles() const override;

	t_real GetLength() const { return m_length; }
	t_real GetDepth() const { return m_depth; }
	t_real GetHeight() const { return m_height; }

	void SetLength(t_real l);
	void SetDepth(t_real d);
	void SetHeight(t_real h);

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;

#ifdef USE_BULLET
	virtual void CreateRigidBody() override;
	virtual void UpdateRigidBody() override;
#endif


private:
	t_real m_length = 1., m_depth = 1., m_height = 1.;
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

	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
		GetTriangles() const override;

	t_real GetHeight() const { return m_height; }
	t_real GetRadius() const { return m_radius; }

	void SetHeight(t_real h);
	void SetRadius(t_real rad);

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;


private:
	t_real m_height = 1., m_radius = 1.;
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

	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
	GetTriangles() const override;

	t_real GetRadius() const { return m_radius; }
	void SetRadius(t_real rad);

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;


private:
	t_real m_radius = 1.;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// tetrahedron
// ----------------------------------------------------------------------------
class TetrahedronGeometry : public Geometry
{
public:
	TetrahedronGeometry();
	virtual ~TetrahedronGeometry();

	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
	GetTriangles() const override;

	t_real GetRadius() const { return m_radius; }
	void SetRadius(t_real rad) { m_radius = rad; }

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;


private:
	t_real m_radius = 1.;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// octahedron
// ----------------------------------------------------------------------------
class OctahedronGeometry : public Geometry
{
public:
	OctahedronGeometry();
	virtual ~OctahedronGeometry();

	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
	GetTriangles() const override;

	t_real GetRadius() const { return m_radius; }
	void SetRadius(t_real rad) { m_radius = rad; }

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;


private:
	t_real m_radius = 1.;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// dodecahedron
// ----------------------------------------------------------------------------
class DodecahedronGeometry : public Geometry
{
public:
	DodecahedronGeometry();
	virtual ~DodecahedronGeometry();

	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
	GetTriangles() const override;

	t_real GetRadius() const { return m_radius; }
	void SetRadius(t_real rad) { m_radius = rad; }

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;


private:
	t_real m_radius = 1.;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// icosahedron
// ----------------------------------------------------------------------------
class IcosahedronGeometry : public Geometry
{
public:
	IcosahedronGeometry();
	virtual ~IcosahedronGeometry();

	virtual bool Load(const boost::property_tree::ptree& prop) override;
	virtual boost::property_tree::ptree Save() const override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
	GetTriangles() const override;

	t_real GetRadius() const { return m_radius; }
	void SetRadius(t_real rad) { m_radius = rad; }

	virtual std::vector<ObjectProperty> GetProperties() const override;
	virtual void SetProperties(const std::vector<ObjectProperty>& props) override;


private:
	t_real m_radius = 1.;
};
// ----------------------------------------------------------------------------

#endif
