/**
 * resource files
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date Nov-2021
 * @license see 'LICENSE' file
 */

#ifndef __GL_RESOURCES_H__
#define __GL_RESOURCES_H__

#include <vector>
#include <optional>

#if __has_include(<filesystem>)
	#include <filesystem>
	namespace filesystem = std::filesystem;
#else
	#include <boost/filesystem.hpp>
	namespace filesystem = boost::filesystem;
#endif


class Resources
{
public:
	Resources() = default;
	~Resources() = default;

	void AddPath(const filesystem::path& path);
	std::optional<filesystem::path> FindFile(const filesystem::path& file) const;

	void SetBinPath(const filesystem::path& path) { m_bin_path = path; }
	const filesystem::path& GetBinPath() const { return m_bin_path; }


private:
	// resource paths
	std::vector<filesystem::path> m_paths{};

	// program binary path
	filesystem::path m_bin_path{};
};

#endif
