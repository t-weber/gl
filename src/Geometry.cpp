/**
 * geometry objects
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Geometry.h"
#include "tlibs2/libs/expr.h"
#include "tlibs2/libs/str.h"

#include <iostream>

namespace pt = boost::property_tree;


// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

/**
 * convert a vector to a serialisable string
 */
std::string geo_vec_to_str(const t_vec& vec, const char* sep)
{
	std::ostringstream ostr;
	ostr.precision(8);

	for(std::size_t i=0; i<vec.size(); ++i)
	{
		ostr << vec[i];
		if(i < vec.size()-1)
			ostr << sep;
	}

	return ostr.str();
}


/**
 * convert a serialised string to a vector
 */
t_vec geo_str_to_vec(const std::string& str, const char* seps)
{
	// get the components of the vector
	std::vector<std::string> tokens;
	tl2::get_tokens<std::string, std::string>(str, seps, tokens);

	t_vec vec = tl2::create<t_vec>(tokens.size());
	for(std::size_t tokidx=0; tokidx<tokens.size(); ++tokidx)
	{
		// parse the vector component expression to yield a real value
		tl2::ExprParser<t_real> parser;
		if(!parser.parse(tokens[tokidx]))
			throw std::logic_error("Could not parse vector expression.");
		vec[tokidx] = parser.eval();
	}

	return vec;
}


/**
 * convert a matrix to a serialisable string
 */
std::string geo_mat_to_str(const t_mat& mat, const char* seprow, const char* sepcol)
{
	std::ostringstream ostr;
	ostr.precision(8);

	for(std::size_t i=0; i<mat.size1(); ++i)
	{
		for(std::size_t j=0; j<mat.size2(); ++j)
		{
			ostr << mat(i,j);
			if(j < mat.size2()-1)
				ostr << sepcol;
		}

		if(i < mat.size1()-1)
			ostr << seprow;
	}

	return ostr.str();
}


/**
 * convert a serialised string to a matrix
 */
t_mat geo_str_to_mat(const std::string& str, const char* seprow, const char* sepcol)
{
	// get the rows of the matrix
	std::vector<std::string> rowtokens;
	tl2::get_tokens<std::string, std::string>(str, seprow, rowtokens);

	std::size_t ROWS = rowtokens.size();
	t_mat mat = tl2::zero<t_mat>(ROWS, ROWS);
	ROWS = std::min(ROWS, mat.size1());

	for(std::size_t i=0; i<ROWS; ++i)
	{
		// get the columns of the matrix
		std::vector<std::string> coltokens;
		tl2::get_tokens<std::string, std::string>(rowtokens[i], sepcol, coltokens);

		for(std::size_t j=0; j<ROWS; ++j)
		{
			// parse the matrix component expression to yield a real value
			if(j < coltokens.size())
			{
				tl2::ExprParser<t_real> parser;
				if(!parser.parse(coltokens[j]))
					throw std::logic_error("Could not parse vector expression.");
				mat(i,j) = parser.eval();
			}
			else
			{
				mat(i,j) = 0;
			}
		}
	}

	return mat;
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


t_vec Geometry::GetCentre() const
{
	return m_pos;
}


void Geometry::SetCentre(const t_vec& vec)
{
	m_pos = vec;
	m_trafo_needs_update = true;
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
		else if(geotype == "tetrahedron")
		{
			auto tetr = std::make_shared<TetrahedronGeometry>();
			tetr->m_id = geoid;

			if(tetr->Load(geo.second))
				geo_objs.emplace_back(std::move(tetr));
		}
		else if(geotype == "octahedron")
		{
			auto octa = std::make_shared<OctahedronGeometry>();
			octa->m_id = geoid;

			if(octa->Load(geo.second))
				geo_objs.emplace_back(std::move(octa));
		}
		else if(geotype == "dodecahedron")
		{
			auto dode = std::make_shared<DodecahedronGeometry>();
			dode->m_id = geoid;

			if(dode->Load(geo.second))
				geo_objs.emplace_back(std::move(dode));
		}
		else if(geotype == "icosahedron")
		{
			auto icosa = std::make_shared<IcosahedronGeometry>();
			icosa->m_id = geoid;

			if(icosa->Load(geo.second))
				geo_objs.emplace_back(std::move(icosa));
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
 * rotate the object around one of the principal axes
 */
void Geometry::Rotate(t_real angle, char axis)
{
	// create the rotation matrix
	t_vec axis_vec;
	if(axis == 'x')
		axis_vec = tl2::create<t_vec>({1, 0, 0});
	else if(axis == 'y')
		axis_vec = tl2::create<t_vec>({0, 1, 0});
	else /*if(axis == 'z')*/
		axis_vec = tl2::create<t_vec>({0, 0, 1});

	Rotate(angle, axis_vec);
}


/**
 * rotate the object around a given axis
 */
void Geometry::Rotate(t_real angle, const t_vec& axis)
{
	t_mat R = tl2::hom_rotation<t_mat, t_vec>(axis, angle);

	m_rot = R*m_rot;
	m_trafo_needs_update = true;
}


/**
 * obtain the general properties of the geometry object
 */
std::vector<ObjectProperty> Geometry::GetProperties() const
{
	std::vector<ObjectProperty> props;

	props.emplace_back(ObjectProperty{.key="position", .value=m_pos});
	props.emplace_back(ObjectProperty{.key="rotation", .value=m_rot});
	props.emplace_back(ObjectProperty{.key="colour", .value=m_colour});
	props.emplace_back(ObjectProperty{.key="texture", .value=m_texture});

	return props;
}


/**
 * set the properties of the geometry object
 */
void Geometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	for(const auto& prop : props)
	{
		if(prop.key == "position")
			m_pos = std::get<t_vec>(prop.value);
		else if(prop.key == "rotation")
			m_rot = std::get<t_mat>(prop.value);
		else if(prop.key == "colour")
			m_colour = std::get<t_vec>(prop.value);
		else if(prop.key == "texture")
			m_texture  = std::get<std::string>(prop.value);
	}

	m_trafo_needs_update = true;
}


bool Geometry::Load(const pt::ptree& prop)
{
	// position
	if(auto optPos = prop.get_optional<std::string>("position"); optPos)
	{
		m_pos = geo_str_to_vec(*optPos);
		if(m_pos.size() < 3)
			m_pos.resize(3);
	}

	// rotation
	if(auto optRot = prop.get_optional<std::string>("rotation"); optRot)
	{
		m_rot = geo_str_to_mat(*optRot);
	}

	// colour
	if(auto col = prop.get_optional<std::string>("colour"); col)
	{
		m_colour = geo_str_to_vec(*col);
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
	std::vector<ObjectProperty> props = Geometry::GetProperties();

	props.emplace_back(ObjectProperty{.key="height", .value=m_height});
	props.emplace_back(ObjectProperty{.key="depth", .value=m_depth});
	props.emplace_back(ObjectProperty{.key="length", .value=m_length});

	return props;
}


/**
 * set the properties of the geometry object
 */
void BoxGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	Geometry::SetProperties(props);

	for(const auto& prop : props)
	{
		if(prop.key == "height")
			m_height = std::get<t_real>(prop.value);
		else if(prop.key == "depth")
			m_depth = std::get<t_real>(prop.value);
		else if(prop.key == "length")
			m_length = std::get<t_real>(prop.value);
	}

	m_trafo_needs_update = true;
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// plane
// ----------------------------------------------------------------------------

PlaneGeometry::PlaneGeometry()
{
}


PlaneGeometry::~PlaneGeometry()
{
}


bool PlaneGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	// normal
	if(auto optPos = prop.get_optional<std::string>("normal"); optPos)
	{
		m_norm = geo_str_to_vec(*optPos);
		if(m_norm.size() < 3)
			m_norm.resize(3);
	}

	m_width = prop.get<t_real>("width", 0.1);
	m_height = prop.get<t_real>("height", 1.);

	m_trafo_needs_update = true;
	return true;
}


pt::ptree PlaneGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<std::string>("normal", geo_vec_to_str(m_norm));
	prop.put<t_real>("width", m_width);
	prop.put<t_real>("height", m_height);

	pt::ptree propPlane;
	propPlane.put_child("plane", prop);
	return propPlane;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
PlaneGeometry::GetTriangles() const
{
	auto solid = tl2::create_plane<t_mat, t_vec>(m_norm, m_width*0.5, m_height*0.5);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
}


/**
 * obtain all defining properties of the geometry object
 */
std::vector<ObjectProperty> PlaneGeometry::GetProperties() const
{
	std::vector<ObjectProperty> props = Geometry::GetProperties();

	props.emplace_back(ObjectProperty{.key="normal", .value=m_norm});
	props.emplace_back(ObjectProperty{.key="width", .value=m_width});
	props.emplace_back(ObjectProperty{.key="height", .value=m_height});

	return props;
}


/**
 * set the properties of the geometry object
 */
void PlaneGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	Geometry::SetProperties(props);

	for(const auto& prop : props)
	{
		if(prop.key == "normal")
			m_norm = std::get<t_vec>(prop.value);
		else if(prop.key == "width")
			m_width = std::get<t_real>(prop.value);
		else if(prop.key == "height")
			m_height = std::get<t_real>(prop.value);
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
	std::vector<ObjectProperty> props = Geometry::GetProperties();

	props.emplace_back(ObjectProperty{.key="height", .value=m_height});
	props.emplace_back(ObjectProperty{.key="radius", .value=m_radius});

	return props;
}


/**
 * set the properties of the geometry object
 */
void CylinderGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	Geometry::SetProperties(props);

	for(const auto& prop : props)
	{
		if(prop.key == "height")
			m_height = std::get<t_real>(prop.value);
		else if(prop.key == "radius")
			m_radius = std::get<t_real>(prop.value);
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
	std::vector<ObjectProperty> props = Geometry::GetProperties();

	props.emplace_back(ObjectProperty{.key="radius", .value=m_radius});

	return props;
}


/**
 * set the properties of the geometry object
 */
void SphereGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	Geometry::SetProperties(props);

	for(const auto& prop : props)
	{
		if(prop.key == "radius")
			m_radius = std::get<t_real>(prop.value);
	}

	m_trafo_needs_update = true;
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// tetrahedron
// ----------------------------------------------------------------------------

TetrahedronGeometry::TetrahedronGeometry()
{
}


TetrahedronGeometry::~TetrahedronGeometry()
{
}


bool TetrahedronGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = prop.get<t_real>("radius", 0.1);

	m_trafo_needs_update = true;
	return true;
}


pt::ptree TetrahedronGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<t_real>("radius", m_radius);

	pt::ptree propSphere;
	propSphere.put_child("tetrahedron", prop);
	return propSphere;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
TetrahedronGeometry::GetTriangles() const
{
	auto solid = tl2::create_tetrahedron<t_vec>(m_radius);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
}


/**
 * obtain all defining properties of the geometry object
 */
std::vector<ObjectProperty> TetrahedronGeometry::GetProperties() const
{
	std::vector<ObjectProperty> props = Geometry::GetProperties();

	props.emplace_back(ObjectProperty{.key="radius", .value=m_radius});

	return props;
}


/**
 * set the properties of the geometry object
 */
void TetrahedronGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	Geometry::SetProperties(props);

	for(const auto& prop : props)
	{
		if(prop.key == "radius")
			m_radius = std::get<t_real>(prop.value);
	}

	m_trafo_needs_update = true;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// octahedron
// ----------------------------------------------------------------------------

OctahedronGeometry::OctahedronGeometry()
{
}


OctahedronGeometry::~OctahedronGeometry()
{
}


bool OctahedronGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = prop.get<t_real>("radius", 0.1);

	m_trafo_needs_update = true;
	return true;
}


pt::ptree OctahedronGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<t_real>("radius", m_radius);

	pt::ptree propSphere;
	propSphere.put_child("octahedron", prop);
	return propSphere;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
OctahedronGeometry::GetTriangles() const
{
	auto solid = tl2::create_octahedron<t_vec>(m_radius);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
}


/**
 * obtain all defining properties of the geometry object
 */
std::vector<ObjectProperty> OctahedronGeometry::GetProperties() const
{
	std::vector<ObjectProperty> props = Geometry::GetProperties();

	props.emplace_back(ObjectProperty{.key="radius", .value=m_radius});

	return props;
}


/**
 * set the properties of the geometry object
 */
void OctahedronGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	Geometry::SetProperties(props);

	for(const auto& prop : props)
	{
		if(prop.key == "radius")
			m_radius = std::get<t_real>(prop.value);
	}

	m_trafo_needs_update = true;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// dodecahedron
// ----------------------------------------------------------------------------

DodecahedronGeometry::DodecahedronGeometry()
{
}


DodecahedronGeometry::~DodecahedronGeometry()
{
}


bool DodecahedronGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = prop.get<t_real>("radius", 0.1);

	m_trafo_needs_update = true;
	return true;
}


pt::ptree DodecahedronGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<t_real>("radius", m_radius);

	pt::ptree propSphere;
	propSphere.put_child("dodecahedron", prop);
	return propSphere;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
DodecahedronGeometry::GetTriangles() const
{
	auto solid = tl2::create_dodecahedron<t_vec>(m_radius);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
}


/**
 * obtain all defining properties of the geometry object
 */
std::vector<ObjectProperty> DodecahedronGeometry::GetProperties() const
{
	std::vector<ObjectProperty> props = Geometry::GetProperties();

	props.emplace_back(ObjectProperty{.key="radius", .value=m_radius});

	return props;
}


/**
 * set the properties of the geometry object
 */
void DodecahedronGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	Geometry::SetProperties(props);

	for(const auto& prop : props)
	{
		if(prop.key == "radius")
			m_radius = std::get<t_real>(prop.value);
	}

	m_trafo_needs_update = true;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// icosahedron
// ----------------------------------------------------------------------------

IcosahedronGeometry::IcosahedronGeometry()
{
}


IcosahedronGeometry::~IcosahedronGeometry()
{
}


bool IcosahedronGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = prop.get<t_real>("radius", 0.1);

	m_trafo_needs_update = true;
	return true;
}


pt::ptree IcosahedronGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<t_real>("radius", m_radius);

	pt::ptree propSphere;
	propSphere.put_child("icosahedron", prop);
	return propSphere;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
IcosahedronGeometry::GetTriangles() const
{
	auto solid = tl2::create_icosahedron<t_vec>(m_radius);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
}


/**
 * obtain all defining properties of the geometry object
 */
std::vector<ObjectProperty> IcosahedronGeometry::GetProperties() const
{
	std::vector<ObjectProperty> props = Geometry::GetProperties();

	props.emplace_back(ObjectProperty{.key="radius", .value=m_radius});

	return props;
}


/**
 * set the properties of the geometry object
 */
void IcosahedronGeometry::SetProperties(const std::vector<ObjectProperty>& props)
{
	Geometry::SetProperties(props);

	for(const auto& prop : props)
	{
		if(prop.key == "radius")
			m_radius = std::get<t_real>(prop.value);
	}

	m_trafo_needs_update = true;
}
// ----------------------------------------------------------------------------
