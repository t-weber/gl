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
#include "settings_variables.h"
#include "src/common/ExprParser.h"

#include "mathlibs/libs/poly_algos.h"

#include <iostream>

namespace pt = boost::property_tree;


// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

/**
 * convert a serialised string to a value
 */
template<class t_val>
std::string geo_val_to_str(const t_val& val)
{
	std::ostringstream ostr;
	ostr.precision(g_prec);
	ostr << val;
	return ostr.str();
}


/**
 * convert a serialised string to a value
 */
template<class t_var = t_real>
t_var geo_str_to_val(const std::string& str)
{
	ExprParser<t_var> parser;
	return parser.Parse(str);
}


/**
 * convert a vector to a serialisable string
 */
std::string geo_vec_to_str(const t_vec& vec, const char* sep)
{
	std::ostringstream ostr;
	ostr.precision(g_prec);

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
t_vec geo_str_to_vec(const std::string& str, const char* _seps)
{
	// get the components of the vector
	std::string seps = _seps;
	std::vector<std::string> tokens;
	boost::split(tokens, str,
		[&seps](auto c) -> bool
		{ return std::find(seps.begin(), seps.end(), c) != seps.end(); },
		boost::token_compress_on);

	t_vec vec = m::create<t_vec>(tokens.size());
	for(std::size_t tokidx=0; tokidx<tokens.size(); ++tokidx)
	{
		// parse the vector component expression to yield a real value
		ExprParser<t_real> parser;
		vec[tokidx] = parser.Parse(tokens[tokidx]);
	}

	return vec;
}


/**
 * convert a matrix to a serialisable string
 */
std::string geo_mat_to_str(const t_mat& mat, const char* seprow, const char* sepcol)
{
	std::ostringstream ostr;
	ostr.precision(g_prec);

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
t_mat geo_str_to_mat(const std::string& str, const char* _seprow, const char* _sepcol)
{
	// get the rows of the matrix
	std::string seprow = _seprow;
	std::string sepcol = _sepcol;
	std::vector<std::string> rowtokens;
	boost::split(rowtokens, str,
		[&seprow](auto c) -> bool
		{ return std::find(seprow.begin(), seprow.end(), c) != seprow.end(); },
		boost::token_compress_on);

	std::size_t ROWS = rowtokens.size();
	t_mat mat = m::zero<t_mat>(ROWS, ROWS);
	ROWS = std::min(ROWS, mat.size1());

	for(std::size_t i=0; i<ROWS; ++i)
	{
		// get the columns of the matrix
		std::vector<std::string> coltokens;
		boost::split(coltokens, rowtokens[i],
			[&sepcol](auto c) -> bool
			{ return std::find(sepcol.begin(), sepcol.end(), c) != sepcol.end(); },
			boost::token_compress_on);


		for(std::size_t j=0; j<ROWS; ++j)
		{
			// parse the matrix component expression to yield a real value
			if(j < coltokens.size())
			{
				ExprParser<t_real> parser;
				mat(i,j) = parser.Parse(coltokens[j]);
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


Geometry& Geometry::operator=(const Geometry& geo)
{
	this->m_id = geo.m_id; // + " (clone)";
	this->m_colour = geo.m_colour;
	this->m_lighting = geo.m_lighting;
	this->m_light_id = geo.m_light_id;
	this->m_texture = geo.m_texture;
	this->m_fixed = geo.m_fixed;
	//this->m_trafo = geo.m_trafo;
	//this->m_det = geo.m_det;
	this->m_portal_id = geo.m_portal_id;
	this->m_portal_trafo = geo.m_portal_trafo;
	this->m_portal_det = geo.m_portal_det;
	this->SetRotation(geo.GetRotation());
	this->SetPosition(geo.GetPosition());

#ifdef USE_BULLET
	this->m_mass = geo.m_mass;
	//UpdateRigidBody();
#endif
	return *this;
}


t_vec Geometry::GetPosition() const
{
	t_vec pos = m::col<t_mat, t_vec>(m_trafo, 3);
	pos.resize(3);
	return pos;
}


void Geometry::SetPosition(const t_vec& vec)
{
	m::set_col<t_mat, t_vec>(m_trafo, vec, 3);

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
	// set rotation part
	for(std::size_t i=0; i<3; ++i)
		for(std::size_t j=0; j<3; ++j)
			m_trafo(i, j) = rot(i, j);

	// ignore translation part for determinant
	m_det = m::det<t_mat, t_vec>(rot);

#ifdef USE_BULLET
	SetStateFromMatrix();
#endif
}


void Geometry::SetPortalTrafo(const t_mat& trafo)
{
	m_portal_trafo = trafo;
	// ignore translation part for determinant
	t_mat33 rot = m::convert<t_mat33>(m_portal_trafo);
	m_portal_det = m::det<t_mat33, t_vec3>(rot);
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
		axis_vec = m::create<t_vec>({1, 0, 0});
	else if(axis == 'y')
		axis_vec = m::create<t_vec>({0, 1, 0});
	else /*if(axis == 'z')*/
		axis_vec = m::create<t_vec>({0, 0, 1});

	Rotate(angle, axis_vec);
}


/**
 * rotate the object around a given axis
 */
void Geometry::Rotate(t_real angle, const t_vec& axis)
{
	t_mat R = m::hom_rotation<t_mat, t_vec>(axis, angle);
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
	props.emplace_back(ObjectProperty{.key="light_id", .value=GetLightId()});
	props.emplace_back(ObjectProperty{.key="texture", .value=GetTexture()});
	props.emplace_back(ObjectProperty{.key="portal_id", .value=GetPortalId()});
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
		else if(prop.key == "light_id")
			SetLightId(std::get<int>(prop.value));
		else if(prop.key == "texture")
			SetTexture(std::get<std::string>(prop.value));
		else if(prop.key == "portal_id")
			SetPortalId(std::get<int>(prop.value));
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
	if(auto optLight = prop.get_optional<int>("light_id"); optLight)
		SetLightId(*optLight);

	// texture
	if(auto texture = prop.get_optional<std::string>("texture"); texture)
		SetTexture(*texture);
	else
		SetTexture("");

	// portal
	if(auto optPort = prop.get_optional<int>("portal_id"); optPort)
		SetPortalId(*optPort);
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
	prop.put<std::string>("light_id", geo_val_to_str(GetLightId()));
	prop.put<std::string>("texture", GetTexture());
	prop.put<std::string>("portal_id", geo_val_to_str(GetPortalId()));
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


PlaneGeometry& PlaneGeometry::operator=(const Geometry& _geo)
{
	Geometry::operator=(_geo);
	const PlaneGeometry& geo = dynamic_cast<const PlaneGeometry&>(_geo);

	this->m_norm = geo.m_norm;
	this->m_width = geo.m_width;
	this->m_height = geo.m_height;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
	return *this;
}


std::shared_ptr<Geometry> PlaneGeometry::clone() const
{
	auto geo = std::make_shared<PlaneGeometry>();
	geo->operator=(dynamic_cast<const Geometry&>(*this));
	return geo;
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
	auto solid = m::create_plane<t_mat, t_vec>(
		m_norm, m_width*0.5, m_height*0.5);
	auto [verts, norms, uvs] = m::create_triangles<t_vec>(solid);

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


BoxGeometry& BoxGeometry::operator=(const Geometry& _geo)
{
	Geometry::operator=(_geo);
	const BoxGeometry& geo = dynamic_cast<const BoxGeometry&>(_geo);

	this->m_length = geo.m_length;
	this->m_depth = geo.m_depth;
	this->m_height = geo.m_height;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
	return *this;
}


std::shared_ptr<Geometry> BoxGeometry::clone() const
{
	auto geo = std::make_shared<BoxGeometry>();
	geo->operator=(dynamic_cast<const Geometry&>(*this));
	return geo;
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
	auto solid = m::create_cube<t_vec>(
		m_length*0.5, m_depth*0.5, m_height*0.5);
	auto [verts, norms, uvs] = m::create_triangles<t_vec>(solid);

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


CylinderGeometry& CylinderGeometry::operator=(const Geometry& _geo)
{
	Geometry::operator=(_geo);
	const CylinderGeometry& geo = dynamic_cast<const CylinderGeometry&>(_geo);

	this->m_height = geo.m_height;
	this->m_radius = geo.m_radius;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
	return *this;
}


std::shared_ptr<Geometry> CylinderGeometry::clone() const
{
	auto geo = std::make_shared<CylinderGeometry>();
	geo->operator=(dynamic_cast<const Geometry&>(*this));
	return geo;
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
	auto solid = m::create_cylinder<t_vec>(m_radius, m_height, 1, 32);
	auto [verts, norms, uvs] = m::create_triangles<t_vec>(solid);

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


SphereGeometry& SphereGeometry::operator=(const Geometry& _geo)
{
	Geometry::operator=(_geo);
	const SphereGeometry& geo = dynamic_cast<const SphereGeometry&>(_geo);

	this->m_radius = geo.m_radius;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
	return *this;
}


std::shared_ptr<Geometry> SphereGeometry::clone() const
{
	auto geo = std::make_shared<SphereGeometry>();
	geo->operator=(dynamic_cast<const Geometry&>(*this));
	return geo;
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
	auto solid = m::create_icosahedron<t_vec>(1.);
	auto [verts, norms, uvs] = m::spherify<t_vec>(
		m::subdivide_triangles<t_vec>(
			m::create_triangles<t_vec>(solid),
			numsubdivs),
		m_radius);

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


TetrahedronGeometry& TetrahedronGeometry::operator=(const Geometry& _geo)
{
	Geometry::operator=(_geo);
	const TetrahedronGeometry& geo = dynamic_cast<const TetrahedronGeometry&>(_geo);

	this->m_radius = geo.m_radius;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
	return *this;
}


std::shared_ptr<Geometry> TetrahedronGeometry::clone() const
{
	auto geo = std::make_shared<TetrahedronGeometry>();
	geo->operator=(dynamic_cast<const Geometry&>(*this));
	return geo;
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
	auto solid = m::create_tetrahedron<t_vec>(m_radius);
	auto [verts, norms, uvs] = m::create_triangles<t_vec>(solid);

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


OctahedronGeometry& OctahedronGeometry::operator=(const Geometry& _geo)
{
	Geometry::operator=(_geo);
	const OctahedronGeometry& geo = dynamic_cast<const OctahedronGeometry&>(_geo);

	this->m_radius = geo.m_radius;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
	return *this;
}


std::shared_ptr<Geometry> OctahedronGeometry::clone() const
{
	auto geo = std::make_shared<OctahedronGeometry>();
	geo->operator=(dynamic_cast<const Geometry&>(*this));
	return geo;
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
	auto solid = m::create_octahedron<t_vec>(m_radius);
	auto [verts, norms, uvs] = m::create_triangles<t_vec>(solid);

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


DodecahedronGeometry& DodecahedronGeometry::operator=(const Geometry& _geo)
{
	Geometry::operator=(_geo);
	const DodecahedronGeometry& geo = dynamic_cast<const DodecahedronGeometry&>(_geo);

	this->m_radius = geo.m_radius;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
	return *this;
}


std::shared_ptr<Geometry> DodecahedronGeometry::clone() const
{
	auto geo = std::make_shared<DodecahedronGeometry>();
	geo->operator=(dynamic_cast<const Geometry&>(*this));
	return geo;
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
	return std::make_tuple(std::vector<t_vec>{}, std::vector<t_vec>{}, std::vector<t_vec>{});

	// TODO
	//auto solid = tl2::create_dodecahedron<t_vec>(m_radius);
	//auto [verts, norms, uvs] = m::create_triangles<t_vec>(solid);

	//return std::make_tuple(verts, norms, uvs);
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


IcosahedronGeometry& IcosahedronGeometry::operator=(const Geometry& _geo)
{
	Geometry::operator=(_geo);
	const IcosahedronGeometry& geo = dynamic_cast<const IcosahedronGeometry&>(_geo);

	this->m_radius = geo.m_radius;

#ifdef USE_BULLET
	UpdateRigidBody();
#endif
	return *this;
}


std::shared_ptr<Geometry> IcosahedronGeometry::clone() const
{
	auto geo = std::make_shared<IcosahedronGeometry>();
	geo->operator=(dynamic_cast<const Geometry&>(*this));
	return geo;
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
	auto solid = m::create_icosahedron<t_vec>(m_radius);
	auto [verts, norms, uvs] = m::create_triangles<t_vec>(solid);

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
