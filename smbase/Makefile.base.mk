# Makefile.base.mk
# this Makefile will be included by other Makefiles; it
#   provides a basic set of definitions
# a Makefile that includes this one should specify,
# *before* including this one:
#   - a default target, since I define a target here intended
#     to catch undefined PLATFORM
#   - libraries: any needed libraries
#   - includes: any needed includes
#   - ccflags: any other needed flags

# pull in platform-specifics
PLAT = ${HOME}/wrk/lib/plat
include ${PLAT}/Makefile.${PLATFORM}

# catch undefined PLATFORM (this appears to fail on older 'make'.. try gmake)
${PLAT}/Makefile.:
	@echo "--- You must setenv PLATFORM to one of the legal values: ---"
	@(cd ${PLAT}; ls Makefile.* | sed s/Makefile.// | grep -v template | grep -v '~')
	@echo "           (PLATFORM is currently \"${PLATFORM}\")"
	@echo "------------------------------------------------------------"
	exit 1

whatis-platform:
	@echo PLATFORM is \"${PLATFORM}\"


# --- add to previous definitions ---
defines += ${platform_defines}
includes += -I. ${platform_includes}
libraries += -L. ${platform_libraries}
ccflags += -g -Wall ${platform_ccflags}


# --- standard definitions ---
compile = g++ -c ${ccflags} ${defines} ${includes}
no-warnings = -w
link = g++ ${ccflags} ${defines} ${includes}
linkend = ${libraries}
makelib = ar -r
ranlib = ranlib


# --- conversion targets ---
# method to convert .cpp to .o (more reliable than ".cpp.o" method)
# (seems to be GNU-make-specific)
%.o : %.cpp
	${compile} $< -o $@

# and for .cc
%.o : %.cc
	${compile} $< -o $@
