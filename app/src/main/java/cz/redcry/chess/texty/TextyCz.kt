package cz.redcry.chess.texty

import cz.redcry.chess.model.Side
import cz.redcry.chess.ui.PolozkaHry

// Ceske texty aplikace (krok J1). Zaroven seznam textu k prekladu: u kazdeho
// je, kde se ukazuje, kdyz to neni zrejme ze jmena. Texty jsou opsane
// beze zmeny z mist, kde byly do kroku J1.
//
// Nepatri sem: SAN (K Q R B N), nazvy zahajeni (Lichess), jmena motoru,
// znacky PGN, nazvy souboru. Desetinna carka je od kroku J3 tady
// ([desetinnyOddelovac]), cislo se sklada v Hodnoceni.kt.

object TextyCz : Texty {
    // --- Hlavni obrazovka: stavovy radek pod deskou -------------------------

    /** Popisek pred stranou: "Na tahu bílý" (krok J2, anglicky obracene). */
    override val naTahuZaStranou = false
    /** Vlevo pod deskou: "Na tahu bílý". */
    override val naTahu = "Na tahu"
    /** Strana na tahu ve stavovem radku a sloupec Strana v CSV analyzy. */
    override val bily = "bílý"
    override val cerny = "černý"
    /** Uprostred pod deskou, za nim skore ("Hodnota +0,24"). */
    override val hodnota = "Hodnota"
    /** Vpravo pod deskou, za nim cislo tahu. */
    override val tah = "Tah"
    /** Konec partie misto "Na tahu": "Mat — vyhrává bílý". */
    override val konecMat = "Mat —"
    override val vyhravaBily = "vyhrává bílý"
    override val vyhravaCerny = "vyhrává černý"
    /** "Pat — remíza". */
    override val konecPat = "Pat —"
    override val remiza = "remíza"
    /** "Remíza — pravidlo 50 tahů" / "Remíza — trojí opakování pozice". */
    override val konecRemiza = "Remíza —"
    override val pravidlo50Tahu = "pravidlo 50 tahů"
    override val trojiOpakovani = "trojí opakování pozice"

    // --- Hlavni obrazovka: velke tlacitko a dlazdice ------------------------

    override val premyslej = "Přemýšlej"
    /** Tlacitko po stisku Hraj nejlepší, nez motor vrati tah. */
    override val dobiha = "Dobíhá…"
    /** Fialove tlacitko za hledani, nez je znamy cas. */
    override val hrajNejlepsi = "Hraj nejlepší"
    /** Fialove tlacitko za hledani s uplynulym casem. */
    override fun hrajNejlepsiSekundy(sekund: Int) = "Hraj nejlepší (" + sekund + " s)"
    /** Leva dlazdice za hledani. */
    override val nejlepsiTah = "Nejlepší tah"
    /** Leva dlazdice mimo hledani: naposledy zahrany tah. */
    override val aktualniTah = "Aktuální tah"
    /** Prava dlazdice a radek v okne (i). */
    override val hloubka = "Hloubka"
    /** Hodnota hloubky po tahu z knihovny zahajeni (dlazdice i okno (i)). */
    override val hloubkaKniha = "kniha"

    // --- Hlavni obrazovka: ovladani motoru ----------------------------------

    /** Popisek vyberu motoru. */
    override val engine = "Motor"
    /** Prepinac u Stockfishe. */
    override val plnaSila = "Plná síla"
    /** Radek s Elo u Stockfishe a nadpis dialogu zadani Elo. */
    override val silaElo = "Síla ELO"
    /** Nad krokovacem hloubky a nadpis dialogu zadani. */
    override val maxHloubka = "Max hloubka"
    /** Nad krokovacem sekund a nadpis dialogu zadani. */
    override val maxSekund = "Max sekund"
    /** Tlacitko s menu vpravo dole. */
    override val hra = "Hra"

    // --- Menu Hra -----------------------------------------------------------

    override fun polozkaHry(polozka: PolozkaHry) = when (polozka) {
        PolozkaHry.NOVA_PARTIE -> "Nová partie…"
        PolozkaHry.OTOCIT -> "Otočit desku"
        PolozkaHry.KNIHOVNA -> "Knihovna zahájení"
        PolozkaHry.ANALYZA -> "Analýza partie…"
        PolozkaHry.NACIST -> "Načíst partii"
        PolozkaHry.ULOZIT -> "Uložit partii"
    }

    // --- Dialogy na hlavni obrazovce ----------------------------------------

    /** Nadpis dialogu pred novou partii. */
    override val zahajitNovouPartii = "Zahájit novou partii?"

    /**
     * Text dialogu pred novou partii. Sklonovani podle cisla, at nestoji
     * "(1 tahů)".
     */
    override fun potvrzeniNovePartie(pocetTahu: Int): String {
        val tahu = when {
            pocetTahu == 1 -> "1 tah"
            pocetTahu in 2..4 -> "" + pocetTahu + " tahy"
            else -> "" + pocetTahu + " tahů"
        }
        return "Rozehraná partie (" + tahu + ") se zahodí. Pokud ji chceš mít, nejdřív ji ulož."
    }

    /** Potvrzovaci tlacitko dialogu pred novou partii. */
    override val novaPartie = "Nová partie"
    /** Tlacitko ve vsech dialozich, ktere nic neudela. */
    override val zrusit = "Zrušit"
    /** Potvrzeni v dialogu zadani cisla. */
    override val nastavit = "Nastavit"
    /** Napoveda v poli dialogu Max sekund. */
    override fun rozsahSekund(min: Int, max: Int) = "" + min + "–" + max + ", 0 = bez omezení"
    /** Nadpis dialogu vyberu figury pri promene. */
    override val promena = "Proměna"

    /** Figury v dialogu promeny (TalkBack), v poradi D, V, S, J. */
    override val dama = "Dáma"
    override val vez = "Věž"
    override val strelec = "Střelec"
    override val jezdec = "Jezdec"

    // --- Okno (i) -----------------------------------------------------------

    /** Prvni radek okna, za nim ECO a nazev. */
    override val zahajeni = "Zahájení"
    /** Nadpis levého sloupce okna. */
    override val zaznamPartie = "Záznam partie"
    /** Misto zaznamu, kdyz partie nema tah. */
    override val zatimZadnyTah = "Zatím žádný tah."
    override val hodnoceni = "Hodnocení"
    override val varianta = "Varianta"
    override val predchoziVolba = "Předchozí volba"
    /** Pod predchozi volbou. */
    override fun doHloubky(hloubka: Int) = "do hloubky " + hloubka
    /** Pod predchozi volbou; [sekundy] uz s desetinnou carkou ("0,3"). */
    override fun zmenaPo(sekundy: String) = "změna po " + sekundy + " s"
    override val poznamkaObnoveni =
        "⟳ vrátí načtenou partii a postaví se na tah, kde se od ní " +
            "rozehraná partie odchýlila."
    override val poznamkaElo =
        "Síla ELO je přibližná, podle žebříčku počítačových programů " +
            "CCRL (bleskové partie) – nejde o lidské Elo FIDE."
    /** Pod tabulkou Elo. */
    override val poznamkaSlabsiStockfish =
        "Nastavené Elo hraje Stockfish v aplikaci asi o 200 slabší " +
            "(měřeno při Elo 2650)."
    override val poznamkaHodnoceni =
        "Kladná čísla hodnocení jsou ve prospěch bílého, záporná ve " +
            "prospěch černého. +M3 znamená mat ve třech tazích pro " +
            "bílého, -M3 pro černého."
    override val poznamkaPredchoziVolba =
        "Předchozí volba je tah, který motor považoval za nejlepší, než " +
            "ve větší hloubce našel lepší. Nemusí to být objektivně druhý " +
            "nejlepší tah."
    /** Posledni poznamka v okne (krok P1, zasady Google Play). */
    override val poznamkaSoukromi =
        "Ochrana soukromí: RedCry nesbírá, neodesílá ani nesdílí žádné " +
            "osobní údaje. Nemá přístup k internetu, reklamy ani analytiku. " +
            "Partie a analýzy ukládá do souborů, které sám vybereš, " +
            "nastavení a průběh analýzy si drží v telefonu. Celé znění: " +
            "github.com/Stan2125/RedCry-source/blob/main/PRIVACY.md"
    /** Tlacitko dole v okne. */
    override val zavrit = "Zavřít"
    /** Hlavicka tabulky Elo motoru: tri sloupce. */
    override val eloMotor = "Motor"
    override val eloElo = "Elo"
    override val eloMereni = "5 s na tah (emulátor)"
    /** Treti sloupec tabulky Elo u LightRedu a RedMaxu. */
    override val eloZmereno = "změřeno"
    /** Treti sloupec tabulky Elo u Crafty. */
    override val eloPrepocet = "přepočet, neměřeno"
    /** Treti sloupec tabulky Elo u Stockfishe. */
    override val eloUdajAutoru = "údaj autorů Stockfishe"

    // --- Okno Analyza partie a pruh analyzy ---------------------------------

    /** Nadpis okna. */
    override val analyzaPartie = "Analýza partie"
    /** Okno, kdyz je rozpracovana analyza teze partie. */
    override val nedokoncenaAnalyza = "Tahle partie má nedokončenou analýzu."
    /** Popisek vyberu motoru a radku pokracovani. */
    override val motor = "Motor"
    /** Radek pokracovani. */
    override val casNaPozici = "Čas na pozici"
    /** Krokovac v okne. */
    override val casNaPoziciSekundy = "Čas na pozici (s)"
    /** Hodnota casu v radku pokracovani: "5 s". */
    override fun sekund(sekund: Int) = "" + sekund + " s"
    /** Radek pokracovani, za nim rozsah tahu. */
    override val tahy = "Tahy"
    override val odTahu = "Od tahu"
    override val doTahu = "Do tahu"
    override val slozkaProVysledek = "Složka pro výsledek"
    override val slozkaNevybrana = "nevybrána"
    /** Tlacitko vedle slozky. */
    override val zmenitSlozku = "Změnit…"
    override val napovedaSlozky =
        "Vyber podsložku, např. Documents/RedCry. Kořen úložiště ani " +
            "samotnou složku Download Android nedovolí."
    /** Misto odhadu doby, kdyz rozsah nema tah. */
    override val vRozsahuNeniTah = "V rozsahu není žádný tah."
    /** Tlacitko pokracovani rozpracovane analyzy. */
    override fun pokracovatOdTahu(tah: Int) = "Pokračovat od tahu $tah"
    override val spustit = "Spustit"
    override val zacitZnovu = "Začít znovu"
    /** Odhad doby: "1 pozice", "3 pozice", "81 pozic". */
    override fun pocetPozic(pocet: Int) = pocet.toString() + " " + (if (pocet in 1..4) "pozice" else "pozic")
    /** Doba v odhadu a v pruhu: "7 min". */
    override fun dobaMinuty(minut: Int) = "$minut min"
    /** Doba v odhadu a v pruhu: "2 h 5 min", "1 h". */
    override fun dobaHodiny(hodin: Int, minut: Int) = if (minut == 0) "$hodin h" else "$hodin h $minut min"
    /** Odhad v okne: "81 pozic × 5 s ≈ 7 min". */
    override fun odhadAnalyzy(pozice: String, sekundNaPozici: Int, celkem: String) =
        pozice + " × " + sekundNaPozici + " s ≈ " + celkem
    /** Konec pruhu: "zbývá asi 3 min". */
    override fun zbyvaAsi(doba: String) = "zbývá asi $doba"
    override val zbyvaMeneNezMinuta = "zbývá méně než minuta"
    /** Pruh dole za analyzy: "Probíhá analýza partie – tah 12/40, zbývá asi 3 min". */
    override fun pruhAnalyzy(tah: Int, zTahu: Int, zbyva: String) =
        "Probíhá analýza partie – tah $tah/$zTahu, " + zbyva
    /** Pruh dole, kdyz analyza stoji (aplikace na pozadi). */
    override val pruhPozastavena = "Analýza pozastavena"
    /** Dialog po klepnuti na pruh. */
    override val prerusitAnalyzuDotaz = "Přerušit analýzu?"
    override val prerusitAnalyzuText = "Hotové pozice zůstanou uložené a analýzu půjde později dokončit."
    override val prerusit = "Přerušit"
    override val pokracovat = "Pokračovat"

    // --- Vystupy analyzy (PGN a CSV) ----------------------------------------

    /** Hodnota znacky Annotator v PGN analyzy. */
    override fun annotator(jmenoMotoru: String, sekundNaPozici: Int) =
        "RedCry: $jmenoMotoru, $sekundNaPozici s/pozice"
    /**
     * Sloupce CSV analyzy presne podle zadani A1. Hlavicka z nich
     * ([Texty.hlavickaCsv]) je "Tah;Strana;Zahraný tah;Hodnota;Mat;Hloubka;
     * Tah motoru;Varianta;Ztráta", stejne jako do J3.
     */
    override val sloupceCsv = listOf(
        "Tah", "Strana", "Zahraný tah", "Hodnota", "Mat", "Hloubka", "Tah motoru", "Varianta", "Ztráta",
    )
    /** Cesky Excel cte strednik. */
    override val oddelovacCsv = ';'

    // --- Cisla (krok J3) ----------------------------------------------------

    /** "+0,35", "změna po 2,7 s", Hodnota a Ztráta v CSV. */
    override val desetinnyOddelovac = ','

    // --- Hlasky (Snackbar) --------------------------------------------------

    /** Nacitani partie: soubor nejde precist. */
    override val souborNeslPrecist = "Soubor se nepodařilo přečíst."
    override val partieNactena = "Partie načtena."
    override val partieUlozena = "Partie uložena."
    override val souborNeslUlozit = "Soubor se nepodařilo uložit."
    /** Stisk za zakladani motoru (Stockfish). */
    override fun motorSeNacita(motor: String) = motor + " se ještě načítá, chvilku počkej."
    /** Klepnuti na figuru strany, ktera neni na tahu. */
    override fun jeNaTahu(strana: Side) = (if (strana == Side.WHITE) "Bílý" else "Černý") + " je na tahu."
    override val tamFiguraNemuze = "Tam ta figura nemůže."
    override val neplatnyTah = "Neplatný tah."
    /** Hraj nejlepší driv, nez motor dokoncil prvni hloubku. */
    override val motorNenaselTah = "Motor ještě nenašel tah, zkus to znovu."
    override val zadnyTahKonecPartie = "Žádný tah — konec partie."
    override val engineVratilNeplatnyTah = "Engine vrátil neplatný tah."
    override fun motorNeniKDispozici(motor: String) = "Motor " + motor + " zatím není k dispozici."
    /** Prepnuti motoru, ktery nesel zalozit (bez zpravy od mostu). */
    override fun motorSeNepodariloSpustit(motor: String) = "Motor " + motor + " se nepodařilo spustit."
    override val poziciNesloObnovit = "Pozici se nepodařilo obnovit."
    override val poziciNesloNastavit = "Pozici se nepodařilo nastavit."
    /** Sipka ‹ na zacatku partie. */
    override val jsiNaZacatku = "Jsi na začátku partie."
    /** Sipka › na konci partie. */
    override val jsiNaPoslednimTahu = "Jsi na posledním tahu."
    /** Nacitani PGN se znackou FEN, kterou motor odmitne. */
    override val vychoziPoziciNesloNastavit = "Výchozí pozici partie se nepodařilo nastavit."
    /** Nacitani PGN zastavene na tahu, ktery nejde zahrat. */
    override fun tahNejdeZahrat(poradi: Int, token: String) = "Tah " + poradi + " (" + token + ") nejde zahrát."
    /**
     * Zmena pozice nebo nastaveni za hledani. Dokud hledani bezi, nesmi
     * ViewModel sahnout na pozici enginu - nativni strana zmenu stejne
     * odmitne a stav UI by se s enginem rozesel. Kazda takova cesta to
     * rekne timhle textem.
     */
    override val enginePremysli = "Engine přemýšlí — počkej, nebo stiskni Hraj nejlepší."
    /**
     * Totez po stisku Hraj nejlepší, nez motor vrati tah: nativni strana
     * jeste hleda. "Nejdriv zastav" by tady radilo neco, co uzivatel uz
     * udelal.
     */
    override val hledaniDobiha = "Předchozí hledání ještě dobíhá."
    /**
     * Tah ze starsi pozice zahodi vsechno za aktualnim indexem. Rekne se,
     * az kdyz tah opravdu padne a neni to tah, ktery je v zaznamu dalsi
     * (krok U7/B). Premyslej ma od U7/B [starsiPoziceHledani].
     */
    override val starsiPozice = "Hraješ ze starší pozice — pozdější tahy se zahodí."
    /**
     * Premyslej ze starsi pozice (krok U7/B): jestli se neco urizne,
     * zalezi na tom, jaky tah motor najde.
     */
    override val starsiPoziceHledani =
        "Hledáš ze starší pozice — jiný tah, než je v záznamu, pozdější tahy zahodí."
    /** Prepnuti v menu Hra. */
    override val knihovnaZapnuta = "Knihovna zahájení zapnuta."
    override val knihovnaVypnuta = "Knihovna zahájení vypnuta."
    /** Knihovna z assets nesla nacist (krok K1/B); hraje se bez ni. */
    override val knihovnaChyba = "Knihovnu zahájení se nepodařilo načíst."
    /** Krokovac hloubky na spodnim kraji. */
    override fun nizsiHloubkaNejde(min: Int) = "Nižší hloubka než " + min + " nejde."
    /** Krokovac hloubky na hornim kraji. */
    override fun umiNejvysHloubku(motor: String, max: Int) = motor + " umí nejvýš hloubku " + max + "."
    /** Krokovac sekund na spodnim kraji. */
    override val meneNezBezOmezeni = "Méně než bez omezení už nejde."
    /** Krokovac sekund na hornim kraji. */
    override fun nejvysSekund(max: Int) = "Nejvýš " + max + " sekund na tah."
    /** Zadani hloubky cislem pod mezi. */
    override fun nizsiHloubkaNastaveno(hloubka: Int) =
        "Nižší hloubka než " + hloubka + " nejde — nastaveno " + hloubka + "."
    /** Zadani hloubky cislem nad mezi. */
    override fun umiHloubkuNastaveno(motor: String, hloubka: Int) =
        motor + " umí nejvýš hloubku " + hloubka + " — nastaveno " + hloubka + "."
    /** Zadani zaporneho poctu sekund. */
    override val zapornyCasNastaveno = "Záporný čas nejde — nastaveno bez omezení."
    /** Zadani sekund cislem nad mezi. */
    override fun nejvysSekundNastaveno(sekund: Int) =
        "Nejvýš " + sekund + " sekund na tah — nastaveno " + sekund + "."
    /** Zadani necisla v dialogu. */
    override fun neniCislo(text: String) = "„" + text.trim() + "“ není číslo — nic se nezměnilo."
    /** Zadani Elo mimo meze. */
    override fun umiEloNastaveno(motor: String, min: Int, max: Int, elo: Int?) =
        motor + " umí Elo " + min + "–" + max + " — nastaveno " + elo + "."
    /** Za analyzy - vsechny straze a zamcene ovladani. */
    override val analyzaBezi = "Probíhá analýza partie."
    /** Analyza partie bez tahu. */
    override val partieNemaTah = "Partie nemá žádný tah."
    override val nejdrivVyberSlozku = "Nejdřív vyber složku pro výsledek."
    override val vZadanemRozsahuNeniTah = "V zadaném rozsahu není žádný tah."
    /** Chyba motoru za analyzy, [zprava] od motoru. */
    override fun analyzaPrerusenaChybou(zprava: String) = "Analýza přerušena: $zprava"
    /**
     * Duvod, proc se analyza nespustila: motor nenabehl. Od J2 i hlaska
     * [cz.redcry.chess.engine.ChybaMotoru.Nespustil] ("Stockfish se nepodařilo spustit.").
     */
    override fun motorNesloSpustitAnalyza(motor: String) = motor + " se nepodařilo spustit."
    /** Duvod, proc se analyza nespustila: soubory. */
    override val souboryNesloZalozit = "ve složce se nepodařilo založit soubory."
    override val analyzaPozicNesloNastavit = "Analýza přerušena: pozici se nepodařilo nastavit."
    override val analyzaChybaZapisu = "Analýza přerušena: soubory se nepodařilo zapsat."
    /** Oba vystupy analyzy v hlasce: "redcry-analyza-….pgn a .csv". */
    override fun souboryAnalyzy(nazev: String) = nazev + ".pgn a .csv"
    override fun analyzaHotova(soubory: String) = "Analýza hotová: $soubory"
    /** Prerusena drive, nez se zalozily soubory. */
    override val analyzaPrerusena = "Analýza přerušena."
    override fun analyzaPrerusenaNaTahu(tah: Int, soubory: String) =
        "Analýza přerušena na tahu " + tah + ", uloženo: " + soubory
    override fun analyzaPrerusenaBaterie(procent: Int) = "Analýza přerušena: slabá baterie ($procent %)."
    /** [zprava] je jeden z duvodu vyse, nebo chyba motoru pri prepnuti. */
    override fun analyzaSeNespustila(zprava: String) = "Analýza se nespustila: " + zprava

    // --- Chyby motoru (krok J2; do J1 cesky primo v engine/) ---------------

    /** StockfishEngine: zalozeni nedobehlo do [sekund] s. */
    override fun motorSeNespustilDo(motor: String, sekund: Int) = motor + " se nespustil do " + sekund + " s."
    /** StockfishEngine: predchozi instance se neuvolnila do [sekund] s. */
    override fun motorSeNeuvolnil(motor: String, sekund: Int) =
        motor + " se nepodařilo spustit: předchozí instance se neuvolnila do " + sekund + " s."
    /** StockfishBridge, CraftyBridge: nativni instance uz existuje. */
    override fun motorUzBezi(motor: String) =
        motor + " už běží v jiné instanci (nebo došla paměť) - dva motory " + motor + " současně nejdou"

    // --- TalkBack (cteni obrazovky) -----------------------------------------

    /** Akce klepnuti na hlasku dole. */
    override val zavritHlasku = "Zavřít hlášku"
    /** Tlacitko ‹. */
    override val predchoziTahPopis = "Předchozí tah"
    /** Tlacitko ›. */
    override val dalsiTahPopis = "Další tah"
    /** Tlacitko ⟳. */
    override val zpetKNactenePartii = "Zpět k načtené partii"
    /** Fajfka u Knihovna zahájení v menu Hra. */
    override val zapnuto = "zapnuto"
    /** Prepinac jazyka. */
    override val jazyk = "Jazyk"
    /** Stav prepinace jazyka. */
    override val nazevCestiny = "čeština"
    override val nazevAnglictiny = "angličtina"
    /** Tlacitko (i). */
    override val zaznamPartiePopis = "Záznam partie"
    /** Akce klepnuti na pruh analyzy. */
    override val prerusitAnalyzu = "Přerušit analýzu"
    /** Akce klepnuti na cislo v krokovaci okna analyzy. */
    override val zadatCislo = "Zadat číslo"
}
