# Symulacja i analiza rozkładów cząstek dla pp @ √s = 13.6 TeV (Pythia 8 + ROOT)

Celem zadania było przygotowanie kompletnego pipeline od generacji zderzeń proton–proton przy energii √s = 13.6 TeV w Pythii 8, przez analizę cząstek końcowych w C++/ROOT, do wykresów w ROOT. Analiza tworzy histogramy krotności na zdarzenie (N), pędu poprzecznego (pT), pseudorapidity (η) i kąta azymutalnego (φ) dla wszystkich cząstek naładowanych oraz pionów π±, kaonów K± i protonów p/p̄.


## Wymagania

- **C++17** (g++ lub clang) i **GNU Make**
- **ROOT 6** (https://root.cern/install/)
- **Pythia 8** (https://pythia.org/)

**Wersja systemu, na której projekt był uruchamiany:**
- Windows + WSL2, Ubuntu 22.04
- g++ 11.4, GNU Make
- ROOT 6.36.12 (oficjalna paczka binarna dla Ubuntu 22.04)
- Pythia 8.318 (kompilacja ze źródeł)


## Kompilacja

W głównym katalogu projektu:
```bash
make build
```

Po kompilacji w katalogu `build/` znajdą się:
- `generate` - generator zdarzeń
- `analysis` - analiza i histogramy


## Uruchomienie

### Cały pipeline

```bash
# Kompilacja, generacja, analiza i wykresy
make

# Test calego pipeline na 1000 zdarzeniach (wyniki w osobnym katalogu test/)
make test
make test TEST_EVENTS=5000
```

`make` wykonuje tylko te kroki, których wejście się zmieniło:
- zmiana `macros/draw_plots.C` - tylko nowe wykresy,
- zmiana `src/analysis.cc` lub `include/Categories.h` - nowe histogramy i wykresy, bez ponownej generacji,
- zmiana `config/*.cmnd` lub `src/generate.cc` - cały łańcuch od nowa.

### Poszczególne etapy

Każdy krok można uruchomić osobno przez `make` albo bezpośrednio. Kroki trzeba wykonywać w kolejności 1 → 2 → 3, bo każdy czyta wynik poprzedniego.

### 1. Generacja zdarzeń
```bash
make generate
# lub bezposrednio:
./build/generate config/minbias_13600.cmnd data/events.root
```

**Opcje:**
- `<config.cmnd>` - plik ustawień Pythii (domyślnie: `config/minbias_13600.cmnd`)
- `<wyjście.root>` - plik ze zdarzeniami (domyślnie: `data/events.root`)
- `"Ustawienie = wartość" ...` - dowolna liczba ustawień Pythii nadpisujących plik `.cmnd`, np.:
```bash
./build/generate config/minbias_13600.cmnd out.root "Main:numberOfEvents = 1000" "Random:seed = 42"
```
- przez `make`: `make CONFIG=config/inny.cmnd`

**Wyjście:** drzewo `events` (jeden wpis = jedno zdarzenie) z gałęziami `id` (kod PDG), `charge`, `pt`, `eta`, `phi` oraz obiekt `generator_info` z ustawieniami generatora.

### 2. Analiza danych
```bash
make analysis
# lub bezposrednio:
./build/analysis data/events.root results/analysis.root
```

**Wyjście:** 32 histogramy w `results/analysis.root`:
- `hN_*` - krotność na zdarzenie
- `hPt_*` - pT [GeV]
- `hEta_*` - η
- `hPhi_*` - |φ| [rad]

Kategorie: `all`, `ch`, `pip`, `pim`, `Kp`, `Km`, `p`, `pbar`. Na koniec program wypisuje średnie krotności.

### 3. Generowanie wykresów
```bash
make plots
# lub bezposrednio:
root -l -b -q 'macros/draw_plots.C("results/analysis.root", "results/plots")'
```

**Wyjście** w `results/plots/`:
- 32 wykresy pojedyncze `h<Zmienna>_<kategoria>.png/.pdf`
- 16 wykresów porównawczych `cmp_<Zmienna>_<grupa>.png/.pdf` (π⁺/π⁻, K⁺/K⁻, p/p̄ i wszystkie gatunki razem)
- `all_plots.pdf` - wszystkie 48 wykresów w jednym pliku

### Czyszczenie
```bash
make clean        # usuwa build/ i test/
make clean-data   # usuwa data/events.root
```

## Struktura projektu

```
cern_00/
├── Makefile                  
├── config/
│   └── minbias_13600.cmnd              # konfiguracja generatora
├── include/
│   └── Categories.h                    # kategorie cząstek i binowanie 
├── src/
│   ├── generate.cc                     # [1] generator Pythia 8
│   └── analysis.cc                     # [2] analiza i histogramy
├── macros/
│   └── draw_plots.C                    # [3] makro rysujące
├── data/                     
├── results/
│   ├── analysis.root                   # histogramy hN_*, hPt_*, hEta_*, hPhi_*
│   └── plots/                          # wykresy PNG/PDF
├── build/                              # skompilowane programy (poza repozytorium)
├── pp_events_simulation_analysis.pdf   #prezentacja z histogramami                 
└── test/                               # wyniki "make test" (poza repozytorium)

```

## Decyzje implementacyjne

**Minimum bias = `SoftQCD:nonDiffractive`**
- Zadanie nie wskazuje konkretnej flagi SoftQCD.
- Dokumentacja Pythii definiuje minimum bias jako zdarzenia nieelastyczne niedyfrakcyjne (https://pythia8.web.cern.ch/manuals/pythia8318/QCDSoftProcesses.html).

**Zakres φ**
- Pythia zwraca φ w zakresie (-π, π], a zadany zakres to 0-π.
- Rozkład jest symetryczny, więc wpisywane jest |φ| (stała `kFoldPhi`). Żadna cząstka nie wypada poza zakres.

**Wpisy spoza zakresu histogramów**
- Zakresy z zadania nie obejmują wszystkiego: ok. 25% cząstek ma |η| > 5, a w ok. 17% zdarzeń krotność `all` przekracza 400.
- Takie wpisy trafiają do underflow/overflow. Na wykresie pojawia się wtedy napis z ich liczbą i procentem.
- `TH1::StatOverflows(true)` sprawia, że średnia i odchylenie standardowe w ramce statystyk liczone są ze wszystkich wpisów.


## Wyniki

Średnie krotności na zdarzenie (100 000 zdarzeń, seed 14032026):
- wszystkie cząstki końcowe: 240.80 (17.34% zdarzeń z N > 400)
- naładowane: 114.94 (0.12% zdarzeń z N > 400)
- π⁺ / π⁻: 47.93 / 47.22
- K⁺ / K⁻: 5.34 / 5.21
- p / p̄: 4.44 / 3.29

Obserwacje:
- Piony to ok. 83% cząstek naładowanych, kaony ok. 9%, protony ok. 7%.
- Cząstek dodatnich jest więcej niż ujemnych, najwyraźniej dla p wobec p̄. Wynika to z zachowania ładunku i liczby barionowej (stan początkowy pp ma ładunek +2 i liczbę barionową +2).
- Krotność cząstek naładowanych jest zawsze parzysta (też z zachowania ładunku), dlatego co drugi bin `hN_ch` jest pusty.
- Rozkłady φ są płaskie, a rozkłady η symetryczne względem zera, zgodnie z symetrią zderzenia.
