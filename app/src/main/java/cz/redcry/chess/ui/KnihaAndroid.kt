package cz.redcry.chess.ui

import android.content.res.AssetManager
import android.util.Log
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.ViewModelProvider.AndroidViewModelFactory.Companion.APPLICATION_KEY
import androidx.lifecycle.viewmodel.initializer
import androidx.lifecycle.viewmodel.viewModelFactory
import cz.redcry.chess.kniha.KnihaZahajeni
import java.util.Locale

// Napojeni knihovny zahajeni na Android (krok K1/B): cteni z assets
// a vyroba GameViewModelu s ni. Jediny soubor knihovny, ktery zna android.*,
// takze GameViewModel i KnihaZahajeni zustavaji testovatelne v JVM.

/**
 * Nacte assets/kniha/kniha.txt a nazvy.txt (UTF-8). Vola se z
 * [GameViewModel] na Dispatchers.IO, nikdy na hlavnim vlakne. Dobu
 * nacteni zapise do logcatu pod znackou "RedCryKniha" - mereni pro
 * docs/POZNAMKY-KNIHA.md.
 */
internal fun nactiKnihuZAssets(assets: AssetManager): KnihaZahajeni {
    val zacatek = System.nanoTime()
    val kniha = assets.open("kniha/kniha.txt").reader(Charsets.UTF_8).use { tahy ->
        assets.open("kniha/nazvy.txt").reader(Charsets.UTF_8).use { nazvy ->
            KnihaZahajeni.nacti(tahy, nazvy)
        }
    }
    Log.i("RedCryKniha", "nacteno za " + (System.nanoTime() - zacatek) / 1_000_000 + " ms")
    return kniha
}

/**
 * Vyroba [GameViewModel] v aplikaci: s knihovnou zahajeni z assets a od
 * kroku J2 s pameti jazyka.
 * Bez factory by viewModel() pouzil konstruktor bez argumentu, ktery
 * kontext aplikace nema.
 */
internal val GameViewModelFactory: ViewModelProvider.Factory = viewModelFactory {
    initializer {
        val aplikace = this[APPLICATION_KEY]!!
        val assets = aplikace.assets
        GameViewModel(
            zdrojKnihy = { nactiKnihuZAssets(assets) },
            // Krok J2: pamet volby jazyka a jazyk telefonu pro prvni start.
            ulozisteJazyka = UlozisteJazykaAndroid(aplikace),
            jazykTelefonu = Locale.getDefault().language,
        )
    }
}
