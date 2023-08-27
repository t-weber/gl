#!/bin/bash
#
# downloads external libraries
# @author Tobias Weber <tweber@ill.fr>
# @date 6-january-2022
# @license GPLv3, see 'LICENSE' file
#


# -----------------------------------------------------------------------------
# tools
# -----------------------------------------------------------------------------
WGET=wget
UNZIP=unzip

TAR=$(which gtar)
if [ $? -ne 0 ]; then
	TAR=$(which tar)
fi

SED=$(which gsed)
if [ $? -ne 0 ]; then
	SED=$(which sed)
fi
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# helper functions
# -----------------------------------------------------------------------------
get_filename_from_url() {
	local url="$1"

	local filename=${url##*[/\\]}
	echo "${filename}"
}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# URLs for external libraries
# -----------------------------------------------------------------------------
MATHLIBS=https://codeload.github.com/t-weber/mathlibs/zip/refs/heads/main
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# cleans externals
# -----------------------------------------------------------------------------
function clean_dirs()
{
	# remove old versions, but not if they're links

	if [ ! -L mathlibs ]; then
		rm -rfv mathlibs
	fi
}

function clean_files()
{
	local MATHLIBS_LOCAL=$(get_filename_from_url ${MATHLIBS})
	rm -fv ${MATHLIBS_LOCAL}
}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# sets up libraries
# -----------------------------------------------------------------------------
function setup_mathlibs()
{
	local MATHLIBS_LOCAL=$(get_filename_from_url ${MATHLIBS})

	if [ -L mathlibs ]; then
		echo -e "A link to mathlibs already exists, skipping action."
		return
	fi

	if ! ${WGET} ${MATHLIBS}
	then
		echo -e "Error downloading mathlibs.";
		exit -1;
	fi

	if ! ${UNZIP} ${MATHLIBS_LOCAL}
	then
		echo -e "Error extracting mathlibs.";
		exit -1;
	fi

	mv -v mathlibs-main mathlibs
}

# -----------------------------------------------------------------------------


echo -e "--------------------------------------------------------------------------------"
echo -e "Removing old files and directories...\n"
clean_dirs
clean_files
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Downloading and setting up mathlibs...\n"
setup_mathlibs
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Removing temporary files...\n"
clean_files
echo -e "--------------------------------------------------------------------------------\n"
