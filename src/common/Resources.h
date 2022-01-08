/**
 * resource file handling
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @note Forked on 5-November-2021 from my privately developed "qm" project (https://github.com/t-weber/qm).
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TASPATHS_RESOURCES__
#define __TASPATHS_RESOURCES__

#include <vector>
#include <string>


class Resources
{
public:
	Resources() = default;
	~Resources() = default;

	void AddPath(const std::string& path);
	std::string FindFile(const std::string& file) const;


private:
	std::vector<std::string> m_paths{};
};


#endif
