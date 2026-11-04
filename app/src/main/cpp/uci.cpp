// ---------------------------------------------------------------------------
//  Light Red - vrstva UCI (implementace)
// ---------------------------------------------------------------------------

#include "uci.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace lightred {

namespace {

const char* JMENO = "Light Red 2.11.2";
const char* AUTOR = "Stanislav, prepis do C++ 2026";

inline std::string mala_pismena(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// hodnota v setinách pěšce, jak čeká UCI
//
// RedCry krok L1: originální Light Red hlásil ZMĚNU proti pozici, ve které
// hledání začalo - kořen má hodnotu 0 a tahy přičítají jen to, co seberou.
// RedCry hlásí hodnotu pozice: k nematové hodnotě přičte `material`, tedy
// bilanci pozice z pohledu strany na tahu. O matu se rozhoduje z hodnoty
// hledání PŘED přičtením, jinak by třeba 45 + 10 vyšlo jako falešný mat.
// Hledání ani volba tahu se tím nemění.
int na_centipawny(double h, double material) {
    // Mat se v Light Red počítá jako hodnota krále dělená stem, tedy stovky.
    if (h > 50.0)  return  30000;
    if (h < -50.0) return -30000;
    return static_cast<int>(std::lround((h + material) * 100.0));
}

// RedCry krok L1: materiál na desce z pohledu strany na tahu, s hodnotami
// figur samotného Light Red (dáma 10, ne 9). Král se nepočítá.
double materialova_bilance(const Pozice& p) {
    double m = 0.0;
    for (int x = 1; x <= 8; ++x)
        for (int y = 1; y <= 8; ++y) {
            const Policko& pole = p.sachovnice[x][y];
            if (pole.strana == NIKDO || pole.figura == KRAL) continue;
            const double h = hodnota_figur[pole.figura];
            m += (pole.strana == p.na_tahu_je) ? h : -h;
        }
    return m;
}

unsigned zabal_tah(const Tah& t) {
    return (static_cast<unsigned>(t.z_pole.x) << 16) |
           (static_cast<unsigned>(t.z_pole.y) << 12) |
           (static_cast<unsigned>(t.na_pole.x) << 8) |
           (static_cast<unsigned>(t.na_pole.y) << 4) |
            static_cast<unsigned>(t.promen_pesce);
}

Tah rozbal_tah(unsigned k) {
    Tah t;
    t.z_pole  = {static_cast<int>((k >> 16) & 0xF), static_cast<int>((k >> 12) & 0xF)};
    t.na_pole = {static_cast<int>((k >>  8) & 0xF), static_cast<int>((k >>  4) & 0xF)};
    t.promen_pesce = static_cast<Figura>(k & 0xF);
    return t;
}


// --- SAN --------------------------------------------------------------------

inline Strana souper(Strana s) { return (s == BILY) ? CERNY : BILY; }

// Písmena figur jsou ANGLICKÁ bez ohledu na jazyk aplikace - tak se SAN píše
// všude a tak ho bude číst i PGN. Pěšec písmeno nemá.
char pismeno_figury(Figura f) {
    switch (f) {
        case VEZ:     return 'R';
        case STRELEC: return 'B';
        case JEZDEC:  return 'N';
        case DAMA:    return 'Q';
        case KRAL:    return 'K';
        default:      return '\0';   // pěšec
    }
}

void pripoj_pole(std::string& s, const Bod& b) {
    s += static_cast<char>('a' + b.x - 1);
    s += static_cast<char>('0' + b.y);
}

// Co je o jednom legálním tahu potřeba vědět, než se dá zapsat. Rozlišení
// se počítá až z celé sady, proto se nejdřív posbírá tohle a teprve pak
// se skládají řetězce.
struct SanZaznam {
    Tah    tah{};
    Figura figura = ZADNA;   // táhnoucí kámen
    bool   bere = false;
    bool   rosada = false;   // true = malá nebo velká, pozná se podle na_pole.x
    bool   sach = false;
    bool   mat  = false;
};

}  // namespace

void PrubeznyStav::vynuluj(double material) {
    material_ = material;
    hloubka_.store(0, std::memory_order_relaxed);
    skore_.store(0, std::memory_order_relaxed);
    listu_.store(0, std::memory_order_relaxed);
    tah_.store(0, std::memory_order_relaxed);
}

void PrubeznyStav::zapis(const PrubeznyVysledek& p) {
    skore_.store(na_centipawny(p.hodnota, material_), std::memory_order_relaxed);
    listu_.store(static_cast<long long>(p.listu), std::memory_order_relaxed);
    tah_.store(p.tah_nalezen ? zabal_tah(p.nejlepsi_tah) : 0u,
               std::memory_order_relaxed);
    // hloubka se zapisuje POSLEDNÍ - je to příznak, že ostatní čtyři
    // hodnoty už patří k sobě
    hloubka_.store(p.hloubka, std::memory_order_release);
}

std::string PrubeznyStav::text() const {
    const int h = hloubka_.load(std::memory_order_acquire);
    if (h <= 0) return {};
    std::ostringstream o;
    o << "depth " << h
      << " score cp " << skore_.load(std::memory_order_relaxed)
      << " nodes " << listu_.load(std::memory_order_relaxed);
    const unsigned k = tah_.load(std::memory_order_relaxed);
    if (k) o << " pv " << tah_na_text(rozbal_tah(k));
    return o.str();
}

void UciEngine::hlaseni_trampolina(void* kontext, const PrubeznyVysledek& p) {
    UciEngine* self = static_cast<UciEngine*>(kontext);
    self->prubeh_.zapis(p);
    std::ostringstream o;
    o << "info depth " << p.hloubka
      << " score cp " << na_centipawny(p.hodnota, self->material_)
      << " nodes " << p.listu;
    if (p.tah_nalezen) o << " pv " << tah_na_text(p.nejlepsi_tah);
    self->info_radky_.push_back(o.str());
}

// --- legální tahy i se zápisem v SAN ----------------------------------------
//
// Dva průchody. První zahraje každý legální tah na kopii pozice a zjistí, co
// jde zjistit jen se zahraným tahem: šach a mat. Druhý skládá řetězce, protože
// rozlišení potřebuje vidět všechny ostatní legální tahy naráz.
std::vector<std::pair<std::string, std::string>> tahy_se_zapisem(const Pozice& pozice) {
    // Tentýž postup jako `legalmoves`, včetně pořadí: generátor vydá
    // pseudolegální tahy a filtr z nich vyhodí ty, po kterých zůstane vlastní
    // král v šachu.
    Pozice q = pozice;
    zjisti_vsechny_legalni_tahy(q);
    vymazej_tahy_kdy_je_v_sachu(q);

    const Strana ja = pozice.na_tahu_je;
    const Strana on = souper(ja);

    std::vector<SanZaznam> zaznamy;
    zaznamy.reserve(static_cast<size_t>(q.pocet_moznych_tahu));

    for (int i = 0; i < q.pocet_moznych_tahu; ++i) {
        SanZaznam z;
        z.tah    = q.tahy[i];
        z.figura = pozice.sachovnice[z.tah.z_pole.x][z.tah.z_pole.y].figura;

        // Braní: u pěšce stačí změna sloupce, a tím se bez zvláštní větve
        // pokryje i braní mimochodem, kde je cílové pole prázdné. U ostatních
        // figur rozhoduje obsazenost cílového pole.
        z.bere = (z.figura == PESEC)
                     ? (z.tah.z_pole.x != z.tah.na_pole.x)
                     : (pozice.sachovnice[z.tah.na_pole.x][z.tah.na_pole.y].figura != ZADNA);

        // Rošádu Light Red v `Tah` výslovně označenou nemá. `zahraj_tah` ji pozná
        // podle toho, že táhne král a mění sloupec o dva (light_red_core.cpp,
        // "rošáda -> pohni ještě věží"), a malá od velké se tam rozlišuje testem
        // na_pole.x == 7. Tentýž test je tady - jiný by se s jádrem mohl rozejít.
        z.rosada = (z.figura == KRAL) && (std::abs(z.tah.z_pole.x - z.tah.na_pole.x) == 2);

        // Šach a mat na kopii. `zahraj_tah` stranu na tahu NEPŘEPÍNÁ, to dělá až
        // `zahraj_a_prepni` - tady se to tedy přepne ručně.
        Pozice r = pozice;
        zahraj_tah(r, z.tah);
        r.na_tahu_je = on;

        const Bod kral = (on == BILY) ? r.pozice_bileho_krale : r.pozice_cerneho_krale;
        z.sach = otestuj_zda_neni_napadene(r, kral.x, kral.y, ja);

        // Mat se ptát nemá smysl, dokud není šach - a stojí to celý další filtr
        // legality, takže se to vyplácí. `vymazej_tahy_kdy_je_v_sachu` nastaví
        // `stav` na MAT / PAT / NORMALNI.
        if (z.sach) {
            zjisti_vsechny_legalni_tahy(r);
            vymazej_tahy_kdy_je_v_sachu(r);
            z.mat = (r.stav == MAT);
        }

        zaznamy.push_back(z);
    }

    std::vector<std::pair<std::string, std::string>> ven;
    ven.reserve(zaznamy.size());

    for (const SanZaznam& z : zaznamy) {
        std::string san;

        if (z.rosada) {
            san = (z.tah.na_pole.x == 7) ? "O-O" : "O-O-O";   // písmeno O, ne nula
        } else if (z.figura == PESEC) {
            // Pěšec nemá písmeno a rozlišení nikdy nepotřebuje; u braní se
            // sloupec výchozího pole uvádí VŽDY, i když je tah jednoznačný.
            if (z.bere) {
                san += static_cast<char>('a' + z.tah.z_pole.x - 1);
                san += 'x';
            }
            pripoj_pole(san, z.tah.na_pole);
            if (z.tah.promen_pesce != ZADNA) {
                san += '=';
                san += pismeno_figury(z.tah.promen_pesce);
            }
        } else {
            san += pismeno_figury(z.figura);

            // Rozlišení jen tehdy, když na totéž pole může JINÁ figura téhož
            // druhu a téže barvy. Počítá se ze `zaznamy`, tedy z LEGÁLNÍCH
            // tahů - vázaný jezdec, který tam legálně nemůže, rozlišení
            // nevynucuje.
            bool kolize = false, sloupec_kolize = false, rada_kolize = false;
            for (const SanZaznam& j : zaznamy) {
                if (j.figura != z.figura) continue;
                if (j.tah.na_pole.x != z.tah.na_pole.x) continue;
                if (j.tah.na_pole.y != z.tah.na_pole.y) continue;
                if (j.tah.z_pole.x == z.tah.z_pole.x &&
                    j.tah.z_pole.y == z.tah.z_pole.y) continue;
                kolize = true;
                if (j.tah.z_pole.x == z.tah.z_pole.x) sloupec_kolize = true;
                if (j.tah.z_pole.y == z.tah.z_pole.y) rada_kolize    = true;
            }
            if (kolize) {
                // Přednost má sloupec, pak řada, nakonec celé výchozí pole.
                if (!sloupec_kolize) {
                    san += static_cast<char>('a' + z.tah.z_pole.x - 1);
                } else if (!rada_kolize) {
                    san += static_cast<char>('0' + z.tah.z_pole.y);
                } else {
                    pripoj_pole(san, z.tah.z_pole);
                }
            }

            if (z.bere) san += 'x';
            pripoj_pole(san, z.tah.na_pole);
        }

        if      (z.mat)  san += '#';
        else if (z.sach) san += '+';

        ven.emplace_back(tah_na_text(z.tah), san);
    }

    return ven;
}

std::vector<std::string> rozdel(const std::string& s) {
    std::vector<std::string> ven;
    std::istringstream in(s);
    std::string w;
    while (in >> w) ven.push_back(w);
    return ven;
}

UciEngine::UciEngine() {
    nast_.max_hloubka = 4;
    nast_.max_hledej_klid = 6;   // prohlubovani zapnute, na rozdil od originalu
    nast_.zahajovaci_preference = true;
    nast_.postupne_prohlubovani = true;   // aby slo hledani kdykoli prerusit
    nova_hra();
}

void UciEngine::nova_hra() {
    nacti_uvodni_pozici(pozice_);
}

void UciEngine::zastav_hledani() {
    zastav_.store(true, std::memory_order_relaxed);
}

// --- vyhledá tah podle zápisu typu e2e4 / a7a8q v seznamu legálních tahů ----
bool UciEngine::najdi_tah(const std::string& zapis, Tah& ven) const {
    if (zapis.size() < 4) return false;

    const int zx = zapis[0] - 'a' + 1;
    const int zy = zapis[1] - '0';
    const int nx = zapis[2] - 'a' + 1;
    const int ny = zapis[3] - '0';

    Figura promena = ZADNA;
    if (zapis.size() >= 5) {
        switch (std::tolower(static_cast<unsigned char>(zapis[4]))) {
            case 'q': promena = DAMA;    break;
            case 'r': promena = VEZ;     break;
            case 'b': promena = STRELEC; break;
            case 'n': promena = JEZDEC;  break;
            default:  return false;
        }
    }

    Pozice q = pozice_;
    zjisti_vsechny_legalni_tahy(q);
    vymazej_tahy_kdy_je_v_sachu(q);

    for (int i = 0; i < q.pocet_moznych_tahu; ++i) {
        const Tah& t = q.tahy[i];
        if (t.z_pole.x != zx || t.z_pole.y != zy) continue;
        if (t.na_pole.x != nx || t.na_pole.y != ny) continue;
        if (promena != ZADNA && t.promen_pesce != promena) continue;
        ven = t;
        return true;
    }
    return false;
}

void UciEngine::zahraj_a_prepni(const Tah& t) {
    const bool byl_pesec = pozice_.sachovnice[t.z_pole.x][t.z_pole.y].figura == PESEC;
    const bool bere = pozice_.sachovnice[t.na_pole.x][t.na_pole.y].figura != ZADNA ||
                      (byl_pesec && t.na_pole.x == pozice_.en_passant.x &&
                                    t.na_pole.y == pozice_.en_passant.y);
    const Strana tahl = pozice_.na_tahu_je;

    zahraj_tah(pozice_, t);
    pozice_.na_tahu_je = (tahl == BILY) ? CERNY : BILY;

    if (byl_pesec || bere) pozice_.pultahy_bez_zmeny = 0;
    else                   ++pozice_.pultahy_bez_zmeny;
    if (tahl == CERNY) ++pozice_.cislo_tahu;
}

// --- position startpos|fen ... [moves ...] ----------------------------------
bool UciEngine::nastav_pozici(const std::vector<std::string>& slova, size_t od) {
    size_t i = od;
    if (i >= slova.size()) return false;

    if (slova[i] == "startpos") {
        nacti_uvodni_pozici(pozice_);
        ++i;
    } else if (slova[i] == "fen") {
        ++i;
        std::string fen;
        // FEN má šest polí, ale Light Red poslední dvě nepoužívá,
        // takže bereme, co přijde, až po "moves".
        while (i < slova.size() && slova[i] != "moves") {
            if (!fen.empty()) fen += ' ';
            fen += slova[i];
            ++i;
        }
        if (!nacti_fen(pozice_, fen)) return false;
    } else {
        return false;
    }

    if (i < slova.size() && slova[i] == "moves") {
        ++i;
        for (; i < slova.size(); ++i) {
            Tah t;
            if (!najdi_tah(slova[i], t)) return false;
            zahraj_a_prepni(t);
        }
    }
    return true;
}

// --- setoption name <jméno> value <hodnota> ---------------------------------
std::vector<std::string> UciEngine::nastav_volbu(const std::vector<std::string>& slova) {
    std::string jmeno, hodnota;
    size_t i = 1;
    if (i < slova.size() && slova[i] == "name") {
        ++i;
        while (i < slova.size() && slova[i] != "value") {
            if (!jmeno.empty()) jmeno += ' ';
            jmeno += slova[i];
            ++i;
        }
    }
    if (i < slova.size() && slova[i] == "value") {
        ++i;
        while (i < slova.size()) {
            if (!hodnota.empty()) hodnota += ' ';
            hodnota += slova[i];
            ++i;
        }
    }

    const std::string j = mala_pismena(jmeno);
    const std::string h = mala_pismena(hodnota);
    const bool ano = (h == "true" || h == "1" || h == "yes");

    if (j == "depth")               nast_.max_hloubka = std::max(1, std::atoi(hodnota.c_str()));
    else if (j == "quiescence")     nast_.max_hledej_klid = std::max(1, std::atoi(hodnota.c_str()));
    else if (j == "ownbook")        nast_.zahajovaci_preference = ano;
    else if (j == "originalkillerbug") nast_.puvodni_chyba_killeru = ano;
    else return {"info string neznama volba: " + jmeno};

    return {};
}

// --- go ---------------------------------------------------------------------
std::vector<std::string> UciEngine::proved_go(const std::vector<std::string>& slova) {
    NastaveniHledani n = nast_;
    n.cas_ms = 0;

    int wtime = 0, btime = 0, movetime = 0;
    bool nekonecno = false;

    for (size_t i = 1; i < slova.size(); ++i) {
        const std::string& s = slova[i];
        auto cislo = [&](int implicit) {
            return (i + 1 < slova.size()) ? std::atoi(slova[++i].c_str()) : implicit;
        };
        if      (s == "depth")    n.max_hloubka = std::max(1, cislo(n.max_hloubka));
        else if (s == "movetime") movetime = cislo(0);
        else if (s == "wtime")    wtime = cislo(0);
        else if (s == "btime")    btime = cislo(0);
        else if (s == "infinite") nekonecno = true;
    }

    if (n.max_hledej_klid < n.max_hloubka) n.max_hledej_klid = n.max_hloubka;

    if (movetime > 0) {
        n.cas_ms = movetime;
    } else if (!nekonecno) {
        const int zbyva = (pozice_.na_tahu_je == BILY) ? wtime : btime;
        if (zbyva > 0) n.cas_ms = std::max(20, zbyva / 30);
    }

    zastav_.store(false, std::memory_order_relaxed);
    n.zastav = &zastav_;

    // RedCry krok L1: materiál jednou, z pozice, kterou dostane hledej
    material_ = materialova_bilance(pozice_);
    prubeh_.vynuluj(material_);
    info_radky_.clear();
    n.hlaseni = &UciEngine::hlaseni_trampolina;
    n.hlaseni_kontext = this;

    const VysledekHledani v = hledej(pozice_, n);

    // jeden info řádek za každou dokončenou hloubku, jak je nasbírala
    // trampolína, a nakonec souhrn s časem
    std::vector<std::string> ven = info_radky_;

    std::ostringstream info;
    info << "info depth " << (v.dosazena_hloubka > 0 ? v.dosazena_hloubka : n.max_hloubka)
         << " score cp " << na_centipawny(v.hodnota, material_)
         << " nodes " << v.listu
         << " time " << static_cast<long long>(v.sekundy * 1000.0);
    if (v.tah_nalezen) info << " pv " << tah_na_text(v.nejlepsi_tah);
    ven.push_back(info.str());

    if (v.preruseno) ven.push_back("info string hledani preruseno");

    if (v.tah_nalezen) {
        ven.push_back("bestmove " + tah_na_text(v.nejlepsi_tah));
    } else {
        ven.push_back(std::string("info string ") +
                      (v.stav == MAT ? "mat" : (v.stav == PAT ? "pat" : "zadny tah")));
        ven.push_back("bestmove 0000");
    }
    return ven;
}

// --- rozcestník --------------------------------------------------------------
std::vector<std::string> UciEngine::prikaz(const std::string& radek) {
    const std::vector<std::string> slova = rozdel(radek);
    if (slova.empty()) return {};

    const std::string& c = slova[0];

    if (c == "uci") {
        return {
            std::string("id name ") + JMENO,
            std::string("id author ") + AUTOR,
            "option name Depth type spin default 4 min 1 max 12",
            "option name Quiescence type spin default 6 min 1 max 20",
            "option name OwnBook type check default true",
            "option name OriginalKillerBug type check default false",
            "uciok"
        };
    }

    if (c == "isready")   return {"readyok"};
    if (c == "ucinewgame") { nova_hra(); return {}; }
    if (c == "quit")      { skoncit_ = true; return {}; }
    if (c == "stop")      { zastav_hledani(); return {}; }
    if (c == "setoption") return nastav_volbu(slova);
    if (c == "go")        return proved_go(slova);

    if (c == "position") {
        if (!nastav_pozici(slova, 1)) return {"info string neplatne position"};
        return {};
    }

    // --- příkazy nad rámec UCI, pro UI ---

    if (c == "legalmoves") {
        Pozice q = pozice_;
        zjisti_vsechny_legalni_tahy(q);
        vymazej_tahy_kdy_je_v_sachu(q);
        std::string s;
        for (int i = 0; i < q.pocet_moznych_tahu; ++i) {
            if (!s.empty()) s += ' ';
            s += tah_na_text(q.tahy[i]);
        }
        return {s};
    }

    // Dvojtečka je oddělovač proto, že se nevyskytuje ani v zápisu UCI, ani
    // v SAN. Rovnítko ne - to je uvnitř zápisu proměny (e8=Q).
    if (c == "sanmoves") {
        std::string s;
        for (const auto& t : tahy_se_zapisem(pozice_)) {
            if (!s.empty()) s += ' ';
            s += t.first;
            s += ':';
            s += t.second;
        }
        return {s};
    }

    if (c == "applymove") {
        if (slova.size() < 2) return {"info string chybi tah"};
        Tah t;
        if (!najdi_tah(slova[1], t)) return {"info string nelegalni tah: " + slova[1]};
        zahraj_a_prepni(t);
        return {"ok"};
    }

    if (c == "d" || c == "board") {
        Pozice q = pozice_;
        zjisti_vsechny_legalni_tahy(q);
        vymazej_tahy_kdy_je_v_sachu(q);
        // Poradi je zavazne: (1) zadny legalni tah -> mat, nebo pat,
        // (2) jinak aspon 100 pultahu bez brani a tahu pescem -> remiza50,
        // (3) jinak normalni. Mat na stem pultahu je tedy mat (FIDE 5.1.1,
        // 9.6.2). Remiza50 je jen ohlaseni pro UI - hledani o pravidle nevi.
        const char* st = (q.stav == MAT) ? "mat"
                       : (q.stav == PAT) ? "pat"
                       : (pozice_.pultahy_bez_zmeny >= 100) ? "remiza50"
                       : "normalni";
        return {zapis_fen(pozice_), std::string("stav ") + st};
    }

    return {"info string neznamy prikaz: " + c};
}

}  // namespace lightred
