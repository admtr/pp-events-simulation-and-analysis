// draw_plots.C
// Makro ROOT, ktore rysuje histogramy z analysis.root i zapisuje je jako PNG i PDF.
//
// Kategorie, zmienne i podpisy osi pochodza z include/Categories.h (te same co w analizie).
//
// Uruchomienie (tryb wsadowy, bez otwierania okien):
//   root -l -b -q 'macros/draw_plots.C("results/analysis.root", "results/plots")'
//
// Wynik w katalogu wyjsciowym:
//   - h<Zmienna>_<kategoria>.png/.pdf  kazdy histogram na osobnym canvasie (32 wykresy)
//   - cmp_<Zmienna>_<grupa>.png/.pdf   porownania kategorii z legenda (16 wykresow)
//   - all_plots.pdf                    wszystkie wykresy w jednym wielostronicowym PDF

#include "../include/Categories.h"

#include "TCanvas.h"
#include "TError.h"
#include "TFile.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

const TString kCollision = "pp, #sqrt{s} = 13.6 TeV";

struct LineStyle { int color; int style; };
const std::map<std::string, LineStyle> kStyles = {
  {"all",  {kGray + 2,  1}},
  {"ch",   {kBlack,     1}},
  {"pip",  {kBlue + 1,  1}},
  {"pim",  {kBlue + 1,  2}},
  {"Kp",   {kRed + 1,   1}},
  {"Km",   {kRed + 1,   2}},
  {"p",    {kGreen + 2, 1}},
  {"pbar", {kGreen + 2, 2}},
};

struct Group { const char* tag; const char* title; std::vector<std::string> cats; };
const std::vector<Group> kGroups = {
  {"pions",   "#pi^{+} vs #pi^{-}",             {"pip", "pim"}},
  {"kaons",   "K^{+} vs K^{-}",                 {"Kp", "Km"}},
  {"protons", "p vs #bar{p}",                   {"p", "pbar"}},
  {"species", "Charged and identified hadrons", {"ch", "pip", "pim", "Kp", "Km", "p", "pbar"}},
};

const ana::Category* FindCategory(const std::string& name) {
  for (const auto& c : ana::kCategories)
    if (name == c.name) return &c;
  return nullptr;
}

TH1D* GetHist(TFile* f, const ana::Variable& v, const ana::Category& c) {
  const std::string name = ana::HistName(v.name, c.name);
  auto* h = f->Get<TH1D>(name.c_str());
  if (!h) Warning("draw_plots", "Brak histogramu %s", name.c_str());
  return h;
}

std::vector<TCanvas*> gCanvases;

void SaveCanvas(TCanvas* c, const TString& dir, const TString& base) {
  c->SaveAs(dir + "/" + base + ".png");
  c->SaveAs(dir + "/" + base + ".pdf");
  gCanvases.push_back(c);
}

void SaveAllInOne(const TString& allPdf) {
  if (gCanvases.empty()) return;
  for (size_t i = 0; i < gCanvases.size(); ++i) {
    TString opt = (i == 0) ? "(" : (i + 1 == gCanvases.size() ? ")" : "");
    if (gCanvases.size() == 1) opt = "";
    gCanvases[i]->Print(allPdf + opt, TString("Title:") + gCanvases[i]->GetTitle());
  }
}

void DrawOutOfRangeNote(TH1D* h, const char* unit) {
  const double under = h->GetBinContent(0);
  const double over  = h->GetBinContent(h->GetNbinsX() + 1);
  if ((under <= 0. && over <= 0.) || h->GetEntries() <= 0.) return;
  auto part = [&](const char* label, double n) {
    const double pct = 100. * n / h->GetEntries();
    return pct >= 0.01 ? TString::Format("%s: %.0f %s (%.2f%%)", label, n, unit, pct)
                       : TString::Format("%s: %.0f %s (<0.01%%)", label, n, unit);
  };
  TLatex note;
  note.SetNDC();
  note.SetTextSize(0.035);
  double y = 0.85;
  if (under > 0.) { note.DrawLatex(0.16, y, part("Underflow", under)); y -= 0.05; }
  if (over > 0.)  note.DrawLatex(0.16, y, part("Overflow", over));
}

void DrawSingle(TFile* f, const ana::Variable& v, const ana::Category& cat, const TString& dir) {
  TH1D* h = GetHist(f, v, cat);
  if (!h) return;

  const TString base = ana::HistName(v.name, cat.name);
  auto* c = new TCanvas("c_" + base, base, 800, 600);
  if (v.logY) c->SetLogy();

  h->SetTitle(TString(cat.label) + ": " + v.descr + " (" + kCollision + ")");
  h->GetXaxis()->SetTitle(v.xTitle);
  h->GetYaxis()->SetTitle(v.yTitle);
  const LineStyle& st = kStyles.at(cat.name);
  h->SetLineColor(st.color);
  h->SetLineStyle(st.style);
  h->SetLineWidth(2);
  h->SetFillColorAlpha(st.color, 0.25);
  if (v.logY) { h->SetMaximum(h->GetMaximum() * 30.); h->SetMinimum(0.5); }
  else        { h->SetMaximum(h->GetMaximum() * 1.4); h->SetMinimum(0.); }
  h->Draw("HIST");

  DrawOutOfRangeNote(h, std::string(v.name) == "N" ? "events" : "particles");

  SaveCanvas(c, dir, base);
}

void DrawGroup(TFile* f, const ana::Variable& v, const Group& g, const TString& dir) {
  std::vector<std::pair<TH1D*, const ana::Category*>> hs;
  double ymax = 0.;

  for (const auto& name : g.cats) {
    const ana::Category* cat = FindCategory(name);
    if (!cat) continue;
    TH1D* h0 = GetHist(f, v, *cat);
    if (!h0) continue;

    auto* h = static_cast<TH1D*>(h0->Clone(TString(h0->GetName()) + "_" + g.tag));
    h->SetDirectory(nullptr);
    h->SetStats(0);
    h->SetLineColor(kStyles.at(name).color);
    h->SetLineStyle(kStyles.at(name).style);
    h->SetLineWidth(2);
    h->SetFillStyle(0);
    h->SetMaximum();
    h->SetMinimum();
    ymax = std::max(ymax, h->GetBinContent(h->GetMaximumBin()));
    hs.emplace_back(h, cat);
  }
  if (hs.empty()) return;

  const TString base = TString("cmp_") + v.name + "_" + g.tag;
  auto* c = new TCanvas("c_" + base, base, 800, 600);
  if (v.logY) c->SetLogy();

  TH1D* first = hs.front().first;
  first->SetTitle(TString(g.title) + ": " + v.descr + " (" + kCollision + ")");
  first->GetXaxis()->SetTitle(v.xTitle);
  first->GetYaxis()->SetTitle(v.yTitle);
  if (v.logY) { first->SetMaximum(ymax * 30.); first->SetMinimum(0.5); }
  else        { first->SetMaximum(ymax * 1.6); first->SetMinimum(0.); }

  for (size_t k = 0; k < hs.size(); ++k) hs[k].first->Draw(k == 0 ? "HIST" : "HIST SAME");

  const int nCol = (hs.size() > 4) ? 2 : 1;
  const int nRow = static_cast<int>(std::ceil(hs.size() / static_cast<double>(nCol)));
  auto* leg = new TLegend(nCol == 2 ? 0.50 : 0.68, 0.88 - 0.06 * nRow, 0.88, 0.88);
  leg->SetNColumns(nCol);
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);
  for (auto& [h, cat] : hs) leg->AddEntry(h, cat->label, "l");
  leg->Draw();

  SaveCanvas(c, dir, base);
}

}

void draw_plots(const char* fileName = "results/analysis.root", const char* outDir = "results/plots") {
  gErrorIgnoreLevel = kWarning;

  gStyle->SetOptStat(1110);
  gStyle->SetPadLeftMargin(0.13);
  gStyle->SetPadBottomMargin(0.12);
  gStyle->SetTitleOffset(1.3, "Y");

  TFile* f = TFile::Open(fileName, "READ");
  if (!f || f->IsZombie()) {
    Error("draw_plots", "Nie mozna otworzyc pliku %s", fileName);
    return;
  }

  const TString dir(outDir);
  gSystem->mkdir(dir, kTRUE);
  const TString allPdf = dir + "/all_plots.pdf";
  gCanvases.clear();

  for (const auto& v : ana::kVariables)
    for (const auto& cat : ana::kCategories)
      DrawSingle(f, v, cat, dir);

  for (const auto& v : ana::kVariables)
    for (const auto& g : kGroups)
      DrawGroup(f, v, g, dir);

  SaveAllInOne(allPdf);

  gErrorIgnoreLevel = kInfo;
  Info("draw_plots", "Gotowe. Wykresy w %s/ (zbiorczo: %s)", dir.Data(), allPdf.Data());
}
