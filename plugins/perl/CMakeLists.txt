#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net
#
#	Verlihub is free software; You can redistribute it
#	and modify it under the terms of the GNU General
#	Public License as published by the Free Software
#	Foundation, either version 3 of the license, or at
#	your option any later version.
#
#	Verlihub is distributed in the hope that it will be
#	useful, but without any warranty, without even the
#	implied warranty of merchantability or fitness for
#	a particular purpose. See the GNU General Public
#	License for more details.
#
#	Please see http://www.gnu.org/licenses/ for a copy
#	of the GNU General Public License.

SET(PERLSCRIPT_VERSION_MAJOR 1)
SET(PERLSCRIPT_VERSION_MINOR 1)
SET(PERLSCRIPT_VERSION_PATCH 1)
SET(PERLSCRIPT_VERSION "${PERLSCRIPT_VERSION_MAJOR}.${PERLSCRIPT_VERSION_MINOR}.${PERLSCRIPT_VERSION_PATCH}")

ADD_SUBDIRECTORY(vh)

Find_Package(PerlLibs)
Find_Package(Perl)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_DEFINITIONS(-g -DPERLSCRIPT_VERSION="${PERLSCRIPT_VERSION}" `perl -MExtUtils::Embed -e ccopts`)

SET(PERLSCRIPT_HDRS
	callbacks.h
	cconsole.h
	cperlinterpreter.h
	cperlmulti.h
	cperlscript.h
	cpiperl.h
)

SET(PERLSCRIPT_SRCS
	callbacks.cpp
	cconsole.cpp
	cperlinterpreter.cpp
	cperlmulti.cpp
	cperlscript.cpp
	cpiperl.cpp
)

ADD_LIBRARY(libperl_pi MODULE ${PERLSCRIPT_SRCS})
SET_TARGET_PROPERTIES(libperl_pi PROPERTIES OUTPUT_NAME "perl_pi" INSTALL_RPATH_USE_LINK_PATH TRUE)
target_link_libraries(libperl_pi ${PERL_LIBRARY})

INSTALL(TARGETS libperl_pi LIBRARY DESTINATION ${PLUGINDIR})
