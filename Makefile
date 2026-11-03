# Makefile: kompilacja i caly pipeline analizy.
#
#   make             cala produkcja: generacja -> analiza -> wykresy (100 000 zdarzen)
#   make build       tylko kompilacja programow do build/
#   make generate    tylko krok 1: data/events.root
#   make analysis    kroki 1-2:    results/analysis.root
#   make plots       kroki 1-3:    results/plots/
#   make test        szybki test calego lancucha na TEST_EVENTS zdarzeniach, wyniki w test/
#   make clean       usuwa build/ i test/ (dane i wyniki zostaja)
#   make clean-data  usuwa data/events.root (ponowna generacja trwa kilka minut)
#
# Make uruchamia tylko te kroki, ktorych wejscie sie zmienilo. Np. po zmianie
# macros/draw_plots.C "make" przerysuje wykresy, ale nie bedzie generowal zdarzen od nowa.
# Zmiana config/*.cmnd albo src/generate.cc powoduje ponowna generacje.
#
# Wymaga root-config i pythia8-config w PATH.

# --- Pliki i katalogi (mozna nadpisac, np. make CONFIG=config/inny.cmnd) ---
CONFIG      ?= config/minbias_13600.cmnd
EVENTS      := data/events.root
ANALYSIS    := results/analysis.root
PLOTS_DIR   := results/plots
PLOTS_STAMP := $(PLOTS_DIR)/all_plots.pdf

TEST_EVENTS ?= 1000
TEST_DIR    := test

# --- Sprawdzenie srodowiska ---
ifeq ($(filter clean clean-data,$(MAKECMDGOALS)),)
  ifeq ($(shell command -v root-config 2>/dev/null),)
    $(error Nie znaleziono root-config w PATH. Wykonaj "source <katalog ROOT>/bin/thisroot.sh" (patrz README, sekcja Instalacja))
  endif
  ifeq ($(shell command -v pythia8-config 2>/dev/null),)
    $(error Nie znaleziono pythia8-config w PATH. Dodaj <katalog Pythii>/bin do PATH (patrz README, sekcja Instalacja))
  endif
endif

# --- Kompilacja ---
CXX          ?= g++
CXXFLAGS     := -O2 -Wall -Wextra -Iinclude
ROOT_CFLAGS  := $(shell root-config --cflags 2>/dev/null)
ROOT_LIBS    := $(shell root-config --libs 2>/dev/null) -Wl,-rpath,$(shell root-config --libdir 2>/dev/null)
PY8_CFLAGS   := $(filter-out -std=% -pedantic,$(shell pythia8-config --cxxflags 2>/dev/null))
PY8_LIBS     := $(shell pythia8-config --libs 2>/dev/null) -Wl,-rpath,$(shell pythia8-config --libdir 2>/dev/null)

ROOT_BATCH   := root -l -b -q

# --- Cele glowne ---
all: plots

build: build/generate build/analysis
generate: $(EVENTS)
analysis: $(ANALYSIS)
plots: $(PLOTS_STAMP)

# --- Programy ---
build/generate: src/generate.cc | build/
	$(CXX) $(CXXFLAGS) $(PY8_CFLAGS) $(ROOT_CFLAGS) $< -o $@ $(ROOT_LIBS) $(PY8_LIBS)

build/analysis: src/analysis.cc include/Categories.h | build/
	$(CXX) $(CXXFLAGS) $(ROOT_CFLAGS) $< -o $@ $(ROOT_LIBS)

# --- Krok 1: generacja zdarzen ---
$(EVENTS): build/generate $(CONFIG) | data/
	./build/generate $(CONFIG) $@

# --- Krok 2: analiza -> histogramy ---
$(ANALYSIS): build/analysis $(EVENTS) | results/
	./build/analysis $(EVENTS) $@

# --- Krok 3: wykresy ---
$(PLOTS_STAMP): macros/draw_plots.C include/Categories.h $(ANALYSIS)
	$(ROOT_BATCH) 'macros/draw_plots.C("$(ANALYSIS)", "$(PLOTS_DIR)")'

# --- Test calego lancucha ---
test: build | $(TEST_DIR)/
	./build/generate $(CONFIG) $(TEST_DIR)/events.root "Main:numberOfEvents = $(TEST_EVENTS)"
	./build/analysis $(TEST_DIR)/events.root $(TEST_DIR)/analysis.root
	$(ROOT_BATCH) 'macros/draw_plots.C("$(TEST_DIR)/analysis.root", "$(TEST_DIR)/plots")'

# --- Katalogi tworzone w razie potrzeby ---
%/:
	mkdir -p $@

# --- Sprzatanie ---
clean:
	rm -rf build $(TEST_DIR)

clean-data:
	rm -f $(EVENTS)

.PHONY: all build generate analysis plots test clean clean-data
