package cz.redcry.chess.ui

import cz.redcry.chess.engine.STAV_NORMALNI
import cz.redcry.chess.model.Move

// Zaznam partie v pameti: pozice, klice opakovani, SAN, tahy, index na desce,
// stav po poslednim tahu a nactena partie pro tlacitko ⟳. Vzniklo v kroku
// R2/C presunem z GameViewModel.kt. Ven jdou jen seznamy pro cteni; menit
// zaznam jde jen funkcemi teto tridy.

/**
 * Zaznam partie (krok R2/C). Motor sem nesmi: klic pozice a stav se ptaji
 * PRAVE aktualniho motoru az ve chvili, kdy je zaznam opravdu potrebuje -
 * stejne casto jako pred presunem.
 *
 * @param klicPozice  klic opakovani pozice, na ktere motor prave stoji
 *                    ([klicOpakovani] nad jeho legalnimi tahy)
 * @param stavMotoru  `engine.stav()` motoru, ktery prave stoji na konci zaznamu
 */
class ZaznamPartie(
    private val klicPozice: (String) -> String,
    private val stavMotoru: () -> String,
) {
    private val _history = ArrayList<String>()

    /** Pozice partie jako FEN; `history[i]` je pozice PO i tazich. */
    val history: List<String> get() = _history

    /**
     * Klic opakovani ke kazde pozici [history], stejny index (krok U4,
     * [klicOpakovani]). Pocita se JEN tam, kde pozice do historie pribyva
     * ([pridejPozici]) - motor na ni v tu chvili stoji a jeho mezipamet
     * legalnich tahu plati. Pri listovani v zaznamu se nepocita nic znovu,
     * takze se nikdy nepta motoru, ktery hleda nebo stoji jinde.
     */
    private val _klice = ArrayList<String>()
    val klice: List<String> get() = _klice

    /** FEN pozice, na ktere motor PRAVE stoji, do historie i s klicem. */
    private fun pridejPozici(fen: String) {
        _history.add(fen)
        _klice.add(klicPozice(fen))
    }

    /**
     * Zaznam partie v SAN. Pozor na tu jednicku: `history[i]` je pozice PO
     * i tazich, `sanHistory[i]` je zapis i-teho tahu, tedy toho, ktery vede
     * z `history[i]` do `history[i + 1]`. Pro N tahu ma `history` N+1 prvku
     * a `sanHistory` N.
     */
    private val _sanHistory = ArrayList<String>()
    val sanHistory: List<String> get() = _sanHistory

    /**
     * Tytez tahy jako [sanHistory], ale jako [Move] - `tahy[i]` vede
     * z `history[i]` do `history[i + 1]`. Orezavaji se spolu v [pushHistory].
     *
     * Kvuli GameViewModel.stepTo: pozice se motoru posila jako vychozi FEN
     * + tahy, aby RedMAX mel v hledani celou historii partie, viz
     * ChessEngine.setPosition.
     */
    private val _tahy = ArrayList<Move>()
    val tahy: List<Move> get() = _tahy

    /** Pozice na desce: `history[index]` (do kroku R2/C historyIndex). */
    var index = 0

    /**
     * Nactena partie, ke ktere se vraci tlacitko ⟳ (krok U6/B). Kopie vseho,
     * co partii v pameti tvori, ve chvili, kdy se dohrala z PGN nebo FEN -
     * soubor se znovu necte, pristup pres SAF uz nemusi platit.
     *
     * [klice] se kopiruji, ne pocitaji znovu: [klicOpakovani] se pta motoru
     * na legalni tahy pozice, na ktere motor stoji, a pri obnoveni stoji
     * jinde.
     */
    private class ReferencniPartie(
        val history: List<String>,
        val klice: List<String>,
        val sanHistory: List<String>,
        val tahy: List<Move>,
        val stavPoPoslednimTahu: String,
        /** Hlavicky PGN, jak prisly (u FEN prazdne). Bere z nich [GameViewModel.exportPgn]. */
        val tagy: Map<String, String>,
    )

    /**
     * Nastavuje ji [GameViewModel.nactiPgn] a [GameViewModel.importFen],
     * zahazuje GameViewModel.novaPartie (tedy i prepnuti motoru). Ulozeni
     * partie ji nemeni.
     */
    private var reference: ReferencniPartie? = null

    fun ulozReferenci(tagy: Map<String, String>) {
        reference = ReferencniPartie(
            _history.toList(), _klice.toList(), _sanHistory.toList(), _tahy.toList(),
            stavPoPoslednimTahu, tagy.toMap(),
        )
    }

    /** K nactene partii se uz nevraci (nova partie, krok U6/B). */
    fun zahodReferenci() {
        reference = null
    }

    /** Hlavicky PGN nactene partie, null = zadna neni. */
    val tagyReference: Map<String, String>? get() = reference?.tagy

    /** Tah odboceni od reference, null = neni reference nebo se neodchylila. */
    fun tahOdboceni(): Int? = reference?.let { tahOdboceni(_tahy, it.tahy) }

    /**
     * Vychozi FEN a tahy reference do pultahu [index] - co se posle motoru
     * PRED [obnovZReference]. Kdyz motor pozici odmitne, zaznam zustane.
     */
    fun poziceReference(index: Int): Pair<String, List<Move>>? =
        reference?.let { it.history[0] to it.tahy.take(index) }

    /**
     * Zaznam z reference, deska na pultahu [index] (tlacitko ⟳, krok U6/B).
     * Historie, SAN, tahy i klice opakovani se vezmou z reference.
     */
    fun obnovZReference(index: Int) {
        val ref = reference ?: return
        _history.clear()
        _history.addAll(ref.history)
        _klice.clear()
        _klice.addAll(ref.klice)
        _sanHistory.clear()
        _sanHistory.addAll(ref.sanHistory)
        _tahy.clear()
        _tahy.addAll(ref.tahy)
        this.index = index
        stavPoPoslednimTahu = ref.stavPoPoslednimTahu
    }

    /**
     * Stav partie na KONCI zaznamu, tedy po tahu `history.last()`.
     *
     * Bez nej by se vysledek partie nedal zjistit, kdyz uzivatel listuje
     * uprostred: `engine.stav()` plati vzdycky jen pro pozici, na ktere engine
     * prave stoji. Ukládá se proto ve chvili, kdy tah do zaznamu pribyde -
     * to je jedina chvile, kdy engine na konci zaznamu opravdu stoji.
     */
    var stavPoPoslednimTahu: String = STAV_NORMALNI
        private set

    /**
     * Zaznam od pozice [fen], na ktere motor PRAVE stoji: bez tahu, deska na
     * zacatku, stav po poslednim tahu z motoru. Referenci nemeni.
     */
    fun zacni(fen: String) {
        _history.clear()
        _klice.clear()
        pridejPozici(fen)
        _sanHistory.clear()
        _tahy.clear()
        index = 0
        stavPoPoslednimTahu = stavMotoru()
    }

    /**
     * Ureze obe historie za aktualnim indexem a prida novy tah na konec.
     *
     * Ty dve meze se lisi o jednicku, a je to zamerne: `history` ma o prvek
     * vic (vychozi pozici), takze se rezou na `index + 1` a
     * `index`. Po navratu pak plati, ze `sanHistory[index - 1]`
     * je tah, ktery vedl do `history[index]`.
     */
    fun pushHistory(fen: String, san: String, tah: Move) {
        // Krok U7/B: tentyz tah, ktery je v zaznamu dalsi, zaznam neurizne,
        // jen se po nem posune. Zbytek historie i klice zustanou, stejne tak
        // stavPoPoslednimTahu - konec zaznamu se nezmenil.
        if (jeDalsiVZaznamu(tah)) {
            index++
            return
        }
        while (_history.size > index + 1) _history.removeAt(_history.size - 1)
        while (_klice.size > index + 1) _klice.removeAt(_klice.size - 1)
        while (_sanHistory.size > index) _sanHistory.removeAt(_sanHistory.size - 1)
        while (_tahy.size > index) _tahy.removeAt(_tahy.size - 1)
        // Volajici prave zahral tah (applyMove), motor tedy stoji na fen.
        pridejPozici(fen)
        _sanHistory.add(san)
        _tahy.add(tah)
        index = _history.size - 1
        // Engine stoji prave na konci zaznamu, takze tenhle stav uz nikdo
        // pozdeji nezjisti - pri listovani se pozice enginu meni.
        stavPoPoslednimTahu = stavMotoru()
    }

    /**
     * [tah] je presne ten, ktery v zaznamu vede z `history[index]`
     * do `history[index + 1]` (krok U7/B). Porovnava se cely [Move]
     * vcetne promeny: podpromena neni tentyz tah jako promena na damu.
     */
    fun jeDalsiVZaznamu(tah: Move): Boolean = _tahy.getOrNull(index) == tah
}
