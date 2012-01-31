#
# - Find lemon executable and provides macros to generate custom build rules
# The module defines the following variables
#
#  LEMON_EXECUTABLE - path to the lemon program
#  LEMON_FOUND - true if the program was found
#
# If lemon is found, the module defines the macro
#  LEMON_TARGET(<Name> <LemonInput> <LemonSource> <LemonHeader>
#		[<ArgString>])
# which will create a custom rule to generate a parser. <LemonInput> is
# the path to a lemon file. <LemonSource> is the desired name for the
# generated source file. <LemonHeader> is the desired name for the
# generated header which contains the token list. Anything in the optional
# <ArgString> parameter is appended to the lemon command line.
#
# The macro defines a set of variables:
# LEMON_${Name}_DEFINED       - True if the macro ran successfully
# LEMON_${Name}_INPUT         - The input source file, an alias for <LemonInput>
# LEMON_${Name}_OUTPUT_SOURCE - The source file generated by lemon, an alias for <LemonSource>
# LEMON_${Name}_OUTPUT_HEADER - The header file generated by lemon, an alias for <LemonHeader>
# LEMON_${Name}_OUTPUTS       - All bin outputs
# LEMON_${Name}_EXTRA_ARGS    - Arguments added to the lemon command line
#
#  ====================================================================
#  Example:
#
#   find_package(LEMON)
#   LEMON_TARGET(MyParser parser.y parser.c parser.h)
#   add_executable(Foo main.cpp ${LEMON_MyParser_OUTPUTS})
#  ====================================================================
#
#=============================================================================
#                 F I N D L E M O N . C M A K E
#
# Originally based off of FindBISON.cmake from Kitware's CMake distribution
#
# Copyright 2010 United States Government as represented by
#                the U.S. Army Research Laboratory.
# Copyright 2009 Kitware, Inc.
# Copyright 2006 Tristan Carel
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# 
# * The names of the authors may not be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

FIND_PROGRAM(LEMON_EXECUTABLE lemon DOC "path to the lemon executable")
MARK_AS_ADVANCED(LEMON_EXECUTABLE)

FIND_PROGRAM(MOVE_EXECUTABLE NAMES mv move DOC "path to the move executable")
MARK_AS_ADVANCED(MOVE_EXECUTABLE)

IF(LEMON_EXECUTABLE AND NOT LEMON_TEMPLATE)
    get_filename_component(lemon_path ${LEMON_EXECUTABLE} PATH)
    IF(lemon_path)
	SET(LEMON_TEMPLATE "")
	IF(EXISTS ${lemon_path}/lempar.c)
	    SET(LEMON_TEMPLATE "${lemon_path}/lempar.c")
	ENDIF(EXISTS ${lemon_path}/lempar.c)
	IF(EXISTS /usr/share/lemon/lempar.c)
	    SET(LEMON_TEMPLATE "/usr/share/lemon/lempar.c")
	ENDIF(EXISTS /usr/share/lemon/lempar.c)
    ENDIF(lemon_path)
ENDIF(LEMON_EXECUTABLE AND NOT LEMON_TEMPLATE)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LEMON DEFAULT_MSG LEMON_EXECUTABLE LEMON_TEMPLATE)
MARK_AS_ADVANCED(LEMON_TEMPLATE)

#============================================================
# FindLEMON.cmake ends here

# Local Variables:
# tab-width: 8
# mode: sh
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
