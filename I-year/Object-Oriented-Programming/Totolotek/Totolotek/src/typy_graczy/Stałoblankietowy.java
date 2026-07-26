package typy_graczy;

import loteria.Blankiet;
import loteria.Gracz;
import loteria.Kolektura;
import wyjątki.ŹleWypełnionyBlankiet;

import java.util.TreeSet;

public class Stałoblankietowy extends Gracz {
    private final Blankiet ulubionyBlankiet;
    private final Kolektura[] UlubioneKolektury;
    private final int tajemniczyCyklLosowań;
    private final int pierwszeLosowanie;
    private int licznik = 0;


    public Stałoblankietowy(String imię, String nazwisko, String pesel, long środkiPieniężne, int pierwszeLosowanie,
                            Blankiet ulubionyBlankiet, Kolektura[] UlubioneKolektury, int tajemniczyCyklLosowań) {
        super(imię, nazwisko, pesel, środkiPieniężne);

        if (ulubionyBlankiet == null || UlubioneKolektury == null
                || UlubioneKolektury.length == 0 || tajemniczyCyklLosowań == 0) {
            throw new IllegalArgumentException();
        }

        this.ulubionyBlankiet = ulubionyBlankiet;
        this.UlubioneKolektury = UlubioneKolektury;
        this.tajemniczyCyklLosowań = tajemniczyCyklLosowań;
        this.pierwszeLosowanie = pierwszeLosowanie;
    }

    @Override
    public void usłyszInformację(TreeSet<Integer> zwycięskaSzóstka, int numerLosowania) {
        this.sprawdźKupony();

        try {
            if (numerLosowania >= this.pierwszeLosowanie &&
                    (numerLosowania - this.pierwszeLosowanie) % this.tajemniczyCyklLosowań == 0) {
                if (this.czyMaConajmniej(this.ulubionyBlankiet.wyceńBlankiet())) {
                    this.kupKupon(this.ulubionyBlankiet, this.UlubioneKolektury[licznik]);

                    this.licznik++;
                    this.licznik %= this.UlubioneKolektury.length;
                }
            }
        } catch (ŹleWypełnionyBlankiet e) {
            System.out.println("Źle wypełniony blankiet");
            System.out.println(e.getMessage());
            System.exit(1);
        }
    }
}
