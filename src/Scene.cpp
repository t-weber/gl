/**
 * scene
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @note some code forked from my private "misc" project: https://github.com/t-weber/misc
 * @license GPLv3, see 'LICENSE' file
 *
 * Reference for Bullet:
 *  - https://github.com/bulletphysics/bullet3/blob/master/examples/HelloWorld/HelloWorld.cpp
 */

#include <unordered_map>
#include <optional>

#if __has_include(<filesystem>)
	#include <filesystem>
	namespace fs = std::filesystem;
#else
	#include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#endif

#include "Scene.h"

namespace pt = boost::property_tree;


Scene::Scene() : m_sigUpdate{std::make_shared<t_sig_update>()}
{
#ifdef USE_BULLET
	m_coll = std::make_shared<btDefaultCollisionConfiguration>(
		btDefaultCollisionConstructionInfo{});
	m_disp = std::make_shared<btCollisionDispatcher/*Mt*/>(m_coll.get());
	m_cache = std::make_shared<btDbvtBroadphase>();
	m_solver = std::make_shared<btSequentialImpulseConstraintSolver>();
	m_world = std::make_shared<btDiscreteDynamicsWorld>(
		m_disp.get(), m_cache.get(), m_solver.get(), m_coll.get());
	m_world->setGravity({0, 0, -9.81});
#endif
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
#ifdef USE_BULLET
	for(auto& obj : m_objs)
	{
		if(auto *rigidbody = obj->GetRigidBody().get(); m_world && rigidbody)
			m_world->removeRigidBody(rigidbody);
	}
#endif

	// clear
	m_objs.clear();

	// remove listeners
	m_sigUpdate = std::make_shared<t_sig_update>();
}


void Scene::tick(const std::chrono::milliseconds& ms)
{
#ifdef USE_BULLET
	if(m_world)
		m_world->stepSimulation(t_real(ms.count()) / 1000.);
#endif

	for(auto& obj : m_objs)
		obj->tick(ms);

#ifdef USE_BULLET
	EmitUpdate();
#endif
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

#ifdef USE_BULLET
		if(auto *rigidbody = obj->GetRigidBody().get(); m_world && rigidbody)
			m_world->addRigidBody(rigidbody);
#endif
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
#ifdef USE_BULLET
		if(auto *rigidbody = (*iter)->GetRigidBody().get(); m_world && rigidbody)
			m_world->removeRigidBody(rigidbody);
#endif

		m_objs.erase(iter);
		return true;
	}

	return false;
}


/**
 * clone an object
 */
std::shared_ptr<Geometry> Scene::CloneObject(const std::string& id)
{
	// find the object with the given id
	if(auto iter = std::find_if(m_objs.begin(), m_objs.end(),
		[&id](const std::shared_ptr<Geometry>& obj) -> bool
	{
		return obj->GetId() == id;
	}); iter != m_objs.end())
	{
		auto obj = (*iter)->clone();

		// assign a new, unique object id
		std::size_t nr = 1;
		while(true)
		{
			std::ostringstream ostrid;
			ostrid << id << " (clone " << (nr++) << ")";

			bool already_assigned = false;
			for(const auto& oldobj : m_objs)
			{
				if(oldobj->GetId() == ostrid.str())
				{
					already_assigned = true;
					break;
				}
			}

			if(!already_assigned)
			{
				obj->SetId(ostrid.str());
				break;
			}
		}

		AddObject(std::vector<std::shared_ptr<Geometry>>{{obj}}, obj->GetId());
		return obj;
	}

	return nullptr;
}


/**
 * rename an object
 */
bool Scene::RenameObject(const std::string& oldid, const std::string& newid)
{
	if(auto obj = FindObject(oldid); obj)
	{
		obj->SetId(newid);
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
	if(auto obj = FindObject(id); obj)
	{
		obj->Rotate(angle, axis);
		return std::make_tuple(true, obj);
	}

	return std::make_tuple(false, nullptr);
}


/**
 * an object is requested to be dragged from the gui
 */
void Scene::DragObject(bool drag_start,
	const std::string& objid,
	[[__maybe_unused__]] const t_vec& pos_startcur,
	const t_vec& pos_cur,
	[[__maybe_unused__]] MouseDragMode drag_mode)
{
	bool obj_dragged = false;

	if(auto obj = FindObject(objid); obj)
	{
		if(obj->IsFixed())
			return;

		t_vec pos_obj = obj->GetPosition();
		if(pos_obj.size() < pos_cur.size())
			pos_obj.resize(pos_cur.size());

		if(drag_start)
			m_drag_pos_axis_start = pos_obj;

#ifdef USE_BULLET
		if(drag_mode == MouseDragMode::FORCE || drag_mode == MouseDragMode::MOMENTUM)
		{
			const t_vec vec_diff = pos_cur - m_drag_pos_axis_start;
			const btVector3 vecDiff
			{
				btScalar(vec_diff[0]),
				btScalar(vec_diff[1]),
				btScalar(vec_diff[2])
			};

			if(drag_mode == MouseDragMode::FORCE)
				obj->GetRigidBody()->applyCentralForce(vecDiff * m_drag_scale_force);
			else if(drag_mode == MouseDragMode::MOMENTUM)
				obj->GetRigidBody()->applyCentralImpulse(vecDiff * m_drag_scale_momentum);
		}
		else if(drag_mode == MouseDragMode::POSITION)
		{
			obj->SetPosition(pos_cur - pos_startcur + m_drag_pos_axis_start);
		}

#else	// !USE_BULLET
		// only position dragging possible
		obj->SetPosition(pos_cur - pos_startcur + m_drag_pos_axis_start);

#endif	// USE_BULLET

		obj_dragged = true;
	}

	if(obj_dragged)
		EmitUpdate();
}


/**
 * find the object with the given id
 */
std::shared_ptr<Geometry> Scene::FindObject(const std::string& objid)
{
	const Scene* _this = this;
	return std::const_pointer_cast<Geometry>(_this->FindObject(objid));
}


/**
 * find the object with the given id
 */
std::shared_ptr<const Geometry> Scene::FindObject(const std::string& objid) const
{
	if(auto iter = std::find_if(m_objs.begin(), m_objs.end(),
		[&objid](const std::shared_ptr<Geometry>& obj) -> bool
		{
			return obj->GetId() == objid;
		}); iter != m_objs.end())
	{
		return *iter;
	}

	return nullptr;
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
		timestamp << *optTime;

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
	if(std::shared_ptr<const Geometry> obj = FindObject(objid); obj)
	{
		return obj->GetProperties();
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
	if(const std::shared_ptr<Geometry> obj = FindObject(objid); obj)
	{
		obj->SetProperties(props);
		return std::make_tuple(true, obj);
	}

	return std::make_tuple(false, nullptr);
}
