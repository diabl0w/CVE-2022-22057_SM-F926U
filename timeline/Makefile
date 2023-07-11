ifndef ANDROID_NDK_HOME
$(error ANDROID_NDK_HOME is not set)
endif

OS=$(shell uname)
OS_ARCH_STR=

ifneq ($(findstring Linux,$(OS)),)
	OS_ARCH=$(shell uname -m)
	ifneq ($(findstring x86_64,$(OS_ARCH)),)
		OS_ARCH_STR=linux-x86_64
	else
		OS_ARCH_STR=linux-x86
	endif
else
	ifeq (Darwin,$(OS))
	OS_ARCH_STR=darwin-x86_64
	else
	$(error "Uname $(OS) not recognized.  I probably need to fix my makefile")
	endif
endif

TARGET_ARCH=aarch64
TOOLCHAIN=$(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/$(OS_ARCH_STR)
TARGET_ARCH_STR=$(TARGET_ARCH)-linux-android
API=30

export CC := $(TOOLCHAIN)/bin/$(TARGET_ARCH_STR)$(API)-clang
export CXX := $(TOOLCHAIN)/bin/$(TARGET_ARCH_STR)$(API)-clang++
export STRIP := $(TOOLCHAIN)/bin/llvm-strip
export AR := $(TOOLCHAIN)/bin/llvm-ar
export AS := $(TOOLCHAIN)/bin/llvm-as
LIBDIRS := $(TOOLCHAIN)
CFLAGS += -O0
CXXFLAGS += -O0

ASFLAGS		:=	-g
LDFLAGS		=	-g

ifeq ($(strip $(STATIC)),true)
    LDFLAGS += -static -static-libgcc
endif

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	timeline
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=
#STATIC		=	true

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project (order is important)
#---------------------------------------------------------------------------------
#LIBS		:=	-llog

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

# use CXX for linking C++ projects, CC for standard C
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET) $(TARGET).stripped

all:	$(BUILD)


#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT)	:	$(OFILES)
	@echo linking $(notdir $@)
	@$(LD) $(LDFLAGS) $(OFILES) --sysroot=$(TOOLCHAIN)/sysroot $(LIBPATHS) $(LIBS) -o $@
	@echo strip $(notdir $@) "->" $(notdir $@).stripped
	@$(STRIP) -s $@ -o $@.stripped


-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------
%.o: %.cpp
	@echo $(notdir $<)
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.o: %.c
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.o: %.s
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

#---------------------------------------------------------------------------------
%.o: %.S
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

%.bin.o : %.bin
	@echo $(notdir $<)
	@$(BIN2S) -a 32 $< | $(AS) -o $(@)

#---------------------------------------------------------------------------------------
endif
