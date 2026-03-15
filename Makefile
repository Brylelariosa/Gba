TARGET   := game
BUILD    := build
SOURCES  := src

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM

include $(DEVKITARM)/gba_rules
