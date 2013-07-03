#
# Copyright (C) 2011-2013 Alexey Galakhov <agalakhov@gmail.com>
#
# Licensed under the GNU General Public License Version 3
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

ROOTDIR = $(dir $(realpath $(filter build.mak %/build.mak , $(MAKEFILE_LIST)) ) )
include $(ROOTDIR)make.cfg
include $(ROOTDIR)packages.cfg

.PHONY: all clean
all:

mypath := $(subst $(ROOTDIR),,$(dir $(realpath Makefile)))
psubs := $(addprefix $(mypath),$(addsuffix /Makefile.d,$(SUBDIRS)))

clean: $(addprefix clean-,$(SUBDIRS))
.PHONY: $($(addprefix clean-,$(SUBDIRS))
$(addprefix clean-,$(SUBDIRS)) : clean-% :
	$(MAKE) -C $* clean

ifeq ('$(mypath)','') ## GLOBAL MODE ###############################

binaries :=
includes :=
libs :=

pkgvarname_INCLUDE = PKG_CFLAGS
pkgvarname_LIBS = PKG_LIBS

recurse_dependencies = $($(1)_$(3)) $(foreach k,\
    $(filter-out $(2),$($(1)_$(3))),\
    $(call recurse_dependencies,$(1),$(2) $(3),$(k))\
)

find_dependencies = $(2) $(call recurse_dependencies,my_$(1),,$(2))

get_my_vars = $(foreach k,$(2),$($(1)_$(k)))

refine_options = $(strip $(sort $(filter-out -l%,$(1))) $(filter -l%,$(1)) )

make_pkgconfig = $(call get_my_vars,$(pkgvarname_$(1)),\
    $(sort $(call get_my_vars,my_USE,$(2)))\
)

get_locals = $(call get_my_vars,local_$(1),$(2)) $(call make_pkgconfig,$(1),$(2))

make_includes = $(call refine_options,\
    -I. \
    $(call get_locals,INCLUDE,\
        $(call find_dependencies,INCLUDE,$(1))\
    ) \
)

make_libs = $(call refine_options,\
    $(call get_locals,LIBS,\
        $(call find_dependencies,LIBS,$(1))\
    ) \
)

all_projects :=
all_use :=
ifneq ($(MAKECMDGOALS),clean)
    include $(psubs)
endif

$(foreach k,$(all_projects),\
    $(eval $(prjname_$(k)):$(call get_my_vars,prjname,$(my_LIBS_$(k)))) \
)

define pkg_config_maker
    ifeq ($$(PKG_CFLAGS_$(1))$$(PKG_LIBS_$(1)),)
        ifneq ($$(PKG_CFLAGS_CMD_$(1)),)
            PKG_CFLAGS_$(1) = $$(shell $$(PKG_CFLAGS_CMD_$(1)))
        else
            PKG_CFLAGS_$(1) = $$(shell $$(PKG_CONFIG) --cflags --silence-errors $(1))
        endif
        ifneq ($$(PKG_LIBS_CMD_$(1)),)
            PKG_LIBS_$(1) = $$(shell $$(PKG_LIBS_CMD_$(1)))
        else
            PKG_LIBS_$(1) = $$(shell $$(PKG_CONFIG) --libs --silence-errors $(1))
        endif
        ifeq ($$(PKG_CFLAGS_$(1))$$(PKG_LIBS_$(1)),)
            $$(warning Package $(1) not found)
        endif
    endif
endef

PKG_CONFIG ?= pkg-config
$(foreach k, $(sort $(all_use)),\
    $(eval $(call pkg_config_maker,$(k))) \
)

%/Makefile.d: %/Makefile $(ROOTDIR)build.mak $(ROOTDIR)make.cfg
	@$(MAKE) -C $* Makefile.d

%.o : %.c
	@echo gcc $<
	@$(CC) $(CFLAGS) $(includes) -c -o $@ $<

%.o : %.cc
	@echo g++ $<
	@$(CXX) $(CXXFLAGS) $(includes) -c -o $@ $<

%.o : %.cpp
	@echo g++ $<
	@$(CXX) $(CXXFLAGS) $(includes) -c -o $@ $<

%.d : %.c
	@echo deps $<
	@$(CC) $(CFLAGS) $(includes) -M -MP -MQ $@ -MQ $(<:%.c=%.o) -o $@ $<

%.d : %.cc
	@echo deps $<
	@$(CXX) $(CXXFLAGS) $(includes) -M -MP -MQ $@ -MQ $(<:%.cc=%.o) -o $@ $<

%.d : %.cpp
	@echo deps $<
	@$(CXX) $(CXXFLAGS) $(includes) -M -MP -MQ $@ -MQ $(<:%.cpp=%.o) -o $@ $<

%$(LIB) :
	@echo ar $@
	@$(AR) r $@ $(filter %.o,$^)

%$(DLL) :
	@echo link-dl $@
	@$(CXX) $(LDFLAGS) -shared -o $@ $(filter %.o,$^) $(libs)

$(binaries) : % :
	@echo link $*
	@$(CXX) $(LDFLAGS) -o $@ $(filter %.o,$^) $(libs)

else ## LOCAL MODE ###############################

objs := $(addsuffix .o,$(basename $(SRCS)))
deps := $(objs:%.o=%.d)
pobjs := $(addprefix $(mypath),$(objs))
pdeps := $(addprefix $(mypath),$(deps))

INCLUDE += $(LIBS)

mydir = $(mypath:%/=%)
ifdef PROGRAM
    myname := $(PROGRAM)$(EXE)
    binary := $(PROGRAM)
endif
ifdef PACKAGE
    myname := lib$(PACKAGE)$(LIB)
    library := $(PACKAGE)
endif
ifdef LIBRARY
    myname := $(LIBRARY)$(DLL)
    library := $(LIBRARY)
    binary := $(LIBRARY)
endif
project := $(mypath)$(myname)

all:
	@$(MAKE) -C $(ROOTDIR) $(project)

$(myname) $(objs) $(deps): %:
	@$(MAKE) -C $(ROOTDIR) $(mypath)$@

clean:
	@echo clean
	@$(RM) $(myname) $(objs) $(deps) Makefile.d

Makefile.d: Makefile $(ROOTDIR)build.mak $(ROOTDIR)make.cfg
	@echo preparing $(mypath)
	@$(RM) $@
ifdef psubs
	@echo 'include $(psubs)' >> $@
endif
ifdef PROGRAM
	@echo 'binaries += $(project)' >> $@
endif
ifdef binary
	@echo 'all: $(project)' >> $@
	@echo '$(project) : libs = $$(call make_libs,$(mydir))' >> $@
endif
ifdef PACKAGE
	@echo 'local_INCLUDE_$(mydir) = -I$(mydir)' >> $@
	@echo 'local_LIBS_$(mydir) = -L$(mydir) -l$(PACKAGE)' >> $@
else
	@echo 'local_INCLUDE_$(mydir) =' >> $@
	@echo 'local_LIBS_$(mydir) =' >> $@
endif
ifdef USE
	@echo 'my_USE_$(mydir) = $(USE)' >> $@
	@echo 'all_use += $(USE)' >> $@
endif
	@echo 'my_LIBS_$(mydir) = $(LIBS)' >> $@
	@echo 'my_INCLUDE_$(mydir) = $(INCLUDE)' >> $@
ifdef myname
	@echo '$(project): $(pobjs)' >> $@
	@echo 'all_projects += $(mydir)' >> $@
	@echo 'prjname_$(mydir) = $(project)' >> $@
endif
ifdef pobjs
	@echo '$(pdeps) $(pobjs) : includes = $$(call make_includes,$(mydir))' >> $@
	@echo '-include $(pdeps)' >> $@
endif
ifdef subs
	@echo 'include $(psubs)' >> $@
endif

endif
