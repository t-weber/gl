/**
 * geometry objects
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @note some code forked from my private "misc" project: https://github.com/t-weber/misc
 * @license GPLv3, see 'LICENSE' file
 *
 * Reference for Bullet:
 *  - https://github.com/bulletphysics/bullet3/blob/master/examples/HelloWorld/HelloWorld.cpp
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
 * convert a serialised string to a value
 */
template<class t_var = t_real>
t_var geo_str_to_val(const std::string& str)
{
	tl2::ExprParser<t_var> parser;
	if(!parser.parse(str))
		throw std::logic_error("Could not parse expression.");
	return parser.eval();
}


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


t_vec Geometry::GetPosition() const
{
	t_vec pos = tl2::col<t_mat, t_vec>(m_trafo, 3);
	pos.resize(3);
	return pos;
}


void Geometry::SetPosition(const t_vec& vec)
{
	tl2::set_col<t_mat, t_vec>(m_trafo, vec, 3);

#ifdef USE_BULLET
	SetStateFromMatrix();
#endif
}


t_mat Geometry::GetRotation() const
{
	t_mat rot = m_trafo;
	rot(0,3) = rot(1,3) = rot(2,3) = t_real(0);
	return rot;
}


void Geometry::SetRotation(const t_mat& rot)
{
	tl2::set_submat<t_mat>(m_trafo, rot, 0, 0, 3, 3);

#ifdef USE_BULLET
	SetStateFromMatrix();
#endif
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
		else if(geotype == "plane")
		{
			auto plane = std::make_shared<PlaneGeometry>();
			plane->m_id = geoid;

			if(plane->Load(geo.second))
				geo_objs.emplace_back(std::move(plane));
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
	SetRotation(R * GetRotation());
}


void Geometry::tick([[maybe_unused]] const std::chrono::milliseconds& ms)
{
#ifdef USE_BULLET
	SetMatrixFromState();
#endif
}



#ifdef USE_BULLET
void Geometry::SetMatrixFromState()
{
	if(!m_rigid_body)
		return;

	btTransform trafo;
	m_rigid_body->getMotionState()->getWorldTransform(trafo);
	btMatrix3x3 mat = trafo.getBasis();
	btVector3 vec = trafo.getOrigin();

	for(int row=0; row<3; ++row)
	{
		for(int col=0; col<3; ++col)
			m_trafo(row, col) = mat.getRow(row)[col];

		m_trafo(row, 3) = vec[row];
	}
}


void Geometry::SetStateFromMatrix()
{
	if(!m_state)
		return;

	t_mat rot = GetRotation();
	t_vec pos = GetPosition();

	btMatrix3x3 mat
	{
		btScalar(rot(0,0)), btScalar(rot(0,1)), btScalar(rot(0,2)),
		btScalar(rot(1,0)), btScalar(rot(1,1)), btScalar(rot(1,2)),
		btScalar(rot(2,0)), btScalar(rot(2,1)), btScalar(rot(2,2))
	};

	btVector3 vec
	{
		btScalar(pos[0]),
		btScalar(pos[1]),
		btScalar(pos[2])
	};

	btTransform trafo{mat, vec};

	if(m_rigid_body)
	{
		m_rigid_body->setWorldTransform(trafo);
		m_rigid_body->getMotionState()->setWorldTransform(trafo);
		m_rigid_body->activate();
	}
	else
	{
		m_state->m_graphicsWorldTrans = trafo;
		m_state->m_startWorldTrans = trafo;
	}
}


void Geometry::CreateRigidBody()
{
}


void Geometry::UpdateRigidBody()
{
}
#endif


/**
 * obtain the general properties of the geometry object
 */
std::vector<ObjectProperty> Geometry::GetProperties() const
{
	std::vector<ObjectProperty> props;

	props.emplace_back(ObjectProperty{.key="position", .value=GetPosition()});
	props.emplace_back(ObjectProperty{.key="rotation", .value=GetRotation()});
	props.emplace_back(ObjectProperty{.key="fixed", .value=IsFixed()});
	props.emplace_back(ObjectProperty{.key="colour", .value=GetColour()});
	props.emplace_back(ObjectProperty{.key="lighting", .value=IsLightingEnabled()});
	props.emplace_back(ObjectProperty{.key="texture", .value=GetTexture()});
	props.emplace_back(ObjectProperty{.key="portal", .value=IsPortal()});
	props.emplace_back(ObjectProperty{.key="portal_trafo", .value=GetPortalTrafo()});

#ifdef USE_BULLET
	props.emplace_back(ObjectProperty{.key="mass", .value=m_mass});
#endif

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
			SetPosition(std::get<t_vec>(prop.value));
		else if(prop.key == "rotation")
			SetRotation(std::get<t_mat>(prop.value));
		else if(prop.key == "fixed")
			SetFixed(std::get<bool>(prop.value));
		else if(prop.key == "colour")
			SetColour(std::get<t_vec>(prop.value));
		else if(prop.key == "lighting")
			SetLighting(std::get<bool>(prop.value));
		else if(prop.key == "texture")
			SetTexture(std::get<std::string>(prop.value));
		else if(prop.key == "portal")
			SetPortal(std::get<bool>(prop.value));
		else if(prop.key == "portal_trafo")
			SetPortalTrafo(std::get<t_mat>(prop.value));
#ifdef USE_BULLET
		else if(prop.key == "mass")
			m_mass = std::get<t_real>(prop.value);
#endif
	}
}


bool Geometry::Load(const pt::ptree& prop)
{
	// position
	if(auto optPos = prop.get_optional<std::string>("position"); optPos)
		SetPosition(geo_str_to_vec(*optPos));

	// rotation
	if(auto optRot = prop.get_optional<std::string>("rotation"); optRot)
		SetRotation(geo_str_to_mat(*optRot));

	// fixed
	if(auto optFixed = prop.get_optional<bool>("fixed"); optFixed)
		SetFixed(*optFixed);

	// colour
	if(auto col = prop.get_optional<std::string>("colour"); col)
	{
		m_colour = geo_str_to_vec(*col);
		if(m_colour.size() < 3)
			m_colour.resize(3);
	}

	// lighting
	if(auto optLight = prop.get_optional<bool>("lighting"); optLight)
		SetLighting(*optLight);

	// texture
	if(auto texture = prop.get_optional<std::string>("texture"); texture)
		SetTexture(*texture);
	else
		SetTexture("");

	// portal
	if(auto optPort = prop.get_optional<bool>("portal"); optPort)
		SetPortal(*optPort);
	if(auto optPort = prop.get_optional<std::string>("portal_trafo"); optPort)
		SetPortalTrafo(geo_str_to_mat(*optPort));

#ifdef USE_BULLET
	if(auto optMass = prop.get_optional<std::string>("mass"); optMass)
		m_mass = geo_str_to_val<t_real>(*optMass);
#endif

	return true;
}


pt::ptree Geometry::Save() const
{
	pt::ptree prop;

	prop.put<std::string>("<xmlattr>.id", GetId());
	prop.put<std::string>("position", geo_vec_to_str(GetPosition()));
	prop.put<std::string>("rotation", geo_mat_to_str(GetRotation()));
	prop.put<std::string>("fixed", IsFixed() ? "1" : "0");
	prop.put<std::string>("colour", geo_vec_to_str(GetColour()));
	prop.put<std::string>("lighting", IsLightingEnabled() ? "1" : "0");
	prop.put<std::string>("texture", GetTexture());
	prop.put<std::string>("portal", IsPortal() ? "1" : "0");
	prop.put<std::string>("portal_trafo", geo_mat_to_str(GetPortalTrafo()));

#ifdef USE_BULLET
	prop.put<t_real>("mass", m_mass);
#endif

	return prop;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// plane
// ----------------------------------------------------------------------------

PlaneGeometry::PlaneGeometry() : Geometry()
{
#ifdef USE_BULLET
	CreateRigidBody();
#endif
}


PlaneGeometry::~PlaneGeometry()
{
}


void PlaneGeometry::SetNormal(const t_vec& n)
{
	m_norm = n;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}


void PlaneGeometry::SetWidth(t_real w)
{
	m_width = w;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}


void PlaneGeometry::SetHeight(t_real h)
{
	m_height = h;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}


#ifdef USE_BULLET
void PlaneGeometry::CreateRigidBody()
{
	m_state = std::make_shared<btDefaultMotionState>();
	SetStateFromMatrix();

	m_shape = std::make_shared<btBoxShape>(
		btVector3
		{
			btScalar(m_width * 0.5),
			btScalar(m_height * 0.5),
			btScalar(0.01)
		});

	m_rigid_body = std::make_shared<btRigidBody>(
		btRigidBody::btRigidBodyConstructionInfo{
			0, m_state.get(), m_shape.get(),
			{0, 0, 0}});
}


void PlaneGeometry::UpdateRigidBody()
{
	if(!m_rigid_body)
		return;

	m_shape->setImplicitShapeDimensions(
		btVector3
		{
			btScalar(m_width * 0.5),
			btScalar(m_height * 0.5),
			btScalar(0.01)
		});
}
#endif


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

	m_width = geo_str_to_val<t_real>(prop.get<std::string>("width", "1."));
	m_height = geo_str_to_val<t_real>(prop.get<std::string>("height", "1."));

#ifdef USE_BULLET
	UpdateRigidBody();
#endif

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
	auto solid = tl2::create_plane<t_mat, t_vec>(
		m_norm, m_width*0.5, m_height*0.5);
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

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// box
// ----------------------------------------------------------------------------

BoxGeometry::BoxGeometry() : Geometry()
{
#ifdef USE_BULLET
	CreateRigidBody();
#endif
}


BoxGeometry::~BoxGeometry()
{
}


void BoxGeometry::SetLength(t_real l)
{
	m_length = l;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}


void BoxGeometry::SetDepth(t_real d)
{
	m_depth = d;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}


void BoxGeometry::SetHeight(t_real h)
{
	m_height = h;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}


#ifdef USE_BULLET
void BoxGeometry::CreateRigidBody()
{
	btScalar mass = m_fixed ? btScalar(0) : btScalar(m_mass);
	btVector3 com{0, 0, 0};

	m_shape = std::make_shared<btBoxShape>(
		btVector3
		{
			btScalar(m_length * 0.5),
			btScalar(m_depth * 0.5),
			btScalar(m_height * 0.5),
		});

	m_shape->calculateLocalInertia(mass, com);

	m_state = std::make_shared<btDefaultMotionState>();
	SetStateFromMatrix();

	m_rigid_body = std::make_shared<btRigidBody>(
		btRigidBody::btRigidBodyConstructionInfo{
			mass,
			m_state.get(), m_shape.get(),
			com});
}


void BoxGeometry::UpdateRigidBody()
{
	if(!m_rigid_body)
		return;

	m_shape->setImplicitShapeDimensions(
		btVector3
		{
			btScalar(m_length * 0.5),
			btScalar(m_depth * 0.5),
			btScalar(m_height * 0.5),
		});

	btScalar mass = m_fixed ? btScalar(0) : btScalar(m_mass);
	btVector3 com{0, 0, 0};

	m_shape->calculateLocalInertia(mass, com);
	m_rigid_body->setMassProps(mass, com);
}
#endif


bool BoxGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_length = geo_str_to_val<t_real>(prop.get<std::string>("length", "1."));
	m_depth = geo_str_to_val<t_real>(prop.get<std::string>("depth", "1."));
	m_height = geo_str_to_val<t_real>(prop.get<std::string>("height", "1."));

#ifdef USE_BULLET
	UpdateRigidBody();
#endif

	return true;
}


pt::ptree BoxGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<t_real>("length", m_length);
	prop.put<t_real>("depth", m_depth);
	prop.put<t_real>("height", m_height);

	pt::ptree propBox;
	propBox.put_child("box", prop);
	return propBox;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
BoxGeometry::GetTriangles() const
{
	auto solid = tl2::create_cuboid<t_vec>(
		m_length*0.5, m_depth*0.5, m_height*0.5);
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

	props.emplace_back(ObjectProperty{.key="length", .value=m_length});
	props.emplace_back(ObjectProperty{.key="depth", .value=m_depth});
	props.emplace_back(ObjectProperty{.key="height", .value=m_height});

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
		if(prop.key == "length")
			m_length = std::get<t_real>(prop.value);
		else if(prop.key == "depth")
			m_depth = std::get<t_real>(prop.value);
		else if(prop.key == "height")
			m_height = std::get<t_real>(prop.value);
	}

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// cylinder
// ----------------------------------------------------------------------------

CylinderGeometry::CylinderGeometry() : Geometry()
{
#ifdef USE_BULLET
	CreateRigidBody();
#endif
}


CylinderGeometry::~CylinderGeometry()
{
}


void CylinderGeometry::SetHeight(t_real h)
{
	m_height = h;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}


void CylinderGeometry::SetRadius(t_real rad)
{
	m_radius = rad;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}


#ifdef USE_BULLET
void CylinderGeometry::CreateRigidBody()
{
	btScalar mass = m_fixed ? btScalar(0) : btScalar(m_mass);
	btVector3 com{0, 0, 0};

	m_shape = std::make_shared<btCylinderShapeZ>(
		btVector3
		{
			btScalar(m_radius),
			btScalar(0.),
			btScalar(m_height * 0.5),
		});

	m_shape->calculateLocalInertia(mass, com);

	m_state = std::make_shared<btDefaultMotionState>();
	SetStateFromMatrix();

	m_rigid_body = std::make_shared<btRigidBody>(
		btRigidBody::btRigidBodyConstructionInfo{
			mass,
			m_state.get(), m_shape.get(),
			com});
}


void CylinderGeometry::UpdateRigidBody()
{
	if(!m_rigid_body)
		return;

	m_shape->setImplicitShapeDimensions(
		btVector3
		{
			btScalar(m_radius),
			btScalar(0.),
			btScalar(m_height * 0.5),
		});

	btScalar mass = m_fixed ? btScalar(0) : btScalar(m_mass);
	btVector3 com{0, 0, 0};

	m_shape->calculateLocalInertia(mass, com);
	m_rigid_body->setMassProps(mass, com);
}
#endif


bool CylinderGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_height = geo_str_to_val<t_real>(prop.get<std::string>("height", "1."));
	m_radius = geo_str_to_val<t_real>(prop.get<std::string>("radius", "0.1"));

#ifdef USE_BULLET
	UpdateRigidBody();
#endif

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

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// sphere
// ----------------------------------------------------------------------------

SphereGeometry::SphereGeometry() : Geometry()
{
#ifdef USE_BULLET
	CreateRigidBody();
#endif
}


SphereGeometry::~SphereGeometry()
{
}


void SphereGeometry::SetRadius(t_real rad)
{
	m_radius = rad;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}


#ifdef USE_BULLET
void SphereGeometry::CreateRigidBody()
{
	btScalar mass = m_fixed ? btScalar(0) : btScalar(m_mass);
	btVector3 com{0, 0, 0};

	m_shape = std::make_shared<btSphereShape>(btScalar(m_radius));
	m_shape->calculateLocalInertia(mass, com);
	m_state = std::make_shared<btDefaultMotionState>();
	SetStateFromMatrix();

	m_rigid_body = std::make_shared<btRigidBody>(
		btRigidBody::btRigidBodyConstructionInfo{
			mass,
			m_state.get(), m_shape.get(),
			com});
}


void SphereGeometry::UpdateRigidBody()
{
	if(!m_rigid_body)
		return;

	m_shape->setImplicitShapeDimensions(
		btVector3
		{
			btScalar(m_radius),
			btScalar(m_radius),
			btScalar(m_radius),
		});

	btScalar mass = m_fixed ? btScalar(0) : btScalar(m_mass);
	btVector3 com{0, 0, 0};

	m_shape->calculateLocalInertia(mass, com);
	m_rigid_body->setMassProps(mass, com);
}
#endif


bool SphereGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = geo_str_to_val<t_real>(prop.get<std::string>("radius", "0.1"));

#ifdef USE_BULLET
	UpdateRigidBody();
#endif

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

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// tetrahedron
// ----------------------------------------------------------------------------

TetrahedronGeometry::TetrahedronGeometry() : Geometry()
{
}


TetrahedronGeometry::~TetrahedronGeometry()
{
}


bool TetrahedronGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = geo_str_to_val<t_real>(prop.get<std::string>("radius", "0.1"));

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
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// octahedron
// ----------------------------------------------------------------------------

OctahedronGeometry::OctahedronGeometry() : Geometry()
{
}


OctahedronGeometry::~OctahedronGeometry()
{
}


bool OctahedronGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = geo_str_to_val<t_real>(prop.get<std::string>("radius", "0.1"));

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
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// dodecahedron
// ----------------------------------------------------------------------------

DodecahedronGeometry::DodecahedronGeometry() : Geometry()
{
}


DodecahedronGeometry::~DodecahedronGeometry()
{
}


bool DodecahedronGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = geo_str_to_val<t_real>(prop.get<std::string>("radius", "0.1"));

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
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// icosahedron
// ----------------------------------------------------------------------------

IcosahedronGeometry::IcosahedronGeometry() : Geometry()
{
}


IcosahedronGeometry::~IcosahedronGeometry()
{
}


bool IcosahedronGeometry::Load(const pt::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	m_radius = geo_str_to_val<t_real>(prop.get<std::string>("radius", "0.1"));

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
}
// ----------------------------------------------------------------------------
