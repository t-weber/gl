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
TLIBS2=https://code.ill.fr/scientific-software/takin/tlibs2/-/archive/master/tlibs2-master.zip
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# cleans externals
# -----------------------------------------------------------------------------
function clean_dirs()
{
	# remove old versions, but not if they're links

	if [ ! -L tlibs2 ]; then
		rm -rfv tlibs2
	fi

	if [ ! -L externals ]; then
		rm -rfv externals
	fi
}

function clean_files()
{
	local TLIBS2_LOCAL=$(get_filename_from_url ${TLIBS2})
	rm -fv ${TLIBS2_LOCAL}
}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# sets up libraries
# -----------------------------------------------------------------------------
function setup_tlibs2()
{
	local TLIBS2_LOCAL=$(get_filename_from_url ${TLIBS2})

	if [ -L tlibs2 ]; then
		echo -e "A link to tlibs2 already exists, skipping action."
		return
	fi

	if ! ${WGET} ${TLIBS2}
	then
		echo -e "Error downloading tlibs2.";
		exit -1;
	fi

	if ! ${UNZIP} ${TLIBS2_LOCAL}
	then
		echo -e "Error extracting tlibs2.";
		exit -1;
	fi

	mv -v tlibs2-master tlibs2
}
# -----------------------------------------------------------------------------


echo -e "--------------------------------------------------------------------------------"
echo -e "Removing old files and directories...\n"
clean_dirs
clean_files
mkdir externals
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Downloading and setting up tlibs2...\n"
setup_tlibs2
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Removing temporary files...\n"
clean_files
echo -e "--------------------------------------------------------------------------------\n"
