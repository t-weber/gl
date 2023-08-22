/**
 * recent files
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date Oct-2021
 * @license see 'LICENSE' file
 */

#ifndef __GL_RECENT_H__
#define __GL_RECENT_H__

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QDir>

#include <vector>
#include <algorithm>
#include <memory>

#if __has_include(<filesystem>)
	#include <filesystem>
	namespace fs = std::filesystem;
#else
	#include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#endif


/**
 * maintains a list of recently opened files
 */
class RecentFiles
{
public:
	RecentFiles(QWidget* parent = nullptr, std::size_t cap = 16)
		: m_parentWidget{parent}, m_recent_file_capacity{cap}
	{}

	~RecentFiles() = default;

	// copy constructor
	RecentFiles(const RecentFiles& other) = default;
	RecentFiles& operator=(const RecentFiles& other) = default;


	void SetRecentMenu(std::shared_ptr<QMenu> recent) { m_menuRecent = recent; }
	std::shared_ptr<QMenu> GetRecentMenu() { return m_menuRecent; }

	const QStringList& GetRecentFiles() const { return m_recentFiles; }
	void SetRecentFiles(const QStringList& recent) { m_recentFiles = recent; }

	const QString& GetRecentDir() const { return m_recentDir; }
	void SetRecentDir(const QString& recent) { m_recentDir = recent; }

	const QString& GetOpenFile() const { return m_openFile; }
	void SetOpenFile(const QString& file) { m_openFile = file; }

	void AddForbiddenDir(const QString& dir) { m_forbidden_dirs.emplace_back(dir.toStdString()); }


	/**
	 * create a menu with the recent files
	 */
	template<class t_loadfunc>
	void CreateRecentFileMenu(t_loadfunc loadfunc)
	{
		m_menuRecent->clear();

		for(auto iter = m_recentFiles.begin(); iter != m_recentFiles.end();)
		{
			const QString& filename = *iter;

			fs::path file{filename.toStdString()};
			if(!fs::exists(file) || IsInForbiddenDir(file))
			{
				iter = m_recentFiles.erase(iter);
				continue;
			}

			QAction *actionFile = new QAction{file.filename().string().c_str(), m_parentWidget};
			actionFile->setToolTip(filename);

			QObject::connect(actionFile, &QAction::triggered, [this, loadfunc, filename]()
			{
				if(loadfunc(filename))
					m_openFile = filename;
			});

			m_menuRecent->addAction(actionFile);

			++iter;
		}
	}


	/**
	 * add a recent file to the list
	 */
	template<class t_loadfunc>
	void AddRecentFile(const QString& filename, t_loadfunc loadfunc)
	{
		m_recentFiles.push_front(filename);
		m_recentFiles.removeDuplicates();

		if((std::size_t)m_recentFiles.size() > m_recent_file_capacity)
			m_recentFiles.erase(m_recentFiles.begin()+m_recent_file_capacity, m_recentFiles.end());

		CreateRecentFileMenu<t_loadfunc>(loadfunc);
	}


protected:
	/**
	 * is the file under the list of forbidden directories
	 */
	bool IsInForbiddenDir(const fs::path& file) const
	{
		// count number of elements in a path (or, generally, in an iterator range)
		auto num_elems = [](auto begin, auto end) -> std::size_t
		{
			auto iter = begin;
			std::size_t cnt = 0;

			while(iter++ != end)
				++cnt;

			return cnt;
		};


		fs::path file_abs = fs::absolute(file);
		std::size_t file_elems = num_elems(file_abs.begin(), file_abs.end());

		for(const fs::path& dir : m_forbidden_dirs)
		{
			fs::path dir_abs = fs::absolute(dir);
			std::size_t dir_elems = num_elems(dir_abs.begin(), dir_abs.end());
			//std::cout << "dir: " << dir_abs.string() << " (elems: " << dir_elems << ")"
			//	<< ", file: " << file_abs.string() << " (elems: " << file_elems << ")"
			//	<< std::endl;

			// file path is shorter than directory?
			if(file_elems < dir_elems)
				continue;

			if(std::equal(dir_abs.begin(), dir_abs.end(), file_abs.begin()))
				return true;
		}

		return false;
	}


private:
	// recent directory
	QString m_recentDir{QDir::homePath()};

	// recent files
	QStringList m_recentFiles{};
	std::shared_ptr<QMenu> m_menuRecent{};

	// currently open file
	QString m_openFile{};

	// parent widget
	QWidget* m_parentWidget{nullptr};

	// maximum number of recent files in the list
	std::size_t m_recent_file_capacity{16};

	// list of directories for which recent files shouldn't be saved
	std::vector<fs::path> m_forbidden_dirs{};
};


#endif
