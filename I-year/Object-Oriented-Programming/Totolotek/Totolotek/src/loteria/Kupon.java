package loteria;

import wyjątki.NiepoprawnyArgument;

import java.util.ArrayList;
import java.util.Random;
import java.util.TreeSet;

public class Kupon {
    private final int numerPorządkowy;
    private final int liczbaLosowań;
    private final int numerPierwszegoLosowania;
    private boolean czyZrealizowany = false;

    private String identyfikatorKuponu;
    private final Kolektura miejsceZakupu;
    private final ArrayList<TreeSet<Integer>> WytypowaneLiczby;

    private final static int DOMYŚLNY_STOPNIEŃ = 7;
    private final static int MAX_LICZBA_LOSOWAŃ = 10;
    private final static int MAX_LICZBA_ZAKŁADÓW = 8;

    // Widoczność pakietowa
    Kupon(Kolektura miejsceZakupu, ArrayList<TreeSet<Integer>> WytypowaneLiczby, int numerPorządkowy,
                                                        int numerPierwszegoLosowania, int liczbaLosowań) {
        this.miejsceZakupu = miejsceZakupu;
        this.WytypowaneLiczby = WytypowaneLiczby;
        this.numerPorządkowy = numerPorządkowy;
        this.numerPierwszegoLosowania = numerPierwszegoLosowania;
        this.liczbaLosowań = liczbaLosowań;

        if (WytypowaneLiczby.isEmpty()) {
            TreeSet<Integer> domyślneStrzały = new TreeSet<>();

            for (int i = 1; i <= 6; i++) {
                domyślneStrzały.add(i);
            }

            WytypowaneLiczby.add(domyślneStrzały);
        }

        ustawIdentyfikatorKuponu(this.numerPorządkowy, this.miejsceZakupu.dajNumerPorządkowy());
    }

    // wersja kupowania na "chybił-trafił"
    Kupon(Kolektura miejsceZakupu, int numerPorządkowy, int numerPierwszegoLosowania,
                                                                    int liczbaLosowań, int liczbaZakładów) throws NiepoprawnyArgument {
        if (liczbaLosowań <= 0 || liczbaLosowań > MAX_LICZBA_LOSOWAŃ) {
            throw new NiepoprawnyArgument("Niepoprawna liczba losowań");
        }

        if (liczbaZakładów <= 0 || liczbaZakładów > MAX_LICZBA_ZAKŁADÓW) {
            throw new NiepoprawnyArgument("Niepoprawna liczba zakładów");
        }

        this.miejsceZakupu = miejsceZakupu;
        this.numerPorządkowy = numerPorządkowy;
        this.numerPierwszegoLosowania = numerPierwszegoLosowania;
        this.liczbaLosowań = liczbaLosowań;
        this.WytypowaneLiczby = new ArrayList<>();

        for (int i = 0; i < liczbaZakładów; i++) {
            TreeSet<Integer> skreśloneLiczby = new TreeSet<>();
            Random r = new Random();
            while(skreśloneLiczby.size() != 6) {
                skreśloneLiczby.add(1 + r.nextInt(49));
            }
        }

        this.ustawIdentyfikatorKuponu(this.numerPorządkowy, this.miejsceZakupu.dajNumerPorządkowy());
    }


    ArrayList<Integer> stopnieNagród(TreeSet<Integer> ZwycięskaSzóstka, int numerLosowania) {
        ArrayList<Integer> pomocnicza = new ArrayList<>();

        int maxNumerLosowania = this.numerPierwszegoLosowania + this.liczbaLosowań - 1;

        // Rozszerzam tu pojęcie stopnia wygranej do liczb od 1 do 7 w sposób liniowy.
        if (this.numerPierwszegoLosowania <= numerLosowania && numerLosowania <= maxNumerLosowania) {
            for (var liczby : this.WytypowaneLiczby) {
                int stopień = DOMYŚLNY_STOPNIEŃ;

                for (var liczba : liczby) {
                    if (ZwycięskaSzóstka.contains(liczba)) {
                        stopień--;
                    }
                }

                pomocnicza.add(stopień);
            }

            return pomocnicza;
        } else {
            return null;
        }

    }

    public boolean czyOdbyłySięWszystkieLosowania() {
        return this.numerPierwszegoLosowania + this.liczbaLosowań - 1
                <= this.miejsceZakupu.dajNumerOstatniegoLosowania();
    }

    public boolean czyZrealizowany() {
        return this.czyZrealizowany;
    }

    void ustawKuponNaZrealizowany() {
        this.czyZrealizowany = true;
    }

    private void ustawIdentyfikatorKuponu(int numerPorządkowy, int numerPorządkowyKolektury) {
        Random r = new Random();
        int losowyZnacznik = r.nextInt(1_000_000);
        int kopiaLZ = losowyZnacznik;

        int sumaKontrolna = 0;

        while (numerPorządkowy > 0) {
            sumaKontrolna += numerPorządkowy % 10;
            numerPorządkowy /= 10;
        }

        while (kopiaLZ > 0) {
            sumaKontrolna += kopiaLZ % 10;
            kopiaLZ /= 10;
        }

        while (numerPorządkowyKolektury > 0) {
            sumaKontrolna += numerPorządkowyKolektury % 10;
            numerPorządkowyKolektury /= 10;
        }

        String końcówka;
        if (sumaKontrolna % 100 < 10) {
            końcówka = "0" + sumaKontrolna;
        } else {
            końcówka = "" + (sumaKontrolna % 10);
        }

        this.identyfikatorKuponu = this.numerPorządkowy + "-" + this.miejsceZakupu.dajNumerPorządkowy() +
                "-" + losowyZnacznik + "-" + końcówka;
    }

    public String dajIdentyfikatorKuponu() {
        return this.identyfikatorKuponu;
    }

    public long dajOdprowadzanyPodatek() {
        try {
            return Kolektura.wyceńKupon(this.WytypowaneLiczby.size(), this.liczbaLosowań) / 5;
        } catch (NiepoprawnyArgument e) {
            System.exit(1);
            return 0; // z jakiegoś powodu intellij prosi o tego "returna".
        }
    }

    public long wyceńKupon() {
        try {
            return Kolektura.wyceńKupon(this.WytypowaneLiczby.size(), this.liczbaLosowań);
        } catch (NiepoprawnyArgument e) {
            System.exit(1);
            return 0;
        }
    }

    public Kolektura dajMiejsceZakupu() {
        return this.miejsceZakupu;
    }

    public int dajNumerPierwszegoLosowania() {
        return this.numerPierwszegoLosowania;
    }

    public int dajLiczbęLosowań() {
        return this.liczbaLosowań;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();

        sb.append("KUPON NR ").append(this.identyfikatorKuponu).append("\n");
        for (int i = 0; i < this.WytypowaneLiczby.size(); i++) {
            sb.append((i + 1)).append(": ");
            for (var liczba: this.WytypowaneLiczby.get(i)) {
                if (liczba < 10) {
                    sb.append(" ").append(liczba);
                } else {
                    sb.append(liczba);
                }
                sb.append(" ");
            }
            sb.append("\n");
        }

        sb.append("LICZBA LOSOWAŃ: ").append(this.liczbaLosowań).append("\n");
        sb.append("NUMERY LOSOWAŃ: \n");

        for (int i = 0; i < this.liczbaLosowań; i++) {
            if (this.numerPierwszegoLosowania + i < 10) {
                sb.append(" ").append((this.numerPierwszegoLosowania + i));
            } else {
                sb.append((this.numerPierwszegoLosowania + i));
            }
            sb.append(" ");
        }

        sb.append("\n");
        sb.append("CENA: ").append((this.wyceńKupon()/100)).append(" zł ");
        sb.append((this.wyceńKupon()%100)).append(" gr\n");

        return sb.toString();
    }
}
