/**
 * recent files
 * @author Tobias Weber <tweber@ill.fr>
 * @date nov-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __RECENT_FILES_H__
#define __RECENT_FILES_H__

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>

#include <functional>


// ----------------------------------------------------------------------------
class RecentFiles
{
public:
	RecentFiles() = default;
	~RecentFiles() = default;

	RecentFiles(const RecentFiles& other) = default;
	RecentFiles& operator=(const RecentFiles& other) = default;


	// adds a file to the recent files menu
	void AddRecentFile(const QString &file);

	// sets the recent file menu
	void SetRecentFiles(const QStringList &files);
	const QStringList& GetRecentFiles() const { return m_recentFiles; }

	// creates the "recent files" sub-menu
	void RebuildRecentFiles();

	// remove superfluous entries
	void TrimEntries();

	// set the function to be called when the menu element is clicked
	void SetOpenFunc(const std::function<bool(const QString& filename)>* func)
	{ m_open_func = func; }

	void SetRecentFilesMenu(QMenu *menu) { m_menuOpenRecent = menu; };
	QMenu* GetRecentFilesMenu() { return m_menuOpenRecent; }

	void SetCurFile(const QString& file) { m_curFile = file; }
	const QString& GetCurFile() const { return m_curFile; }

	void AddForbiddenDir(const QString& dir) { m_forbiddenDirs << dir; }
	bool IsFileInForbiddenDir(const QString& file) const;

	void SetMaxRecentFiles(std::size_t num) { m_maxRecentFiles = num; }


private:
	// maximum number of recent files
	std::size_t m_maxRecentFiles{ 16 };

	// recent file menu
	QMenu* m_menuOpenRecent{ nullptr };

	// recent file list
	QStringList m_recentFiles{};

	// currently active file
	QString m_curFile{};

	// list of directories for which recent files shouldn't be saved
	QStringList m_forbiddenDirs{};

	// function to be called when the menu element is clicked
	const std::function<bool(const QString& filename)>* m_open_func{ nullptr };
};
// ----------------------------------------------------------------------------


#endif
