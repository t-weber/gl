/**
 * scene
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @note some code forked from my private "misc" project: https://github.com/t-weber/misc
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __SCENE_SPACE_H__
#define __SCENE_SPACE_H__

#include <memory>
#include <vector>
#include <chrono>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/signals2/signal.hpp>

#include "types.h"
#include "Geometry.h"
#include "Scene.h"


// ----------------------------------------------------------------------------
// scene
// ----------------------------------------------------------------------------
class Scene
{
public:
	// constructor and destructor
	Scene();
	~Scene();

	// copy constructor and operator
	Scene(const Scene& scene);
	const Scene& operator=(const Scene& scene);

	void Clear();
	bool Load(const boost::property_tree::ptree& prop);
	boost::property_tree::ptree Save() const;

	void AddObject(const std::vector<std::shared_ptr<Geometry>>& obj, const std::string& id);
	bool DeleteObject(const std::string& id);
	bool RenameObject(const std::string& oldid, const std::string& newid);
	std::tuple<bool, std::shared_ptr<Geometry>> RotateObject(
		const std::string& id, t_real angle, char axis='x');

	std::shared_ptr<Geometry> FindObject(const std::string& id);
	std::shared_ptr<const Geometry> FindObject(const std::string& id) const;
	const std::vector<std::shared_ptr<Geometry>>& GetObjects() const { return m_objs; }

	void DragObject(bool drag_start, const std::string& obj,
		const t_vec& start, const t_vec& pos);

	// connection to update signal
	template<class t_slot>
	void AddUpdateSlot(const t_slot& slot)
		{ m_sigUpdate->connect(slot); }

	void EmitUpdate() { (*m_sigUpdate)(*this); }

	std::vector<ObjectProperty> GetProperties(const std::string& obj) const;
	std::tuple<bool, std::shared_ptr<Geometry>> SetProperties(
		const std::string& obj, const std::vector<ObjectProperty>& props);

	t_real GetEpsilon() const { return m_eps; }
	void SetEpsilon(t_real eps) { m_eps = eps; }

	void tick(const std::chrono::milliseconds& ms);

	static std::pair<bool, std::string> load(
		/*const*/ boost::property_tree::ptree& prop,
		Scene& scene,
		const std::string* filename = nullptr);

	static std::pair<bool, std::string> load(
		const std::string& filename,
		Scene& scene);


private:
	// objects
	std::vector<std::shared_ptr<Geometry>> m_objs{};

	// starting position for drag operation
	t_vec m_drag_pos_axis_start{};

	// update signal
	using t_sig_update = boost::signals2::signal<void(const Scene&)>;
	std::shared_ptr<t_sig_update> m_sigUpdate{};

	t_real m_eps = 1e-6;

#ifdef USE_BULLET
	std::shared_ptr<btDefaultCollisionConfiguration> m_coll{};
	std::shared_ptr<btCollisionDispatcher/*Mt*/> m_disp{};
	std::shared_ptr<btDbvtBroadphase> m_cache{};
	std::shared_ptr<btSequentialImpulseConstraintSolver> m_solver{};
	std::shared_ptr<btDynamicsWorld> m_world{};
#endif
};
// ----------------------------------------------------------------------------


#endif
