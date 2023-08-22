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
#TLIBS2=https://code.ill.fr/scientific-software/takin/tlibs2/-/archive/master/tlibs2-master.zip
TLIBS2=https://codeload.github.com/tweber-ill/ill_mirror-takin2-tlibs2/zip/refs/heads/master

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

	if [ ! -L tlibs2 ]; then
		rm -rfv tlibs2
	fi
}

function clean_files()
{
	local TLIBS2_LOCAL=$(get_filename_from_url ${TLIBS2})
	rm -fv ${TLIBS2_LOCAL}

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

	#mv -v tlibs2-master tlibs2
	mv -v ill_mirror-takin2-tlibs2-master tlibs2
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
echo -e "Downloading and setting up tlibs2...\n"
setup_tlibs2
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Removing temporary files...\n"
clean_files
echo -e "--------------------------------------------------------------------------------\n"
