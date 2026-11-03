// Categories.h
// Wspolne definicje dla analizy (src/analysis.cc) i rysowania (macros/draw_plots.C):
//   - kategorie czastek: nazwa (sufiks histogramu), opis i warunek wyboru,
//   - zmienne (N, pT, eta, phi), podpisy osi i binowanie histogramow.
//
// Etykiety uzywaja skladni LaTeX ROOT-a ("#" zamiast "\"), np. "#pi^{+}".

#ifndef CATEGORIES_H
#define CATEGORIES_H

#include "TMath.h"

#include <string>
#include <vector>

namespace ana {

enum class Selection {
  kAll,
  kCharged,
  kPdg
};

struct Category {
  const char* name;
  const char* label;
  Selection   sel;
  int         pdg;

  bool Accepts(int id, int charge) const {
    switch (sel) {
      case Selection::kAll:     return true;
      case Selection::kCharged: return charge != 0;
      case Selection::kPdg:     return id == pdg;
    }
    return false;
  }
};

inline const std::vector<Category> kCategories = {
  {"all",  "All final-state particles", Selection::kAll,     0},
  {"ch",   "Charged particles",         Selection::kCharged, 0},
  {"pip",  "#pi^{+}",                   Selection::kPdg,     211},
  {"pim",  "#pi^{-}",                   Selection::kPdg,    -211},
  {"Kp",   "K^{+}",                     Selection::kPdg,     321},
  {"Km",   "K^{-}",                     Selection::kPdg,    -321},
  {"p",    "p",                         Selection::kPdg,     2212},
  {"pbar", "#bar{p}",                   Selection::kPdg,    -2212},
};

constexpr bool kFoldPhi = true;

struct Variable {
  const char* name;
  const char* xTitle;
  const char* yTitle;
  const char* descr;
  int    nBins;
  double xMin;
  double xMax;
  bool   logY;
};

inline const std::vector<Variable> kVariables = {
  {"N",   "Multiplicity N",                          "Events",    "multiplicity",        400,  0., 400.,         true},
  {"Pt",  "p_{T} [GeV]",                             "Particles", "transverse momentum", 100,  0., 10.,          true},
  {"Eta", "#eta",                                    "Particles", "pseudorapidity",      100, -5., 5.,           false},
  {"Phi", kFoldPhi ? "|#phi| [rad]" : "#phi [rad]",  "Particles", "azimuthal angle",      64,  0., TMath::Pi(),  false},
};

inline std::string HistName(const char* var, const char* cat) {
  return std::string("h") + var + "_" + cat;
}

}

#endif
