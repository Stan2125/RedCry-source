package cz.redcry.chess.ui

import android.content.Context

// Pamet zvoleneho jazyka na Androidu (krok J2). Jediny soubor jazyka, ktery
// zna android.*, takze GameViewModel zustava testovatelny v JVM.

/**
 * Volba jazyka v SharedPreferences "nastaveni", klic "jazyk", hodnota
 * [Jazyk.kod] ("cs" / "en"). apply() zapisuje na pozadi.
 */
class UlozisteJazykaAndroid(context: Context) : UlozisteJazyka {
    private val prefs = context.applicationContext.getSharedPreferences("nastaveni", Context.MODE_PRIVATE)

    override fun cti(): Jazyk? = Jazyk.zKodu(prefs.getString(KLIC, null))

    override fun uloz(jazyk: Jazyk) {
        prefs.edit().putString(KLIC, jazyk.kod).apply()
    }

    private companion object {
        const val KLIC = "jazyk"
    }
}
