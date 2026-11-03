// analysis.cc
// Krok 2 pipeline'u: czyta czastki koncowe z pliku zdarzen (wynik build/generate)
// i tworzy histogramy hN_*, hPt_*, hEta_*, hPhi_* dla kazdej kategorii czastek.
//
// Kategorie i binowanie sa zdefiniowane w include/Categories.h.
//
// Uzycie:
//   ./build/analysis <zdarzenia.root> <analysis.root>
// Przyklad:
//   ./build/analysis data/events.root results/analysis.root
//
// Zawartosc pliku wyjsciowego:
//   - hN_<kat>    krotnosc czastek danej kategorii na zdarzenie
//   - hPt_<kat>   pT kazdej czastki kategorii
//   - hEta_<kat>  eta kazdej czastki kategorii
//   - hPhi_<kat>  |phi| (lub phi, patrz kFoldPhi) kazdej czastki kategorii
//   - generator_info  kopia informacji o generacji z pliku wejsciowego

#include "Categories.h"

#include "TFile.h"
#include "TH1D.h"
#include "TNamed.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

const double kPhiMaxInRange = std::nextafter(TMath::Pi(), 0.);

struct CategoryHists {
  const ana::Category* cat;
  TH1D* hN;
  TH1D* hPt;
  TH1D* hEta;
  TH1D* hPhi;
  int   count = 0;
};

TH1D* MakeHist(const ana::Variable& v, const ana::Category& c) {
  const std::string title = std::string(c.label) + ";" + v.xTitle + ";" + v.yTitle;
  return new TH1D(ana::HistName(v.name, c.name).c_str(), title.c_str(), v.nBins, v.xMin, v.xMax);
}

}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Uzycie: " << argv[0] << " <zdarzenia.root> <analysis.root>\n";
    return 1;
  }
  const std::string inName  = argv[1];
  const std::string outName = argv[2];

  TFile inFile(inName.c_str(), "READ");
  if (inFile.IsZombie()) {
    std::cerr << "Nie mozna otworzyc pliku " << inName << "\n";
    return 1;
  }
  TTreeReader reader("events", &inFile);
  TTreeReaderArray<int>   id(reader, "id");
  TTreeReaderArray<int>   charge(reader, "charge");
  TTreeReaderArray<float> pt(reader, "pt");
  TTreeReaderArray<float> eta(reader, "eta");
  TTreeReaderArray<float> phi(reader, "phi");

  TFile outFile(outName.c_str(), "RECREATE");
  if (outFile.IsZombie()) {
    std::cerr << "Nie mozna utworzyc pliku " << outName << "\n";
    return 1;
  }

  const auto& vars = ana::kVariables;
  TH1::StatOverflows(true);
  std::vector<CategoryHists> hists;
  for (const auto& c : ana::kCategories)
    hists.push_back({&c, MakeHist(vars[0], c), MakeHist(vars[1], c),
                         MakeHist(vars[2], c), MakeHist(vars[3], c)});

  Long64_t nEv = 0;
  while (reader.Next()) {
    for (auto& h : hists) h.count = 0;

    for (size_t i = 0; i < id.GetSize(); ++i) {
      const double phiVal = ana::kFoldPhi ? std::min<double>(std::fabs(phi[i]), kPhiMaxInRange) : phi[i];

      for (auto& h : hists) {
        if (!h.cat->Accepts(id[i], charge[i])) continue;
        ++h.count;
        h.hPt->Fill(pt[i]);
        h.hEta->Fill(eta[i]);
        h.hPhi->Fill(phiVal);
      }
    }

    for (auto& h : hists) h.hN->Fill(h.count);

    if (++nEv % 10000 == 0) std::cout << "Przetworzono " << nEv << " zdarzen\n";
  }

  if (auto* info = inFile.Get<TNamed>("generator_info")) {
    outFile.cd();
    info->Clone()->Write();
  }

  std::cout << "\nLiczba zdarzen: " << nEv << "\n"
            << std::left << std::fixed << std::setprecision(2)
            << std::setw(8) << "kat." << std::setw(10) << "<N>" << "zdarzenia z N > 400 (overflow)\n";
  for (const auto& h : hists) {
    const double over = h.hN->GetBinContent(h.hN->GetNbinsX() + 1);
    std::cout << std::setw(8) << h.cat->name << std::setw(10) << h.hN->GetMean()
              << static_cast<long>(over) << " (" << (nEv ? 100. * over / nEv : 0.) << "%)\n";
  }

  outFile.Write();
  outFile.Close();
  std::cout << "Zapisano histogramy do " << outName << "\n";
  return 0;
}
