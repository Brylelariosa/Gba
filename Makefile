#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/gba_rules

#---------------------------------------------------------------------------------
TARGET    := CellAutoLab
BUILD     := build
SOURCES   := src
INCLUDES  := src

#---------------------------------------------------------------------------------
ARCH    := -mthumb -mthumb-interwork

CFLAGS  := -Wall -Wextra -O2 \
            -mcpu=arm7tdmi -mtune=arm7tdmi \
            -fomit-frame-pointer -fno-strict-aliasing \
            -std=gnu11 \
            $(ARCH)

CFLAGS  += $(INCLUDE)

ASFLAGS := $(ARCH)
LDFLAGS  = $(ARCH) -Wl,-Map,$(BUILD)/$(TARGET).map

LIBS    := -lgba
LIBDIRS := $(DEVKITPRO)/libgba

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT  := $(CURDIR)/$(TARGET)
export VPATH   := $(CURDIR)/$(SOURCES)
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(notdir $(wildcard $(CURDIR)/$(SOURCES)/*.c))
SFILES   := $(notdir $(wildcard $(CURDIR)/$(SOURCES)/*.s))

export LD       := $(CC)
export OFILES   := $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE  := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean rebuild debug release

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

debug: CFLAGS += -DDEBUG -g
debug: $(BUILD)

release: CFLAGS += -DNDEBUG
release: $(BUILD)

clean:
	@echo Cleaning...
	@rm -fr $(BUILD) $(TARGET).gba $(TARGET).elf

rebuild: clean $(BUILD)

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).gba : $(OUTPUT).elf
$(OUTPUT).elf : $(OFILES)

-include $(DEPENDS)

endif