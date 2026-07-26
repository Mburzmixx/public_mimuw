package loteria;

import wyjątki.NiepoprawnyArgument;
import wyjątki.NieznanyIndeks;

import java.util.ArrayList;
import java.util.Random;
import java.util.TreeSet;

public class CentralaTotolotka {
    private long środkiFinansowe; // przechowuję liczbę groszy, nie złotówek
    private long kumulacja;
    private int numerPorządkowyKuponów = 0;
    private int numerPorządkowyKolektur = 0;
    private int numerPorządkowyLosowań = 0;
    private final ArrayList<Long> budżetyLosowań = new ArrayList<>();
    private final ArrayList<Losowanie> archiwumLosowań = new ArrayList<>();
    private final ArrayList<Kolektura> autoryzowaneKolektury = new ArrayList<>();

    private final Media media;
    private final BudżetPaństwa budżetPaństwa;

    private final static long NAGRODA_CZWARTEGO_STOPNIA = 24_00L;      // groszy, czyli 24 złote
    private final static long MIN_NAGRODA_TRZECIEGO_STOPNIA = 36_00L;  // adnotacja j.w.
    private final static long MIN_OPODATKOWANA_KWOTA = 2280_00L;
    public final static int PRZEDZIAŁ_STOPNI_LK = 1;
    public final static int PRZEDZIAŁ_STOPNI_PK = 4;
    private final static int DOMYŚLNY_ROZMIAR_TABLIC = PRZEDZIAŁ_STOPNI_PK + 1;
    private final static long MIN_NAGRODA_PIERWSZEGO_STOPNIA = 2_000_000_00L;


    public CentralaTotolotka(BudżetPaństwa budżetPaństwa, Media media) {
        this.budżetPaństwa = budżetPaństwa;
        this.środkiFinansowe = 60_000_000_00L;
        this.media = media;

        // Aby losowania numerować od 1:
        this.archiwumLosowań.add(null);
        for (int i = 0; i <= Kolektura.MAX_LICZBA_LOSOWAŃ; i++) {
            this.budżetyLosowań.add(0L);
        }
        // Z kolekturami tak nie robię, bo Kolektura "0" może być jakąś wyróżnioną,
        // natomiast losowania raczej numerujemy od "1".

        media.nawiążKontaktZCentraląTotolotka(this);
    }


    public void przeprowadźLosowanie() {
        this.numerPorządkowyLosowań++;
        this.budżetyLosowań.add(0L);
        Losowanie losowanie = new Losowanie(this.numerPorządkowyLosowań);


        long[] zwycięskieZakłady = this.obliczZwycięskieZakłady(losowanie);
        long[] pulaNagród = this.obliczPulęNagród(zwycięskieZakłady, losowanie);
        long[] kwotyWygranych = this.obliczKwotyWygranych(pulaNagród, zwycięskieZakłady);

        losowanie.ustawZwycięskieZakłady(zwycięskieZakłady);
        losowanie.ustawPulęNagród(pulaNagród);
        losowanie.ustawKwotyWygranych(kwotyWygranych);

        this.archiwumLosowań.add(losowanie);
        this.media.powiadomOWynikach(losowanie.dajZwycięskąSzóstkę(), this.numerPorządkowyLosowań);
    }

    private long[] obliczZwycięskieZakłady(Losowanie losowanie)     {
        long[] ZwycięskieZakłady = new long[DOMYŚLNY_ROZMIAR_TABLIC];

        for (var kolektura : autoryzowaneKolektury) {
            long[] LokalnyWynik = kolektura.obliczZwycięskieZakłady(losowanie);

            assert LokalnyWynik.length == DOMYŚLNY_ROZMIAR_TABLIC;

            for (int i = PRZEDZIAŁ_STOPNI_LK; i <= PRZEDZIAŁ_STOPNI_PK; i++) {
                ZwycięskieZakłady[i] += LokalnyWynik[i];
            }
        }

        return ZwycięskieZakłady;
    }

    private long[] obliczPulęNagród(long[] zwycięskieZakłady, Losowanie losowanie) {
        long[] pulaNagród = new long[DOMYŚLNY_ROZMIAR_TABLIC];
        long budżetLosowania = 0;
        try {
            budżetLosowania = this.dajBudżetLosowania(losowanie.dajNumerLosowania());
        } catch (NieznanyIndeks e) {
            System.exit(1);
        }

        long pierwszyStopień = (budżetLosowania * 44) / 100;
        pulaNagród[1] = Math.max(MIN_NAGRODA_PIERWSZEGO_STOPNIA, pierwszyStopień);

        long drugiStopień = (budżetLosowania * 8) / 100;
        pulaNagród[2] = drugiStopień;

        long czwartyStopień = zwycięskieZakłady[4] * NAGRODA_CZWARTEGO_STOPNIA;
        pulaNagród[4] = czwartyStopień;

        long trzeciStopień = budżetLosowania * 51 / 100;
        trzeciStopień -= pierwszyStopień;
        trzeciStopień -= drugiStopień;
        trzeciStopień -= czwartyStopień;

        long dolneOgraniczenie = MIN_NAGRODA_TRZECIEGO_STOPNIA * zwycięskieZakłady[3];
        pulaNagród[3] = Math.max(dolneOgraniczenie, trzeciStopień);
        pulaNagród[3] = Math.max(pulaNagród[3], MIN_NAGRODA_TRZECIEGO_STOPNIA);

        pulaNagród[1] = pulaNagród[1] + this.kumulacja;
        this.kumulacja = 0;
        if (zwycięskieZakłady[1] == 0) {
            this.kumulacja = pulaNagród[1];
            // Puli nagród pierwszego stopnia nie zmieniam, bo była-tylko nikt jej nie zdobył.
        }

        return pulaNagród;
    }

    private long[] obliczKwotyWygranych(long[] pulaNagród, long[] zwycięskieZakłady) {
        long[] kwotyWygranych = new long[DOMYŚLNY_ROZMIAR_TABLIC];

        for (int i = PRZEDZIAŁ_STOPNI_LK; i <= PRZEDZIAŁ_STOPNI_PK; i++) {
            if (zwycięskieZakłady[i] == 0) {
                kwotyWygranych[i] = pulaNagród[i];
            } else {
                kwotyWygranych[i] = pulaNagród[i]/ zwycięskieZakłady[i];
            }

            if (i == 4) {
                kwotyWygranych[i] = NAGRODA_CZWARTEGO_STOPNIA;
            }
        }

        return kwotyWygranych;
    }

    // zwraca numer porządkowy
    int autoryzujNowąKolekturę(Kolektura kolektura) {
        this.autoryzowaneKolektury.add(kolektura);
        this.numerPorządkowyKolektur++;
        return this.numerPorządkowyKolektur - 1;
    }

    public int dajNumerNastępnegoLosowania() {
        return this.numerPorządkowyLosowań + 1;
    }

    public int dajNumerOstatniegoLosowania() {
        return this.numerPorządkowyLosowań;
    }

    int dajNumerPorządkowyKuponów() {
        return this.numerPorządkowyKuponów;
    }

    void zwiększNumerPorządkowyKuponów() {
        this.numerPorządkowyKuponów++;
    }

    private void zwiększBudżetLosowania(int indeksLosowania, long kwota) {
        assert kwota >= 0;

        long nowyBudżet = this.budżetyLosowań.get(indeksLosowania) + kwota;
        this.budżetyLosowań.set(indeksLosowania, nowyBudżet);
    }

    public Kolektura dajLosowąKolekturę() {
        Random r = new Random();
        int indeks = r.nextInt(this.autoryzowaneKolektury.size());
        return this.autoryzowaneKolektury.get(indeks);
    }

    private void dobierzSubwencję(long kwota) {
        assert kwota >= 0;

        this.środkiFinansowe += kwota;
        this.budżetPaństwa.udzielSubwencji(kwota);
    }

    void odprowadźPodatek(long kwota) {
        this.budżetPaństwa.przyjmijPodatek(kwota);
    }

    void wydaj(long kwota) {
        if (kwota < 0) {
            throw new IllegalArgumentException();
        }

        this.środkiFinansowe -= kwota;
        if (this.środkiFinansowe < 0) {
            this.dobierzSubwencję(-kwota);
        }
    }

    void odbierzZysk(long kwota, int pierwszeLosowanie, int ostatnieLosowanie) {
        assert kwota >= 0;

        long liczbaLosowań = ostatnieLosowanie - pierwszeLosowanie + 1;
        long średniaKwota = kwota / liczbaLosowań;

        for (int i = pierwszeLosowanie; i <= ostatnieLosowanie; i++) {
            this.zwiększBudżetLosowania(i, średniaKwota);
        }

        this.środkiFinansowe += kwota;
    }

    long zrealizujKupon(Kupon kupon) {
        long wynik = 0;

        int pierwszeLosowanie = kupon.dajNumerPierwszegoLosowania();
        int ostatnieLosowanie = pierwszeLosowanie + kupon.dajLiczbęLosowań() - 1;


        for (int i = pierwszeLosowanie; i <= ostatnieLosowanie; i++) {
            Losowanie losowanie = archiwumLosowań.get(i);

            TreeSet<Integer> ZwycięskaSzóstka = losowanie.dajZwycięskąSzóstkę();
            int numerLosowania = losowanie.dajNumerLosowania();

            ArrayList<Integer> pomocnicza = kupon.stopnieNagród(ZwycięskaSzóstka, numerLosowania);

            // pomocnicza nie jest null-em, bo iteruje się po konkretnych losowaniach z kuponu.
            for (Integer k : pomocnicza) {
                if (PRZEDZIAŁ_STOPNI_LK <= k && k <= PRZEDZIAŁ_STOPNI_PK) {
                    long nagroda = 0;
                    try {
                        nagroda = losowanie.dajKwotęWygranej(k);
                    } catch (NieznanyIndeks e) {
                        System.exit(1);
                    }
                    wynik += nagroda;

                    if (nagroda >= MIN_OPODATKOWANA_KWOTA) {
                        wynik -= nagroda / 10;
                        this.odprowadźPodatek(nagroda / 10);
                    }
                }
            }
        }

        this.wydaj(wynik);

        return wynik;
    }

    public long dajNagrodęZaStopień(int indeksLosowania, int stopień) throws NiepoprawnyArgument, NieznanyIndeks {
        if (indeksLosowania < 0 || this.archiwumLosowań.size() <= indeksLosowania) {
            throw new NieznanyIndeks("Sprawdzanie nagrody konkretnego stopnia.");
        }

        if (stopień < PRZEDZIAŁ_STOPNI_LK || PRZEDZIAŁ_STOPNI_PK < stopień) {
            throw new NiepoprawnyArgument("Sprawdzanie nagrody konkretnego stopnia.");
        }

        return this.archiwumLosowań.get(indeksLosowania).dajKwotęWygranej(stopień);
    }

    public long dajLiczbęWygranychStopnia(int indeksLosowania, int stopień) throws NiepoprawnyArgument, NieznanyIndeks {
        if (indeksLosowania < 0 || this.archiwumLosowań.size() <= indeksLosowania) {
            throw new NieznanyIndeks("Sprawdzanie liczby wygranych konkretnego stopnia.");
        }

        if (stopień < PRZEDZIAŁ_STOPNI_LK || PRZEDZIAŁ_STOPNI_PK < stopień) {
            throw new NiepoprawnyArgument("Sprawdzanie liczby wygranych konkretnego stopnia.");
        }

        return this.archiwumLosowań.get(indeksLosowania).dajLiczbęWygranych(stopień);
    }

    public long dajBudżetLosowania(int indeksLosowania) throws NieznanyIndeks {
        if (indeksLosowania < 0 || this.archiwumLosowań.size() + 10 <= indeksLosowania) {
            throw new NieznanyIndeks("Sprawdzanie budżetu losowania.");
        }

        return this.budżetyLosowań.get(indeksLosowania);
    }

    public String wynikLosowania(int indeksLosowania) throws NieznanyIndeks {
        if (indeksLosowania == 0) {
            throw new NieznanyIndeks("sprawdzanie wyników losowania");
        }
        return this.archiwumLosowań.get(indeksLosowania).toString();
    }

    public String wynikiWszystkichLosowań() throws NieznanyIndeks {
        StringBuilder sb = new StringBuilder();
        sb.append("Przedstawiam wyniki losowań:\n");

        for (int i = 1; i < this.archiwumLosowań.size(); i++) {
            sb.append(this.wynikLosowania(i));
        }

        return sb.toString();
    }

    public String wypiszStanŚrodkówFinansowych() {
        return "Aktualny stań środków finansowych centrali to: " +
                (this.środkiFinansowe / 100) + " złoty oraz " + (this.środkiFinansowe % 100) + " groszy";
    }
}
