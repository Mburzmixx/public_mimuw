package loteria;

import wyjątki.*;

import java.util.ArrayList;
import java.util.TreeSet;

public abstract class Gracz {
    private final String imię;
    private final String nazwisko;
    private final String pesel;
    private long środkiPieniężne; // przechowuję liczbę groszy, nie złotówek

    protected final ArrayList<Kupon> ZakupioneKupony = new ArrayList<>();

    private final static int DOMYŚLNA_LICZBA_ZAKŁADÓW = 1;
    private final static int DOMYŚLNA_LICZBA_LOSOWAŃ = 1;


    public Gracz(String imię, String nazwisko, String pesel, long środkiPieniężne) {
        if(środkiPieniężne < 0) {
            throw new BankructwoGracza("Zlituj się nad nim. Na start już ma długi.");
        }

        this.imię = imię;
        this.nazwisko = nazwisko;
        this.pesel = pesel;
        this.środkiPieniężne = środkiPieniężne;
    }


    public void kupKupon(Blankiet blankiet, Kolektura kolektura) throws ŹleWypełnionyBlankiet {
        if (kolektura == null) {
            throw new NullPointerException();
        }

        Kupon kupon = kolektura.sprzedajKupon(blankiet, this);
        this.ZakupioneKupony.add(kupon);
    }

    public void kupKupon(int liczbaZakładów, int liczbaLosowań, Kolektura kolektura) throws NiepoprawnyArgument {
        if (kolektura == null) {
            throw new NullPointerException();
        }

        Kupon kupon = kolektura.sprzedajKupon(liczbaZakładów, liczbaLosowań, this);
        this.ZakupioneKupony.add(kupon);
    }

    public void zrealizujKupon(int indeksKuponu, Kolektura kolektura) throws NieznanyIndeks, ZłaRealizacjaKuponu {
        if (indeksKuponu < 0 || indeksKuponu >= this.ZakupioneKupony.size()) {
            throw new NieznanyIndeks("Ten gracz nie posiada kuponu o takim indeksie.");
        }

        Kupon kupon = this.ZakupioneKupony.get(indeksKuponu);
        long wypłata = kolektura.zrealizujKupon(kupon);

        this.środkiPieniężne += wypłata;
    }

    public void zrealizujKupon(Kupon kupon, Kolektura kolektura) throws ZłaRealizacjaKuponu {
        if (!this.ZakupioneKupony.contains(kupon)) {
            throw new ZłaRealizacjaKuponu("Nastąpiła pomyłka lub kradzież.");
        }

        if (kolektura == null) {
            throw new NullPointerException();
        }

        long wypłata = kolektura.zrealizujKupon(kupon);
        this.środkiPieniężne += wypłata;
    }

    public void wydaj(long kwota) {
        this.środkiPieniężne -= kwota;

        if (this.środkiPieniężne < 0) {
            throw new BankructwoGracza("Trzeba było mniej wydawać.");
        }
    }

    public boolean czyMaConajmniej(long kwota) {
        return (kwota <= środkiPieniężne);
    }

    public long dajStanKonta() {
        return this.środkiPieniężne;
    }

    protected void usłyszInformację(TreeSet<Integer> zwycięskaSzóstka, int numerLosowania) {
        this.sprawdźKupony();

        for (var kupon : ZakupioneKupony) {
            Kolektura kolektura = kupon.dajMiejsceZakupu();

            try {
                if (this.czyMaConajmniej(Kolektura.wyceńKupon(DOMYŚLNA_LICZBA_ZAKŁADÓW, DOMYŚLNA_LICZBA_LOSOWAŃ))) {
                    this.kupKupon(DOMYŚLNA_LICZBA_ZAKŁADÓW, DOMYŚLNA_LICZBA_LOSOWAŃ, kolektura);
                }
            } catch (NiepoprawnyArgument e) {
                System.exit(1);
            }
        }
    }

    protected void sprawdźKupony() {
        for (var kupon : ZakupioneKupony) {
            if (kupon != null) {
                Kolektura kolektura = kupon.dajMiejsceZakupu();

                if (kupon.czyOdbyłySięWszystkieLosowania() && !kupon.czyZrealizowany()) {
                    try {
                        this.zrealizujKupon(kupon, kolektura);
                    } catch (ZłaRealizacjaKuponu e) {
                        System.exit(1);
                    }
                }
            }
        }
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();

        sb.append("Imię: ").append(this.imię);
        sb.append(", Nazwisko: ").append(this.nazwisko);
        sb.append(", Pesel: ").append(this.pesel).append(".\n");
        sb.append("Mój stan konta to: ").append(this.środkiPieniężne / 100).append(" złoty oraz ");
        sb.append(this.środkiPieniężne % 100).append(" groszy.").append(".\n");
        sb.append("Moje zakupione kupony to:\n");
        for (var kupon : ZakupioneKupony) {
            sb.append(kupon.toString());
        }

        return sb.toString();
    }
}
