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
 * assign data from another scene
 */
Scene::Scene(const Scene& scene)
{
	*this = scene;
}


/**
 * assign data from another scene
 */
const Scene& Scene::operator=(const Scene& scene)
{
	this->m_objs = scene.m_objs;

	this->m_drag_pos_axis_start = scene.m_drag_pos_axis_start;
	this->m_sigUpdate = std::make_shared<t_sig_update>();

	return *this;
}


/**
 * clear all data in the scene
 */
void Scene::Clear()
{
	// clear
	m_objs.clear();

	// remove listeners
	m_sigUpdate = std::make_shared<t_sig_update>();
}


/**
 * load scene and object configuration from a property tree
 */
bool Scene::Load(const pt::ptree& prop)
{
	Clear();

	// objects
	if(auto objs = prop.get_child_optional("objects"); objs)
	{
		// iterate objects
		for(const auto &obj : *objs)
		{
			auto id = obj.second.get<std::string>("<xmlattr>.id", "");
			auto geo = obj.second.get_child_optional("geometry");
			if(!geo)
				continue;

			if(auto geoobj = Geometry::load(*geo); std::get<0>(geoobj))
				AddObject(std::get<1>(geoobj), id);
		}
	}

	return true;
}


/**
 * save the scene and object configuration into a property tree
 */
pt::ptree Scene::Save() const
{
	pt::ptree prop;

	// objects
	pt::ptree propobjs;
	for(std::size_t objidx=0; objidx<m_objs.size(); ++objidx)
	{
		const auto& obj = m_objs[objidx];

		pt::ptree propobj;
		propobj.put<std::string>("<xmlattr>.id", "object " + std::to_string(objidx+1));
		propobj.put_child("geometry", obj->Save());

		pt::ptree propobj2;
		propobj2.put_child("object", propobj);
		propobjs.insert(propobjs.end(), propobj2.begin(), propobj2.end());
	}

	prop.put_child(FILE_BASENAME "objects", propobjs);

	return prop;
}


/**
 * add an object to the scene
 */
void Scene::AddObject(
	const std::vector<std::shared_ptr<Geometry>>& objs,
	const std::string& id)
{
	// get individual 3d primitives that comprise this object
	for(auto& obj : objs)
	{
		if(obj->GetId() == "")
			obj->SetId(id);
		m_objs.push_back(obj);
	}
}


/**
 * delete an object
 */
bool Scene::DeleteObject(const std::string& id)
{
	// find the object with the given id
	if(auto iter = std::find_if(m_objs.begin(), m_objs.end(),
		[&id](const std::shared_ptr<Geometry>& obj) -> bool
	{
		return obj->GetId() == id;
	}); iter != m_objs.end())
	{
		m_objs.erase(iter);
		return true;
	}

	return false;
}


/**
 * rename an object
 */
bool Scene::RenameObject(const std::string& oldid, const std::string& newid)
{
	// find the object with the given id
	if(auto iter = std::find_if(m_objs.begin(), m_objs.end(),
		[&oldid](const std::shared_ptr<Geometry>& obj) -> bool
		{
			return obj->GetId() == oldid;
		}); iter != m_objs.end())
	{
		(*iter)->SetId(newid);
		return true;
	}

	return false;
}


/**
 * rotate an object by the given angle
 */
std::tuple<bool, std::shared_ptr<Geometry>> 
Scene::RotateObject(const std::string& id, t_real angle, char axis)
{
	// find the object with the given id
	if(auto iter = std::find_if(m_objs.begin(), m_objs.end(), 
		[&id](const std::shared_ptr<Geometry>& obj) -> bool
	{
		return obj->GetId() == id;
	}); iter != m_objs.end())
	{
		(*iter)->Rotate(angle, axis);
		return std::make_tuple(true, *iter);
	}

	return std::make_tuple(false, nullptr);
}


/**
 * an object is requested to be dragged from the gui
 */
void Scene::DragObject(bool drag_start, const std::string& objid,
	const t_vec& pos_startcur, const t_vec& pos_cur)
{
	bool obj_dragged = false;

	for(auto& obj : GetObjects())
	{
		if(obj->GetId() != objid)
			continue;

		t_vec pos_obj = obj->GetCentre();
		if(pos_obj.size() < pos_cur.size())
			pos_obj.resize(pos_cur.size());

		if(drag_start)
			m_drag_pos_axis_start = pos_obj;

		pos_obj = pos_cur - pos_startcur + m_drag_pos_axis_start;
		obj->SetCentre(pos_obj);
		obj_dragged = true;
	}

	if(obj_dragged)
		EmitUpdate();
}


/**
 * load a scene definition from a property tree
 */
std::pair<bool, std::string> Scene::load(
	/*const*/ pt::ptree& prop, Scene& scene, const std::string* filename)
{
	std::string unknown = "<unknown>";
	if(!filename)
		filename = &unknown;

	if(auto scenetree = prop.get_child_optional(FILE_BASENAME); scenetree)
	{
		if(!scene.Load(*scenetree))
			return std::make_pair(false, "Scene configuration \"" +
				*filename + "\" could not be loaded.");
	}
	else
	{
		return std::make_pair(false, "No scene definition found in \"" +
			*filename + "\".");
	}

	std::ostringstream timestamp;
	if(auto optTime = prop.get_optional<t_real>(FILE_BASENAME "timestamp"); optTime)
		timestamp << tl2::epoch_to_str(*optTime);;

	return std::make_pair(true, timestamp.str());
}


/**
 * load a scene definition from an xml file
 */
std::pair<bool, std::string> Scene::load(
	const std::string& filename, Scene& scene)
{
	if(filename == "" || !fs::exists(fs::path(filename)))
		return std::make_pair(false, "Scene file \"" + filename + "\" does not exist.");

	// open xml
	std::ifstream ifstr{filename};
	if(!ifstr)
		return std::make_pair(false, "Could not read scene file \"" + filename + "\".");

	// read xml
	pt::ptree prop;
	pt::read_xml(ifstr, prop);
	// check format and version
	if(auto opt = prop.get_optional<std::string>(FILE_BASENAME "ident");
		!opt || *opt != APPL_IDENT)
	{
		return std::make_pair(false, "Scene file \"" + filename +
			"\" has invalid identifier.");
	}

	return load(prop, scene, &filename);
}


/**
 * get the properties of a geometry object in the scene
 */
std::vector<ObjectProperty> Scene::GetProperties(const std::string& objid) const
{
	// find the object with the given id
	if(auto iter = std::find_if(m_objs.begin(), m_objs.end(),
		[&objid](const std::shared_ptr<Geometry>& obj) -> bool
		{
			return obj->GetId() == objid;
		}); iter != m_objs.end())
	{
		return (*iter)->GetProperties();
	}

	return {};
}


/**
 * set the properties of a geometry object in the scene
 */
std::tuple<bool, std::shared_ptr<Geometry>> Scene::SetProperties(
	const std::string& objid, const std::vector<ObjectProperty>& props)
{
	// find the object with the given id
	if(auto iter = std::find_if(m_objs.begin(), m_objs.end(),
		[&objid](const std::shared_ptr<Geometry>& obj) -> bool
		{
			return obj->GetId() == objid;
		}); iter != m_objs.end())
	{
		(*iter)->SetProperties(props);
		return std::make_tuple(true, *iter);
	}

	return std::make_tuple(false, nullptr);
}
