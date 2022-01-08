/**
 * geometry objects
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Geometry.h"
#include "tlibs2/libs/str.h"

#include <iostream>

namespace pt = boost::property_tree;


// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

/**
 * convert a vector to a serialisable string
 */
std::string geo_vec_to_str(const t_vec& vec)
{
	std::ostringstream ostr;
	ostr.precision(8);

	for(t_real val : vec)
		ostr << val << " ";

	return ostr.str();
}


/**
 * convert a matrix to a serialisable string
 */
std::string geo_mat_to_str(const t_mat& mat)
{
	std::ostringstream ostr;
	ostr.precision(8);

	for(std::size_t i=0; i<(std::size_t)mat.size1(); ++i)
	{
		for(std::size_t j=0; j<(std::size_t)mat.size2(); ++j)
			ostr << mat(i,j) << " ";
	}

	return ostr.str();
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// geometry base class
// ----------------------------------------------------------------------------

Geometry::Geometry()
{
}


Geometry::~Geometry()
{
	//Clear();
}


void Geometry::UpdateTrafo() const
{
	m_trafo = tl2::hom_translation<t_mat, t_real>(
		m_pos[0], m_pos[1], m_pos[2]) * m_rot;

	m_trafo_needs_update = false;
}


const t_mat& Geometry::GetTrafo() const
{
	if(m_trafo_needs_update)
		UpdateTrafo();

	return m_trafo;
}


std::tuple<bool, std::vector<std::shared_ptr<Geometry>>>
Geometry::load(const pt::ptree& prop)
{
	std::vector<std::shared_ptr<Geometry>> geo_objs;
	geo_objs.reserve(prop.size());

	// iterate geometry items
	for(const auto& geo : prop)
	{
		const std::string& geotype = geo.first;
		std::string geoid = geo.second.get<std::string>("<xmlattr>.id", "");
		//std::cout << "type = " << geotype << ", id = " << geoid << std::endl;

		if(geotype == "box")
		{
			auto box = std::make_shared<BoxGeometry>();
			box->m_id = geoid;

			if(box->Load(geo.second))
				geo_objs.emplace_back(std::move(box));
		}
		else if(geotype == "cylinder")
		{
			auto cyl = std::make_shared<CylinderGeometry>();
			cyl->m_id = geoid;

			if(cyl->Load(geo.second))
				geo_objs.emplace_back(std::move(cyl));
		}
		else if(geotype == "sphere")
		{
			auto sph = std::make_shared<SphereGeometry>();
			sph->m_id = geoid;

			if(sph->Load(geo.second))
				geo_objs.emplace_back(std::move(sph));
		}
		else
		{
			std::cerr << "Unknown geometry type \"" << geotype << "\"." << std::endl;
			continue;
		}
	}

	return std::make_tuple(true, geo_objs);
}


/**
 * rotate the object around the z axis
 */
void Geometry::Rotate(t_real angle)
{
	/* TODO
	// create the rotation matrix
	t_vec axis = tl2::create<t_vec>({0, 0, 1});
	t_mat R = tl2::rotation<t_mat, t_vec>(axis, angle);

	// remove translation
	t_vec centre = GetCentre();
	SetCentre(tl2::create<t_vec>({0, 0, 0}));

	// rotate the position vectors
	m_pos1 = R*m_pos1;
	m_pos2 = R*m_pos2;

	// restore translation
	SetCentre(centre);
	*/

	m_trafo_needs_update = true;
}


bool Geometry::Load(const pt::ptree& prop)
{
	// position
	if(auto optPos = prop.get_optional<std::string>("position"); optPos)
	{
		m_pos.clear();

		tl2::get_tokens<t_real>(tl2::trimmed(*optPos), std::string{" \t,;"}, m_pos);
		if(m_pos.size() < 3)
			m_pos.resize(3);
	}

	// colour
	if(auto col = prop.get_optional<std::string>("colour"); col)
	{
		m_colour.clear();
		tl2::get_tokens<t_real>(
			tl2::trimmed(*col), std::string{" \t,;"}, m_colour);

		if(m_colour.size() < 3)
			m_colour.resize(3);
	}

	// texture
	if(auto texture = prop.get_optional<std::string>("texture"); texture)
		m_texture = *texture;
	else
		m_texture = "";

	return true;
}


pt::ptree Geometry::Save() const
{
	pt::ptree prop;

	prop.put<std::string>("<xmlattr>.id", GetId());
	prop.put<std::string>("position", geo_vec_to_str(m_pos));
	prop.put<std::string>("rotation", geo_mat_to_str(m_rot));
	prop.put<std::string>("colour", geo_vec_to_str(m_colour));
	prop.put<std::string>("texture", m_texture);

	return prop;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// box
// ----------------------------------------------------------------------------

BoxGeometry::BoxGeometry()
{
}


BoxGeometry::~BoxGeometry()
{
}


void BoxGeometry::Clear()
{
}


t_vec BoxGeometry::GetCentre() const
{
	using namespace tl2_ops;

	t_vec centre = GetTrafo() * tl2::create<t_vec>({0, 0, 0, 1});
	centre.resize(3);

	return centre;
}


void BoxGeometry::SetCentre(const t_vec& vec)
{
	using namespace tl2_ops;

	t_vec oldcentre = GetCentre();
	t_vec newcentre = vec;
	newcentre.resize(3);

	m_pos += (newcentre - oldcentre);

	m_trafo_needs_update = true;
}


bool BoxGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_height = prop.get<t_real>("height", 1.);
	m_depth = prop.get<t_real>("depth", 0.1);
	m_length = prop.get<t_real>("length", 0.1);

	m_trafo_needs_update = true;
	return true;
}


pt::ptree BoxGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<t_real>("height", m_height);
	prop.put<t_real>("depth", m_depth);
	prop.put<t_real>("length", m_length);

	pt::ptree propBox;
	propBox.put_child("box", prop);
	return propBox;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
BoxGeometry::GetTriangles() const
{
	auto solid = tl2::create_cuboid<t_vec>(m_length*0.5, m_depth*0.5, m_height*0.5);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
}


/**
 * obtain all defining properties of the geometry object
 */
std::vector<ObjectProperty> BoxGeometry::GetProperties() const
{
	std::vector<ObjectProperty> props;
	props.reserve(7);

	props.emplace_back(ObjectProperty{.key="position", .value=m_pos});
	props.emplace_back(ObjectProperty{.key="rotation", .value=m_rot});
	props.emplace_back(ObjectProperty{.key="height", .value=m_height});
	props.emplace_back(ObjectProperty{.key="depth", .value=m_depth});
	props.emplace_back(ObjectProperty{.key="length", .value=m_length});
	props.emplace_back(ObjectProperty{.key="colour", .value=m_colour});
	props.emplace_back(ObjectProperty{.key="texture", .value=m_texture});

	return props;
}


/**
 * set the properties of the geometry object
 */
void BoxGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	for(const auto& prop : props)
	{
		if(prop.key == "position")
			m_pos = std::get<t_vec>(prop.value);
		else if(prop.key == "rotation")
			m_rot = std::get<t_mat>(prop.value);
		else if(prop.key == "height")
			m_height = std::get<t_real>(prop.value);
		else if(prop.key == "depth")
			m_depth = std::get<t_real>(prop.value);
		else if(prop.key == "length")
			m_length = std::get<t_real>(prop.value);
		else if(prop.key == "colour")
			m_colour = std::get<t_vec>(prop.value);
		else if(prop.key == "texture")
			m_texture  = std::get<std::string>(prop.value);
	}

	m_trafo_needs_update = true;
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// cylinder
// ----------------------------------------------------------------------------

CylinderGeometry::CylinderGeometry()
{
}


CylinderGeometry::~CylinderGeometry()
{
}


void CylinderGeometry::Clear()
{
}


t_vec CylinderGeometry::GetCentre() const
{
	using namespace tl2_ops;

	t_vec centre = GetTrafo() * tl2::create<t_vec>({0, 0, 0, 1});
	centre.resize(3);

	return centre;
}


void CylinderGeometry::SetCentre(const t_vec& vec)
{
	using namespace tl2_ops;

	t_vec oldcentre = GetCentre();
	t_vec newcentre = vec;
	newcentre.resize(3);

	m_pos += (newcentre - oldcentre);

	m_trafo_needs_update = true;
}


bool CylinderGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_height = prop.get<t_real>("height", 1.);
	m_radius = prop.get<t_real>("radius", 0.1);

	m_trafo_needs_update = true;
	return true;
}


pt::ptree CylinderGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<t_real>("height", m_height);
	prop.put<t_real>("radius", m_radius);

	pt::ptree propCyl;
	propCyl.put_child("cylinder", prop);
	return propCyl;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
CylinderGeometry::GetTriangles() const
{
	auto solid = tl2::create_cylinder<t_vec>(m_radius, m_height, 1, 32);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
}


/**
 * obtain all defining properties of the geometry object
 */
std::vector<ObjectProperty> CylinderGeometry::GetProperties() const
{
	std::vector<ObjectProperty> props;
	props.reserve(6);

	props.emplace_back(ObjectProperty{.key="position", .value=m_pos});
	props.emplace_back(ObjectProperty{.key="rotation", .value=m_rot});
	props.emplace_back(ObjectProperty{.key="height", .value=m_height});
	props.emplace_back(ObjectProperty{.key="radius", .value=m_radius});
	props.emplace_back(ObjectProperty{.key="colour", .value=m_colour});
	props.emplace_back(ObjectProperty{.key="texture", .value=m_texture});

	return props;
}


/**
 * set the properties of the geometry object
 */
void CylinderGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	for(const auto& prop : props)
	{
		if(prop.key == "position")
			m_pos = std::get<t_vec>(prop.value);
		else if(prop.key == "rotation")
			m_rot = std::get<t_mat>(prop.value);
		else if(prop.key == "height")
			m_height = std::get<t_real>(prop.value);
		else if(prop.key == "radius")
			m_radius = std::get<t_real>(prop.value);
		else if(prop.key == "colour")
			m_colour = std::get<t_vec>(prop.value);
		else if(prop.key == "texture")
			m_texture = std::get<std::string>(prop.value);
	}

	m_trafo_needs_update = true;
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// sphere
// ----------------------------------------------------------------------------

SphereGeometry::SphereGeometry()
{
}


SphereGeometry::~SphereGeometry()
{
}


void SphereGeometry::Clear()
{
}


t_vec SphereGeometry::GetCentre() const
{
	using namespace tl2_ops;

	t_vec centre = GetTrafo() * tl2::create<t_vec>({0, 0, 0, 1});
	centre.resize(3);

	return centre;
}


void SphereGeometry::SetCentre(const t_vec& vec)
{
	using namespace tl2_ops;

	t_vec oldcentre = GetCentre();
	t_vec newcentre = vec;
	newcentre.resize(3);

	m_pos += (newcentre - oldcentre);

	m_trafo_needs_update = true;
}


bool SphereGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = prop.get<t_real>("radius", 0.1);

	m_trafo_needs_update = true;
	return true;
}


pt::ptree SphereGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<t_real>("radius", m_radius);

	pt::ptree propSphere;
	propSphere.put_child("sphere", prop);
	return propSphere;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
SphereGeometry::GetTriangles() const
{
	const int numsubdivs = 2;
	auto solid = tl2::create_icosahedron<t_vec>(1.);
	auto [verts, norms, uvs] = tl2::spherify<t_vec>(
		tl2::subdivide_triangles<t_vec>(
			tl2::create_triangles<t_vec>(solid),
			numsubdivs),
		m_radius);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
}


/**
 * obtain all defining properties of the geometry object
 */
std::vector<ObjectProperty> SphereGeometry::GetProperties() const
{
	std::vector<ObjectProperty> props;
	props.reserve(5);

	props.emplace_back(ObjectProperty{.key="position", .value=m_pos});
	props.emplace_back(ObjectProperty{.key="rotation", .value=m_rot});
	props.emplace_back(ObjectProperty{.key="radius", .value=m_radius});
	props.emplace_back(ObjectProperty{.key="colour", .value=m_colour});
	props.emplace_back(ObjectProperty{.key="texture", .value=m_texture});

	return props;
}


/**
 * set the properties of the geometry object
 */
void SphereGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	for(const auto& prop : props)
	{
		if(prop.key == "position")
			m_pos = std::get<t_vec>(prop.value);
		else if(prop.key == "rotation")
			m_rot = std::get<t_mat>(prop.value);
		else if(prop.key == "radius")
			m_radius = std::get<t_real>(prop.value);
		else if(prop.key == "colour")
			m_colour = std::get<t_vec>(prop.value);
		else if(prop.key == "texture")
			m_texture  = std::get<std::string>(prop.value);
	}

	m_trafo_needs_update = true;
}

// ----------------------------------------------------------------------------
