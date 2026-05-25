CXX     ?= g++
CC      ?= gcc
NPROC   ?= $(shell nproc)
ifeq ($(filter -j%,$(MAKEFLAGS)),)
    MAKEFLAGS += -j$(NPROC)
endif

.DEFAULT_GOAL := help

WARNS := \
    -Wall -Wextra -Wpedantic              \
    -Wshadow                              \
    -Wconversion -Wsign-conversion        \
    -Wdouble-promotion                    \
    -Wold-style-cast                      \
    -Wcast-align -Wcast-qual              \
    -Wnull-dereference                    \
    -Wformat=2                            \
    -Wundef                               \
    -Wvla                                 \
    -Wlogical-op                          \
    -Wduplicated-cond                     \
    -Wduplicated-branches                 \
    -Wswitch-enum  -Werror=switch-enum    \
    -Wunused-result                       \
    -Wstack-usage=512                     \
    -Warray-bounds

DISABLED := \
    -fno-exceptions          \
    -fno-rtti                \
    -fno-threadsafe-statics  \
    -fstrict-aliasing

SECTIONS := \
    -ffunction-sections \
    -fdata-sections

TARGET := server
CXXFLAGS := $(WARNS) $(DISABLED) $(SECTIONS) -std=c++23 -fstack-usage
LDFLAGS  := -Wl,--gc-sections -Wl,-Map=$(TARGET).map
LDLIBS   := -luring
INCLUDES := -Isrc -I. -include src/util/no_heap.hpp


FLAGS_RELEASE       := -O3 -DNDEBUG -march=native -DPGS_LOG_COMPILE_LEVEL=3 # error
FLAGS_PROFILE       := -O2 -g3 -ggdb3 -gdwarf-4 -DNDEBUG -fno-omit-frame-pointer \
                         -fno-optimize-sibling-calls
FLAGS_PROFILING     := $(FLAGS_PROFILE) -DPROFILING
FLAGS_SAN           := -O1 -g -ggdb3 -fno-omit-frame-pointer \
                         -fsanitize=address,undefined
FLAGS_DEBUG         := -Og -g -ggdb3 -fno-omit-frame-pointer

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRCS := $(call rwildcard,src,*.cpp)

PAGEGEN_SRC := tools/generate_pages.cpp
PAGEGEN     := build/pagegen
PAGES_DIR   := src/pages
CSS_SRC     := src/assets/styles.css
ASSETS_DIR  := src/assets/img
PAGES_HPP   := src/generated/pages.hpp
ASSETS_HPP  := src/generated/assets.hpp
PAGES       := $(wildcard $(PAGES_DIR)/*.ppp)
ASSETS      := $(wildcard $(ASSETS_DIR)*)

define make_target
BUILD_DIR_$(1) := build/$(1)
OBJS_$(1)      := $$(SRCS:src/%.cpp=build/$(1)/%.o)
DEPS_$(1)      := $$(OBJS_$(1):.o=.d)

build/$(1)/%.o: src/%.cpp | $(PAGES_HPP) $(ASSETS_HPP)
	@mkdir -p $$(@D)
	$(CXX) $(CXXFLAGS) $(2) $(INCLUDES) -MMD -MP -c $$< -o $$@

-include $$(DEPS_$(1))

$(TARGET)_$(1): $$(OBJS_$(1))
	$(CXX) $(CXXFLAGS) $(2) $(3) $(LDFLAGS) $(INCLUDES) $$^ -o $$@ $(LDLIBS)
endef

$(eval $(call make_target,release,  $(FLAGS_RELEASE),))
$(eval $(call make_target,release_log,  $(FLAGS_RELEASE_LOGS),))
$(eval $(call make_target,profile,  $(FLAGS_PROFILE),))
$(eval $(call make_target,profiling,$(FLAGS_PROFILING),))
$(eval $(call make_target,san,      $(FLAGS_SAN),))
$(eval $(call make_target,debug,    $(FLAGS_DEBUG),))



$(PAGEGEN): $(PAGEGEN_SRC)
	@mkdir -p $(@D)
	@$(CXX) -std=c++23 -O2 -I. -Wall -Wextra -o $@ $<


$(PAGES_HPP) $(ASSETS_HPP): $(PAGEGEN) $(PAGES) $(CSS_SRC) $(ASSETS)
	@mkdir -p $(@D)
	$(PAGEGEN) -s $(PAGES_DIR) -o $(PAGES_HPP) -c $(CSS_SRC) \
	           -a $(ASSETS_DIR) -A $(ASSETS_HPP)

$(OBJS): $(PAGES_HPP) $(ASSETS_HPP)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES) $^ -o $@ $(LDLIBS)

build/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@



.PHONY: all clean run

debug:     $(TARGET)_debug ## dev, debug symbols, -Og
release:   $(TARGET)_release ## prod, -O3, -march=native
release_log:   $(TARGET)_release_log ## prod, -O3, -march=native, but with logs
profile:   $(TARGET)_profile ## perf
profiling: $(TARGET)_profiling ## profile + timings
san:       $(TARGET)_san ## assan + ubsan

help: ## Display this help
	@printf "\n\033[1mServer Help\033[0m\n\n"
	@awk '\
		/^#### / { \
			printf "\n\033[33;1m%s\033[0m\n", substr($$0, 6); \
			next \
		} \
		/^[a-zA-Z0-9_-]+:.*## / { \
			split($$0, parts, ":.*## "); \
			printf "   \033[036m%-18s\033[0m  %s\n", parts[1], parts[2]; \
		} \
	' $(firstword $(MAKEFILE_LIST))
	@echo ""

run: debug
	./$(TARGET)_debug $(ARGS)

clean:
	rm -rf build $(TARGET)_* *.map

-include $(DEPS)
