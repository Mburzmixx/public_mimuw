package loteria;

import wyjątki.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.TreeSet;

public class Kolektura {
    private final CentralaTotolotka centrala;
    private final int numerPorządkowy;
    HashMap<String, Kupon> SprzedaneKupony = new HashMap<>();

    private final static long CENA_KUPONU_BRUTTO = 3_00;
    private final static int DOMYŚLNY_ROZMIAR_TABLIC = 5;
    private final static int MAX_LICZBA_ZAKŁADÓW = 8;
    public final static int MAX_LICZBA_LOSOWAŃ = 10;


    public Kolektura(CentralaTotolotka centrala) {
        this.centrala = centrala;
        this.numerPorządkowy = this.centrala.autoryzujNowąKolekturę(this);
    }

    public static long wyceńKupon(int liczbaZakładów, int liczbaLosowań) throws NiepoprawnyArgument {
        if (liczbaZakładów <= 0 || liczbaZakładów > MAX_LICZBA_ZAKŁADÓW) {
            throw new NiepoprawnyArgument("Nie sprzedajemy takich kuponów - zła liczba zakładów.");
        }

        if (liczbaLosowań <= 0 || liczbaLosowań > MAX_LICZBA_LOSOWAŃ) {
            throw new NiepoprawnyArgument("Nie sprzedajemy takich kuponów - zła liczba losowań.");
        }

        return CENA_KUPONU_BRUTTO * liczbaZakładów * liczbaLosowań;
    }

    Kupon sprzedajKupon(Blankiet blankiet, Gracz gracz) throws ŹleWypełnionyBlankiet {
        long cenaKuponu = blankiet.wyceńBlankiet();

        if (!gracz.czyMaConajmniej(cenaKuponu)) {
            throw new BankructwoGracza("Stało się to prz kupowaniu kuponu z blankietu.");
        }

        ArrayList<TreeSet<Integer>> WytypowaneLiczby = blankiet.dajWytypowaneLiczby();

        if (WytypowaneLiczby == null) {
            throw new ŹleWypełnionyBlankiet("Kupowanie kuponu - nie zaznaczono poprawnie nawet jednego zakładu.");
        }

        gracz.wydaj(cenaKuponu);
        int pierwszeLosowanie = this.centrala.dajNumerNastępnegoLosowania();
        int ostatnieLosowanie = pierwszeLosowanie + blankiet.dajLiczbęLosowań() - 1;

        Kupon kupon = new Kupon(this, WytypowaneLiczby, this.centrala.dajNumerPorządkowyKuponów(),
                pierwszeLosowanie, blankiet.dajLiczbęLosowań());

        // autoryzacja kuponu
        this.SprzedaneKupony.put(kupon.dajIdentyfikatorKuponu(), kupon);
        this.centrala.zwiększNumerPorządkowyKuponów();

        long piątaCzęśćKosztu = cenaKuponu / 5;

        this.centrala.odbierzZysk(piątaCzęśćKosztu * 4, pierwszeLosowanie, ostatnieLosowanie);
        this.centrala.odprowadźPodatek(piątaCzęśćKosztu);

        return kupon;
    }

    public Kupon sprzedajKupon(int liczbaZakładów, int liczbaLosowań, Gracz gracz) throws NiepoprawnyArgument {
        long cenaKuponu = Kolektura.wyceńKupon(liczbaZakładów, liczbaLosowań);

        if (!gracz.czyMaConajmniej(cenaKuponu)) {
            throw new BankructwoGracza("Stało się to przy kupowaniu kuponu na 'chybił trafił'.");
        }

        gracz.wydaj(cenaKuponu);
        int pierwszeLosowanie = this.centrala.dajNumerNastępnegoLosowania();
        int ostatnieLosowanie = pierwszeLosowanie + liczbaLosowań - 1;

        try {
            Kupon kupon = new Kupon(this, this.centrala.dajNumerPorządkowyKuponów(),
                    pierwszeLosowanie, liczbaLosowań, liczbaZakładów);
            // autoryzacja kuponu
            this.SprzedaneKupony.put(kupon.dajIdentyfikatorKuponu(), kupon);
            this.centrala.zwiększNumerPorządkowyKuponów();

            long piątaCzęśćKosztu = cenaKuponu / 5;

            this.centrala.odbierzZysk(piątaCzęśćKosztu * 4, pierwszeLosowanie, ostatnieLosowanie);
            this.centrala.odprowadźPodatek(piątaCzęśćKosztu);

            return kupon;
        } catch (NiepoprawnyArgument e) {
            System.exit(1);
            return null;
        }
    }

    long[] obliczZwycięskieZakłady(Losowanie losowanie) {
        if (this.SprzedaneKupony.isEmpty()) {
            return new long[] {0, 0, 0, 0, 0};
        }

        TreeSet<Integer> ZwycięskaSzóstka = losowanie.dajZwycięskąSzóstkę();
        int numerLosowania = losowanie.dajNumerLosowania();

        long[] ZwycięskieZakłady = new long[DOMYŚLNY_ROZMIAR_TABLIC];

        for (var kupon : this.SprzedaneKupony.values()) {
            ArrayList<Integer> stopnie = kupon.stopnieNagród(ZwycięskaSzóstka, numerLosowania);

            if (stopnie != null) {
                for (var stopień: stopnie) {
                    if (CentralaTotolotka.PRZEDZIAŁ_STOPNI_LK <= stopień && stopień <= CentralaTotolotka.PRZEDZIAŁ_STOPNI_PK) {
                        ZwycięskieZakłady[stopień]++;
                    }
                }
            }
        }

        return ZwycięskieZakłady;
    }

    int dajNumerPorządkowy() {
        return this.numerPorządkowy;
    }

    int dajNumerOstatniegoLosowania() {
        return this.centrala.dajNumerOstatniegoLosowania();
    }

    long zrealizujKupon(Kupon kupon) throws ZłaRealizacjaKuponu {
        if (kupon == null) {
            throw new NullPointerException();
        }

        if (this.SprzedaneKupony.get(kupon.dajIdentyfikatorKuponu()) == null) {
            throw new ZłaRealizacjaKuponu("Gracz wybrał złą kolekturą lub podrobił kupon.");
        }

        if (kupon.dajMiejsceZakupu() != this) {
            throw new ZłaRealizacjaKuponu("Gracz wybrał złą kolekturą lub podrobił kupon.");
        }

        if (kupon.czyZrealizowany()) {
            throw new ZłaRealizacjaKuponu("Został już zrealizowany.");
        }

        kupon.ustawKuponNaZrealizowany();
        return centrala.zrealizujKupon(kupon);
    }
}
