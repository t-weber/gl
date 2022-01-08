/**
 * resource file handling
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @note Forked on 5-November-2021 from my privately developed "qm" project (https://github.com/t-weber/qm).
 * @license GPLv3, see 'LICENSE' file
 */

#include "Resources.h"
#include "tlibs2/libs/file.h"


/**
 * add a resource search path entry
 */
void Resources::AddPath(const std::string& pathname)
{
	m_paths.push_back(pathname);
}


/**
 * find a resource file
 */
std::string Resources::FindFile(const std::string& filename) const
{
	fs::path file{filename};

	for(const std::string& pathname : m_paths)
	{
		fs::path path{pathname};
		fs::path respath = path / file;

		if(fs::exists(respath))
			return respath.string();
	}

	return "";
}
