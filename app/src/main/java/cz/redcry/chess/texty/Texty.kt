package cz.redcry.chess.texty

import cz.redcry.chess.engine.ChybaMotoru
import cz.redcry.chess.model.Side
import cz.redcry.chess.ui.PolozkaHry

// Vsechny texty, ktere uzivatel vidi nebo slysi (TalkBack), krok J1.
// Jeden jazyk = jeden objekt; cesky je [TextyCz], kde je u kazdeho textu
// i to, kde se ukazuje. Texty s cisly nebo tvary slov jsou funkce: tvar
// slova patri jazyku, ne volajicimu. Proc tabulka v Kotlinu a ne
// strings.xml: docs/POZNAMKY-JAZYK.md.

interface Texty {
    // --- Hlavni obrazovka: stavovy radek pod deskou -------------------------
    /** true: [naTahu] stoji za stranou ("White to move"), false: pred ni ("Na tahu bílý"). */
    val naTahuZaStranou: Boolean
    val naTahu: String
    val bily: String
    val cerny: String
    val hodnota: String
    val tah: String
    val konecMat: String
    val vyhravaBily: String
    val vyhravaCerny: String
    val konecPat: String
    val remiza: String
    val konecRemiza: String
    val pravidlo50Tahu: String
    val trojiOpakovani: String

    // --- Hlavni obrazovka: velke tlacitko a dlazdice ------------------------
    val premyslej: String
    val dobiha: String
    val hrajNejlepsi: String
    fun hrajNejlepsiSekundy(sekund: Int): String
    val nejlepsiTah: String
    val aktualniTah: String
    val hloubka: String
    val hloubkaKniha: String

    // --- Hlavni obrazovka: ovladani motoru ----------------------------------
    val engine: String
    val plnaSila: String
    val silaElo: String
    val maxHloubka: String
    val maxSekund: String
    val hra: String

    // --- Menu Hra -----------------------------------------------------------
    fun polozkaHry(polozka: PolozkaHry): String

    // --- Dialogy na hlavni obrazovce ----------------------------------------
    val zahajitNovouPartii: String
    fun potvrzeniNovePartie(pocetTahu: Int): String
    val novaPartie: String
    val zrusit: String
    val nastavit: String
    fun rozsahSekund(min: Int, max: Int): String
    val promena: String
    val dama: String
    val vez: String
    val strelec: String
    val jezdec: String

    // --- Okno (i) -----------------------------------------------------------
    val zahajeni: String
    val zaznamPartie: String
    val zatimZadnyTah: String
    val hodnoceni: String
    val varianta: String
    val predchoziVolba: String
    fun doHloubky(hloubka: Int): String
    fun zmenaPo(sekundy: String): String
    val poznamkaObnoveni: String
    val poznamkaElo: String
    val poznamkaSlabsiStockfish: String
    val poznamkaHodnoceni: String
    val poznamkaPredchoziVolba: String
    val poznamkaSoukromi: String
    val zavrit: String
    val eloMotor: String
    val eloElo: String
    val eloMereni: String
    val eloZmereno: String
    val eloPrepocet: String
    val eloUdajAutoru: String

    // --- Okno Analyza partie a pruh analyzy ---------------------------------
    val analyzaPartie: String
    val nedokoncenaAnalyza: String
    val motor: String
    val casNaPozici: String
    val casNaPoziciSekundy: String
    fun sekund(sekund: Int): String
    val tahy: String
    val odTahu: String
    val doTahu: String
    val slozkaProVysledek: String
    val slozkaNevybrana: String
    val zmenitSlozku: String
    val napovedaSlozky: String
    val vRozsahuNeniTah: String
    fun pokracovatOdTahu(tah: Int): String
    val spustit: String
    val zacitZnovu: String
    fun pocetPozic(pocet: Int): String
    fun dobaMinuty(minut: Int): String
    fun dobaHodiny(hodin: Int, minut: Int): String
    fun odhadAnalyzy(pozice: String, sekundNaPozici: Int, celkem: String): String
    fun zbyvaAsi(doba: String): String
    val zbyvaMeneNezMinuta: String
    fun pruhAnalyzy(tah: Int, zTahu: Int, zbyva: String): String
    val pruhPozastavena: String
    val prerusitAnalyzuDotaz: String
    val prerusitAnalyzuText: String
    val prerusit: String
    val pokracovat: String

    // --- Vystupy analyzy (PGN a CSV) ----------------------------------------
    fun annotator(jmenoMotoru: String, sekundNaPozici: Int): String

    /** Sloupce CSV analyzy v poradi (krok J3). */
    val sloupceCsv: List<String>

    /** Oddelovac bunek CSV: cesky Excel ';', anglicky ',' (krok J3). */
    val oddelovacCsv: Char

    /** Prvni radek CSV: [sloupceCsv] spojene [oddelovacCsv]. */
    val hlavickaCsv: String get() = sloupceCsv.joinToString(oddelovacCsv.toString())

    // --- Cisla (krok J3) ----------------------------------------------------
    /**
     * Desetinny oddelovac na obrazovce i v souborech analyzy: cesky ',',
     * anglicky '.'. Cisla se skladaji rucne, ne String.format - ten by vzal
     * oddelovac z Locale zarizeni, ne z jazyka aplikace.
     */
    val desetinnyOddelovac: Char

    // --- Hlasky (Snackbar) --------------------------------------------------
    val souborNeslPrecist: String
    val partieNactena: String
    val partieUlozena: String
    val souborNeslUlozit: String
    fun motorSeNacita(motor: String): String
    fun jeNaTahu(strana: Side): String
    val tamFiguraNemuze: String
    val neplatnyTah: String
    val motorNenaselTah: String
    val zadnyTahKonecPartie: String
    val engineVratilNeplatnyTah: String
    fun motorNeniKDispozici(motor: String): String
    fun motorSeNepodariloSpustit(motor: String): String
    val poziciNesloObnovit: String
    val poziciNesloNastavit: String
    val jsiNaZacatku: String
    val jsiNaPoslednimTahu: String
    val vychoziPoziciNesloNastavit: String
    fun tahNejdeZahrat(poradi: Int, token: String): String
    val enginePremysli: String
    val hledaniDobiha: String
    val starsiPozice: String
    val starsiPoziceHledani: String
    val knihovnaZapnuta: String
    val knihovnaVypnuta: String
    val knihovnaChyba: String
    fun nizsiHloubkaNejde(min: Int): String
    fun umiNejvysHloubku(motor: String, max: Int): String
    val meneNezBezOmezeni: String
    fun nejvysSekund(max: Int): String
    fun nizsiHloubkaNastaveno(hloubka: Int): String
    fun umiHloubkuNastaveno(motor: String, hloubka: Int): String
    val zapornyCasNastaveno: String
    fun nejvysSekundNastaveno(sekund: Int): String
    fun neniCislo(text: String): String
    fun umiEloNastaveno(motor: String, min: Int, max: Int, elo: Int?): String
    val analyzaBezi: String
    val partieNemaTah: String
    val nejdrivVyberSlozku: String
    val vZadanemRozsahuNeniTah: String
    fun analyzaPrerusenaChybou(zprava: String): String
    fun motorNesloSpustitAnalyza(motor: String): String
    val souboryNesloZalozit: String
    val analyzaPozicNesloNastavit: String
    val analyzaChybaZapisu: String
    fun souboryAnalyzy(nazev: String): String
    fun analyzaHotova(soubory: String): String
    val analyzaPrerusena: String
    fun analyzaPrerusenaNaTahu(tah: Int, soubory: String): String
    fun analyzaPrerusenaBaterie(procent: Int): String
    fun analyzaSeNespustila(zprava: String): String

    // --- Chyby motoru (krok J2; do J1 cesky primo v engine/) ---------------
    fun motorSeNespustilDo(motor: String, sekund: Int): String
    fun motorSeNeuvolnil(motor: String, sekund: Int): String
    fun motorUzBezi(motor: String): String

    // --- TalkBack (cteni obrazovky) -----------------------------------------
    val zavritHlasku: String
    val predchoziTahPopis: String
    val dalsiTahPopis: String
    val zpetKNactenePartii: String
    val zapnuto: String
    val jazyk: String
    val nazevCestiny: String
    val nazevAnglictiny: String
    val zaznamPartiePopis: String
    val prerusitAnalyzu: String
    val zadatCislo: String
}

/**
 * Veta pro chybu motoru (krok J2): [cz.redcry.chess.engine.EngineEvent.Chyba]
 * a [cz.redcry.chess.engine.MotorUzBeziException]. Nespustil je cesky
 * "Stockfish se nepodařilo spustit.", stejne jako do J1.
 */
fun Texty.textChybyMotoru(druh: ChybaMotoru, motor: String): String = when (druh) {
    ChybaMotoru.Nespustil -> motorNesloSpustitAnalyza(motor)
    is ChybaMotoru.NespustilDo -> motorSeNespustilDo(motor, druh.sekund)
    is ChybaMotoru.Neuvolnil -> motorSeNeuvolnil(motor, druh.sekund)
    ChybaMotoru.UzBezi -> motorUzBezi(motor)
}
