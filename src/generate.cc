// generate.cc
// Generacja zdarzen pp w Pythii 8 i zapis czastek koncowych do pliku ROOT.
//
// Wszystkie ustawienia fizyczne (energia, procesy, seed, liczba zdarzen) sa w pliku .cmnd.
// Dodatkowe argumenty to pojedyncze ustawienia Pythii, ktore nadpisuja wartosci z pliku.
//
// Uzycie:
//   ./build/generate <config.cmnd> <wyjscie.root> ["Ustawienie = wartosc" ...]
// Przyklady:
//   ./build/generate config/minbias_13600.cmnd data/events.root
//   ./build/generate config/minbias_13600.cmnd test/events.root "Main:numberOfEvents = 1000"
//
// Zawartosc pliku wyjsciowego:
//   - TTree "events": jeden wpis = jedno zdarzenie, galezie (wektory po czastkach):
//       id     kod PDG (np. 211 = pi+, -321 = K-, 2212 = p)
//       charge ladunek [e]
//       pt     ped poprzeczny [GeV]
//       eta    pseudorapidity
//       phi    kat azymutalny [rad], zakres (-pi, pi]
//   - TNamed "generator_info": wersja Pythii i wszystkie zmienione ustawienia (do reprodukcji)

#include "Pythia8/Pythia.h"

#include "TFile.h"
#include "TNamed.h"
#include "TTree.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string GeneratorInfo(Pythia8::Pythia& pythia, long nAccepted) {
  std::ostringstream os;
  os << "Pythia version: " << pythia.parm("Pythia:versionNumber") << "\n"
     << "Accepted events: " << nAccepted << "\n"
     << "sigma_gen [mb]: " << pythia.info.sigmaGen() << "\n"
     << "Changed settings:\n";
  pythia.settings.writeFile(os, false);
  return os.str();
}

}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Uzycie: " << argv[0]
              << " <config.cmnd> <wyjscie.root> [\"Ustawienie = wartosc\" ...]\n";
    return 1;
  }
  const std::string configFile = argv[1];
  const std::string outName    = argv[2];

  Pythia8::Pythia pythia;
  if (!pythia.readFile(configFile)) {
    std::cerr << "Blad odczytu pliku konfiguracyjnego " << configFile << "\n";
    return 1;
  }
  for (int i = 3; i < argc; ++i) {
    if (!pythia.readString(argv[i])) {
      std::cerr << "Nieznane ustawienie: " << argv[i] << "\n";
      return 1;
    }
  }

  const int nEvents   = pythia.mode("Main:numberOfEvents");
  const int maxErrors = pythia.mode("Main:timesAllowErrors");

  if (!pythia.init()) {
    std::cerr << "Blad inicjalizacji Pythii\n";
    return 1;
  }

  TFile outFile(outName.c_str(), "RECREATE");
  if (outFile.IsZombie()) {
    std::cerr << "Nie mozna utworzyc pliku " << outName << "\n";
    return 1;
  }
  TTree tree("events", "Final-state particles, pp @ 13.6 TeV");

  std::vector<int>   id, charge;
  std::vector<float> pt, eta, phi;
  tree.Branch("id",     &id);
  tree.Branch("charge", &charge);
  tree.Branch("pt",     &pt);
  tree.Branch("eta",    &eta);
  tree.Branch("phi",    &phi);

  long nAccepted = 0;
  int  nErrors   = 0;
  while (nAccepted < nEvents) {
    if (!pythia.next()) {
      if (++nErrors > maxErrors) {
        std::cerr << "Przekroczono Main:timesAllowErrors = " << maxErrors << ", przerywam.\n";
        break;
      }
      continue;
    }

    id.clear(); charge.clear(); pt.clear(); eta.clear(); phi.clear();

    for (int i = 0; i < pythia.event.size(); ++i) {
      const Pythia8::Particle& p = pythia.event[i];
      if (!p.isFinal()) continue;

      id.push_back(p.id());
      charge.push_back(p.chargeType() / 3);
      pt.push_back(p.pT());
      eta.push_back(p.eta());
      phi.push_back(p.phi());
    }

    tree.Fill();
    ++nAccepted;
  }

  pythia.stat();

  tree.Write();
  TNamed("generator_info", GeneratorInfo(pythia, nAccepted)).Write();
  outFile.Close();

  std::cout << "\nZapisano " << nAccepted << " zdarzen do " << outName << "\n";
  return 0;
}
