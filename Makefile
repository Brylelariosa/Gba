#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/gba_rules

#---------------------------------------------------------------------------------
TARGET   := game
BUILD    := build
SOURCES  := src          # <-- was "source", your repo uses src
INCLUDES := include
DATA     :=

#---------------------------------------------------------------------------------
ARCH := -mthumb -mthumb-interwork

CFLAGS := -g -Wall -O2 \
           -mcpu=arm7tdmi -mtune=arm7tdmi \
           $(ARCH)

CFLAGS   += $(INCLUDE)
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS    := -lgba
LIBDIRS := $(LIBGBA)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export OUTPUT  := $(CURDIR)/$(TARGET)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                  $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD     := $(CC)
export OFILES := $(BINFILES:%=%.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).gba

else
#---------------------------------------------------------------------------------
$(OUTPUT).gba : $(OUTPUT).elf
$(OUTPUT).elf : $(OFILES)
#---------------------------------------------------------------------------------
endif
