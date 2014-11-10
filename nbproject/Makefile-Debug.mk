#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES=


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-pipe -pthread -D_REENTRANT -D_THREAD_SAFE -O2 -march=native -mtune=native -fomit-frame-pointer -mfpmath=sse -ftree-vectorize -funroll-loops -mno-tls-direct-seg-refs -DBZ_NO_STDIO -DLOG_DEBUG -DMAX_BUFF_SIZE=256 -DTSAFE_LOG=1 -std=c++0x
CXXFLAGS=-pipe -pthread -D_REENTRANT -D_THREAD_SAFE -O2 -march=native -mtune=native -fomit-frame-pointer -mfpmath=sse -ftree-vectorize -funroll-loops -mno-tls-direct-seg-refs -DBZ_NO_STDIO -DLOG_DEBUG -DMAX_BUFF_SIZE=256 -DTSAFE_LOG=1 -std=c++0x

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libitcframework.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libitcframework.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libitcframework.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libitcframework.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libitcframework.a

${OBJECTDIR}/include/ThreadPool.h.gch: include/ThreadPool.h 
	${MKDIR} -p ${OBJECTDIR}/include
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -Iinclude -I../ITCLib/include -I../utils/include -MMD -MP -MF "$@.d" -o "$@" include/ThreadPool.h

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libitcframework.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
