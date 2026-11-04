package cz.redcry.chess.ui

import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.Uri
import android.os.BatteryManager
import android.provider.DocumentsContract
import cz.redcry.chess.analyza.StavBaterie
import cz.redcry.chess.analyza.UlozisteAnalyzy
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

/**
 * Soubory analyzy pres SAF (krok A1/B). Slozka se voli jednou
 * (OpenDocumentTree), pristup k ni je trvaly (takePersistableUriPermission)
 * a jeji Uri lezi v SharedPreferences. Do manifestu kvuli tomu nepribylo
 * zadne opravneni, stejne jako u Nacist a Ulozit (POZNAMKY-PGN.md).
 *
 * Drzi jen applicationContext: ViewModel ji prezije spolu s Activity.
 */
class UlozisteAnalyzyAndroid(context: Context) : UlozisteAnalyzy {

    private val app = context.applicationContext
    private val prefs = app.getSharedPreferences("analyza", Context.MODE_PRIVATE)
    private val souborPrubehu = File(app.filesDir, "analyza-prubeh.txt")

    private val slozka: Uri? get() = prefs.getString(KLIC_SLOZKA, null)?.let(Uri::parse)

    /** Vysledek OpenDocumentTree: trvaly pristup a zapamatovat. */
    fun nastavSlozku(strom: Uri) {
        val prava = Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION
        val stara = slozka
        runCatching { app.contentResolver.takePersistableUriPermission(strom, prava) }
        // Stara slozka uz pristup nepotrebuje; system jich drzi omezeny pocet.
        if (stara != null && stara != strom) {
            runCatching { app.contentResolver.releasePersistableUriPermission(stara, prava) }
        }
        prefs.edit().putString(KLIC_SLOZKA, strom.toString()).apply()
    }

    private fun dokumentSlozky(strom: Uri): Uri =
        DocumentsContract.buildDocumentUriUsingTree(strom, DocumentsContract.getTreeDocumentId(strom))

    override suspend fun jmenoSlozky(): String? = withContext(Dispatchers.IO) {
        val strom = slozka ?: return@withContext null
        val pristup = app.contentResolver.persistedUriPermissions.any {
            it.uri == strom && it.isReadPermission && it.isWritePermission
        }
        if (!pristup) return@withContext null
        runCatching {
            app.contentResolver.query(
                dokumentSlozky(strom),
                arrayOf(DocumentsContract.Document.COLUMN_DISPLAY_NAME),
                null,
                null,
                null,
            )?.use { if (it.moveToFirst()) it.getString(0) else null }
        }.getOrNull()
    }

    override suspend fun zalozSoubory(nazev: String): Pair<String, String>? = withContext(Dispatchers.IO) {
        val strom = slozka ?: return@withContext null
        runCatching {
            val rodic = dokumentSlozky(strom)
            val cr = app.contentResolver
            val pgn = DocumentsContract.createDocument(cr, rodic, TYP_PGN, "$nazev.pgn")
            val csv = DocumentsContract.createDocument(cr, rodic, "text/csv", "$nazev.csv")
            if (pgn == null || csv == null) null else pgn.toString() to csv.toString()
        }.getOrNull()
    }

    override suspend fun zapis(uriPgn: String, uriCsv: String, pgn: String, csv: String): Boolean =
        withContext(Dispatchers.IO) { prepis(uriPgn, pgn) && prepis(uriCsv, csv) }

    /** "wt" = zkratit, jinak by po kratsim obsahu zustal ocasek stareho. */
    private fun prepis(uri: String, text: String): Boolean = runCatching {
        val proud = app.contentResolver.openOutputStream(Uri.parse(uri), "wt")
            ?: error("outputStream je null")
        proud.use { it.write(text.toByteArray(Charsets.UTF_8)) }
    }.isSuccess

    override suspend fun ulozPrubeh(text: String): Boolean = withContext(Dispatchers.IO) {
        runCatching { souborPrubehu.writeText(text, Charsets.UTF_8) }.isSuccess
    }

    override suspend fun ctiPrubeh(): String? = withContext(Dispatchers.IO) {
        runCatching { souborPrubehu.takeIf { it.exists() }?.readText(Charsets.UTF_8) }.getOrNull()
    }

    override suspend fun smazPrubeh() {
        withContext(Dispatchers.IO) { runCatching { souborPrubehu.delete() } }
    }

    /** Lepkavy ACTION_BATTERY_CHANGED, bez registrace prijemce. */
    override fun baterie(): StavBaterie? {
        val i = app.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED)) ?: return null
        val level = i.getIntExtra(BatteryManager.EXTRA_LEVEL, -1)
        val scale = i.getIntExtra(BatteryManager.EXTRA_SCALE, -1)
        if (level < 0 || scale <= 0) return null
        val zapojeno = i.getIntExtra(BatteryManager.EXTRA_PLUGGED, 0) != 0
        return StavBaterie(level * 100 / scale, zapojeno)
    }

    private companion object {
        const val KLIC_SLOZKA = "slozka"
    }
}
