/**
 * scene
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include <unordered_map>
#include <optional>

#include "Scene.h"
#include "tlibs2/libs/file.h"
#include "tlibs2/libs/str.h"

namespace pt = boost::property_tree;


Scene::Scene()
	: m_sigUpdate{std::make_shared<t_sig_update>()}
{
}


Scene::~Scene()
{
	Clear();
}


/**
 * assign data from another instrument space
 */
Scene::Scene(const Scene& instr)
{
	*this = instr;
}


/**
 * assign data from another instrument space
 */
const Scene& Scene::operator=(const Scene& instr)
{
	this->m_floorlen[0] = instr.m_floorlen[0];
	this->m_floorlen[1] = instr.m_floorlen[1];
	this->m_floorcol = instr.m_floorcol;

	this->m_walls = instr.m_walls;

	this->m_drag_pos_axis_start = instr.m_drag_pos_axis_start;
	this->m_sigUpdate = std::make_shared<t_sig_update>();

	return *this;
}


/**
 * clear all data in the instrument space
 */
void Scene::Clear()
{
	// reset to defaults
	m_floorlen[0] = m_floorlen[1] = 10.;
	m_floorcol = tl2::create<t_vec>({0.5, 0.5, 0.5});

	// clear
	m_walls.clear();

	// remove listeners
	m_sigUpdate = std::make_shared<t_sig_update>();
}


/**
 * load instrument and wall configuration from a property tree
 */
bool Scene::Load(const pt::ptree& prop)
{
	Clear();

	// floor size
	if(auto opt = prop.get_optional<t_real>("floor.len_x"); opt)
		m_floorlen[0] = *opt;
	if(auto opt = prop.get_optional<t_real>("floor.len_y"); opt)
		m_floorlen[1] = *opt;


	// floor colour
	if(auto col = prop.get_optional<std::string>("floor.colour"); col)
	{
		m_floorcol.clear();
		tl2::get_tokens<t_real>(
			tl2::trimmed(*col), std::string{" \t,;"}, m_floorcol);

		if(m_floorcol.size() < 3)
			m_floorcol.resize(3);
	}


	// walls
	if(auto walls = prop.get_child_optional("walls"); walls)
	{
		// iterate wall segments
		for(const auto &wall : *walls)
		{
			auto id = wall.second.get<std::string>("<xmlattr>.id", "");
			auto geo = wall.second.get_child_optional("geometry");
			if(!geo)
				continue;

			if(auto geoobj = Geometry::load(*geo); std::get<0>(geoobj))
				AddWall(std::get<1>(geoobj), id);
		}
	}

	return true;
}


/**
 * save the instrument and wall configuration into a property tree
 */
pt::ptree Scene::Save() const
{
	pt::ptree prop;

	// floor
	prop.put<t_real>(FILE_BASENAME "instrument_space.floor.len_x", m_floorlen[0]);
	prop.put<t_real>(FILE_BASENAME "instrument_space.floor.len_y", m_floorlen[1]);
	prop.put<std::string>(FILE_BASENAME "instrument_space.floor.colour", geo_vec_to_str(m_floorcol));

	// walls
	pt::ptree propwalls;
	for(std::size_t wallidx=0; wallidx<m_walls.size(); ++wallidx)
	{
		const auto& wall = m_walls[wallidx];

		pt::ptree propwall;
		propwall.put<std::string>("<xmlattr>.id", "wall " + std::to_string(wallidx+1));
		propwall.put_child("geometry", wall->Save());

		pt::ptree propwall2;
		propwall2.put_child("wall", propwall);
		propwalls.insert(propwalls.end(), propwall2.begin(), propwall2.end());
	}

	prop.put_child(FILE_BASENAME "instrument_space.walls", propwalls);

	return prop;
}


/**
 * add a wall to the instrument space
 */
void Scene::AddWall(const std::vector<std::shared_ptr<Geometry>>& wallsegs, const std::string& id)
{
	// get individual 3d primitives that comprise this wall
	for(auto& wallseg : wallsegs)
	{
		if(wallseg->GetId() == "")
			wallseg->SetId(id);
		m_walls.push_back(wallseg);
	}
}


/**
 * delete an object (so far only walls)
 */
bool Scene::DeleteObject(const std::string& id)
{
	// find the wall with the given id
	if(auto iter = std::find_if(m_walls.begin(), m_walls.end(), [&id](const std::shared_ptr<Geometry>& wall) -> bool
	{
		return wall->GetId() == id;
	}); iter != m_walls.end())
	{
		m_walls.erase(iter);
		return true;
	}

	// TODO: handle other cases besides walls
	return false;
}


/**
 * rename an object (so far only walls)
 */
bool Scene::RenameObject(const std::string& oldid, const std::string& newid)
{
	// find the wall with the given id
	if(auto iter = std::find_if(m_walls.begin(), m_walls.end(),
		[&oldid](const std::shared_ptr<Geometry>& wall) -> bool
		{
			return wall->GetId() == oldid;
		}); iter != m_walls.end())
	{
		(*iter)->SetId(newid);
		return true;
	}

	// TODO: handle other cases besides walls
	return false;
}


/**
 * rotate an object by the given angle
 */
std::tuple<bool, std::shared_ptr<Geometry>> Scene::RotateObject(const std::string& id, t_real angle)
{
	// find the wall with the given id
	if(auto iter = std::find_if(m_walls.begin(), m_walls.end(), [&id](const std::shared_ptr<Geometry>& wall) -> bool
	{
		return wall->GetId() == id;
	}); iter != m_walls.end())
	{
		(*iter)->Rotate(angle);
		return std::make_tuple(true, *iter);
	}

	// TODO: handle other cases besides walls
	return std::make_tuple(false, nullptr);
}


/**
 * an object is requested to be dragged from the gui
 */
void Scene::DragObject(bool drag_start, const std::string& obj,
	t_real x_start, t_real y_start, t_real x, t_real y)
{
	// cases involving walls
	bool wall_dragged = false;

	for(auto& wall : GetWalls())
	{
		if(wall->GetId() != obj)
			continue;

		t_vec pos_startcur = tl2::create<t_vec>({ x_start, y_start });
		t_vec pos_cur = tl2::create<t_vec>({ x, y });

		t_vec pos_obj3 = wall->GetCentre();
		t_vec pos_obj = pos_obj3;
		pos_obj.resize(2);

		if(drag_start)
			m_drag_pos_axis_start = pos_obj;

		t_vec pos_drag = pos_cur - pos_startcur + m_drag_pos_axis_start;
		pos_obj3[0] = pos_drag[0];
		pos_obj3[1] = pos_drag[1];

		wall->SetCentre(pos_obj3);
		wall_dragged = true;
	}


	if(wall_dragged)
		EmitUpdate();
}


/**
 * load an instrument space definition from a property tree
 */
std::pair<bool, std::string> Scene::load(
	/*const*/ pt::ptree& prop, Scene& instrspace, const std::string* filename)
{
	std::string unknown = "<unknown>";
	if(!filename) filename = &unknown;

	// get variables from config file
	std::unordered_map<std::string, std::string> propvars{};

	if(auto vars = prop.get_child_optional(FILE_BASENAME "variables"); vars)
	{
		// iterate variables
		for(const auto &var : *vars)
		{
			const auto& key = var.first;
			std::string val = var.second.get<std::string>("<xmlattr>.value", "");
			//std::cout << key << " = " << val << std::endl;

			propvars.insert(std::make_pair(key, val));
		}
	}

	// load instrument definition
	if(auto instr = prop.get_child_optional(FILE_BASENAME "instrument_space"); instr)
	{
		if(!instrspace.Load(*instr))
			return std::make_pair(false, "Instrument configuration \"" +
				*filename + "\" could not be loaded.");
	}
	else
	{
		return std::make_pair(false, "No instrument definition found in \"" +
			*filename + "\".");
	}

	std::ostringstream timestamp;
	if(auto optTime = prop.get_optional<t_real>(FILE_BASENAME "timestamp"); optTime)
		timestamp << tl2::epoch_to_str(*optTime);;

	return std::make_pair(true, timestamp.str());
}


/**
 * load an instrument space definition from an xml file
 */
std::pair<bool, std::string> Scene::load(
	const std::string& filename, Scene& instrspace)
{
	if(filename == "" || !fs::exists(fs::path(filename)))
		return std::make_pair(false, "Instrument file \"" + filename + "\" does not exist.");

	// open xml
	std::ifstream ifstr{filename};
	if(!ifstr)
		return std::make_pair(false, "Could not read instrument file \"" + filename + "\".");

	// read xml
	pt::ptree prop;
	pt::read_xml(ifstr, prop);
	// check format and version
	if(auto opt = prop.get_optional<std::string>(FILE_BASENAME "ident");
		!opt || *opt != APPL_IDENT)
	{
		return std::make_pair(false, "Instrument file \"" + filename +
			"\" has invalid identifier.");
	}

	return load(prop, instrspace, &filename);
}


/**
 * get the properties of a geometry object in the instrument space
 */
std::vector<ObjectProperty> Scene::GetProperties(const std::string& obj) const
{
	// find the wall with the given id
	if(auto iter = std::find_if(m_walls.begin(), m_walls.end(),
		[&obj](const std::shared_ptr<Geometry>& wall) -> bool
		{
			return wall->GetId() == obj;
		}); iter != m_walls.end())
	{
		return (*iter)->GetProperties();
	}

	return {};
}


/**
 * set the properties of a geometry object in the instrument space
 */
std::tuple<bool, std::shared_ptr<Geometry>> Scene::SetProperties(
	const std::string& obj, const std::vector<ObjectProperty>& props)
{
	// find the wall with the given id
	if(auto iter = std::find_if(m_walls.begin(), m_walls.end(),
		[&obj](const std::shared_ptr<Geometry>& wall) -> bool
		{
			return wall->GetId() == obj;
		}); iter != m_walls.end())
	{
		(*iter)->SetProperties(props);
		return std::make_tuple(true, *iter);
	}

	return std::make_tuple(false, nullptr);
}
