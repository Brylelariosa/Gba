# GBA Makefile using devkitARM + libgba

TARGET   := game
BUILD    := build
SOURCES  := src

# devkitPro paths
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM

include $(DEVKITPRO)/gba_rules
