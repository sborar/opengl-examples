#!/usr/bin/env bash

function cleandir()
{
	echo
	echo "=== Cleaning ${1}"
	# If makefile still exists, try "make clean"
	if [[ -e Makefile ]]; then
		make --quiet -C "${1}" clean
	fi
	rm -vrf "${1}/CMakeFiles"
	rm -vf  "${1}/CMakeCache.txt"
	rm -vf  "${1}/Makefile"
	rm -vf  "${1}/cmake_install.cmake"

	# Text editor backup files:
	rm -vf *~ \#*\#
}

# Get the directory that this script is in (might not be the same as
# the current working directory).
THIS_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# Clean top level directory first so 'make clean' will work.
cleandir ${THIS_DIR}

# Clean subdirectories
for D in *; do # For each file and directory
    if [ -d "${D}" ]; then # If it is a directory
		if [[ -e "${D}/CMakeLists.txt" ]]; then # if CMakeLists file is present
			cleandir "${THIS_DIR}/${D}"
		fi
    fi
done

echo
echo "=== Cleaning other files"
rm -vrf "${THIS_DIR}/doxygen-docs"
rm -vf "${THIS_DIR}/bin/"*.frag "${THIS_DIR}/bin/"*.vert
# Linux:
rm -vf "${THIS_DIR}/bin/"*libOVR*.so*
# Mac:
rm -vrf "${THIS_DIR}/bin/"*.dSYM

# Find any log files anywhere in this tree and delete them.
find ${THIS_DIR} -type f \( -name 'log.txt' -o -name 'log-ivs-left.txt' -o -name 'log-ivs-right.txt' \) -exec rm -vf "{}" \;

if [[ -x "${THIS_DIR}/.git" && -x /usr/bin/git ]]; then
	echo
	echo
	echo "Consider removing the following files because they are not in the git repo and are not ignored:"
	echo
	pushd "${THIS_DIR}" > /dev/null
	git ls-files --others --exclude-standard
	popd > /dev/null
fi
