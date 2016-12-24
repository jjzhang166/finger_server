
## Add appropriate suffixes and extensions


ifneq ($(ARC_TARGET),)
  ARC_TARGET := lib$(ARC_TARGET)$(LIB_SUFFIX).a
endif

ifneq ($(SO_TARGET),)
  SO_TARGET := lib$(SO_TARGET)$(LIB_SUFFIX).so
endif

## Special variables to help with clean targets

DIRSC := $(foreach dir,$(DIRS),$(dir)(clean))
ASMS := $(foreach obj,$(OBJS),$(basename $(obj)).s)

## Put the extension on all objs

OBJS := $(foreach obj,$(OBJS),$(obj).o)


## Turn on debug flag and define DEBUG symbol for debug builds

ifeq ($(DEBUG),1)
  CFLAGS += -g
  ifeq ($(LINUX_COMPILER),_EQUATOR_)
    CFLAGS += -O2
  else
    CFLAGS += -O0
  endif
  CFLAGS += -DDEBUG=$(DEBUG)
endif

ifeq ($(DEBUG),0)
  CFLAGS += -O2
  CFLAGS += -DNDEBUG
endif

ifneq ($(SO_TARGET),)
  CFLAGS += -fpic
endif

ifeq ($(LINUX_COMPILER),_EQUATOR_)
   CFLAGS += -D_EQUATOR_
endif

ifeq ($(PWLIB_SUPPORT),1)
   CFLAGS += -DPWLIB_SUPPORT -DPTRACING=0 -D_REENTRANT -DPHAS_TEMPLATES -DPMEMORY_CHECK=0 -DPASN_LEANANDMEAN -pipe -fPIC
endif

CFLAGS += -D_LINUX_ 

## Add include path and constant definitions to
## compile options

CFLAGS += $(foreach dir,$(INC_PATH),-I$(dir))


## Add library path and libraries to link options
LDFLAGS += $(foreach lib,$(LIB_PATH),-L$(lib))

ifeq ($(LINUX_COMPILER),_HHPPC_)
  LDFLAGS += --static
endif

ifneq ($(SO_TARGET),)
  LDFLAGS += -shared
endif

## When using a shared object library and not building
## the shared object library itself, don't link with the
## libraries. Don't know how to do "or" in make, so use
## an intermediate variable.
LDFLAGS += $(foreach lib,$(LIBS),-l$(lib)$(LIB_SUFFIX))

## Set up library install location
ifndef INSTALL_LIB_PATH
  ifeq ($(INSTALL_LIB_LOC),os)
    INSTALL_LIB_PATH = $(ETI_TOOLKIT_INSTALL)/$(RTOS_DIR)/$(MAP_ARCH)_lib
  else
    INSTALL_LIB_PATH = $(ETI_TOOLKIT_INSTALL)/common/$(MAP_ARCH)_lib
  endif
endif


## Set up application install location
ifndef INSTALL_APP_PATH
  APP_DIR ?= unknown
  INSTALL_APP_PATH = $(ETI_TOOLKIT_INSTALL)/app/$(APP_DIR)
endif


CC      = $(CROSS)g++
CPP     = $(CROSS)g++
LD      = $(CROSS)g++
AR      = $(CROSS)ar
INSTALL = install -D -m 644
OBJDUMP = objdump
RM      = -@rm -f


##------------------------------------------------------------------------
## Rules

## Suffix rules

$(SRC_DIR)/%.o: $(SRC_DIR)/%.s
	$(CC) -c -o $@ $(CFLAGS) $<
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $(CFLAGS) $<


## Rules for making archives

ifneq ($(strip $(ARC_TARGET)),)
  
  ifneq ($(LINUX_COMPILER),_HHPPC_)
      CFLAGS += -DFD_SETSIZE=512
  endif
  
  all: install
  
  install: install_inc install_arc
  
  install_arc: $(ARC_TARGET)
	$(INSTALL) $(ARC_TARGET) $(INSTALL_LIB_PATH)/$(ARC_TARGET)
  
  $(ARC_TARGET) : $(OBJS)
	$(AR) crus $(ARC_TARGET) $(OBJS)

  uninstall: uninstallarc
  
  uninstallarc:
	$(foreach file, $(INSTALL_INC), $(RM) $(INSTALL_INC_PATH)/$(file) ;)
	$(RM) $(INSTALL_LIB_PATH)/$(ARC_TARGET)

  clean: cleanarc
  
  cleanarc:
	$(RM) $(ARC_TARGET)

endif


## Rules for making shared object

ifneq ($(strip $(SO_TARGET)),)
  
  all: install
  
  install: install_inc install_so
  
  install_so: $(SO_TARGET)
	$(INSTALL) $(SO_TARGET) $(INSTALL_LIB_PATH)/$(SO_TARGET)
  
  $(SO_TARGET) : $(OBJS)
	$(LD) $(OBJS) -o $(SO_TARGET) $(LDFLAGS)

  uninstall: uninstallso
  
  uninstallso:
	$(foreach file, $(INSTALL_INC), $(RM) $(INSTALL_INC_PATH)/$(file) ;)
	$(RM) $(INSTALL_LIB_PATH)/$(ARC_TARGET)

  clean: cleanso
  
  cleanso:
	$(RM) $(SO_TARGET)

endif


## Rules for making applications

ifneq ($(strip $(APP_TARGET)),)

  all:install
  
  install: install_inc install_app
  
  install_app: $(APP_TARGET)
	$(INSTALL) $(APP_TARGET) $(INSTALL_APP_PATH)/$(APP_TARGET) 
	
  
  $(APP_TARGET): $(OBJS)
	$(LD) $(OBJS) -o $(APP_TARGET) $(LDFLAGS)
#	$(OBJDUMP) --syms $(APP_TARGET) | sort | grep " g" > $(APP_TARGET).map

  clean: cleanapp
  
  cleanapp:
	$(RM) $(APP_TARGET)

endif


## Rules for making subdirectories

ifneq ($(strip $(DIRS)),)

  all: $(DIRS)
  $(DIRS): FORCE
	$(MAKE) -C $@
  $(DIRSC): FORCE
	$(MAKE) -C $@ clean
  FORCE:
  clean: $(DIRSC)

endif


## Shared rules

install: install_inc

install_inc:
	$(foreach file, $(INSTALL_INC), $(INSTALL) $(file) $(INSTALL_INC_PATH)/$(notdir $(file)) ;)

clean: cleanobjs

cleanobjs:
	$(RM) $(ASMS) $(OBJS) *.pdb *.map


## Rule to pre-install all headers

setup:
	(cd $(TOP);    \
	make install_inc;   \
	echo )
