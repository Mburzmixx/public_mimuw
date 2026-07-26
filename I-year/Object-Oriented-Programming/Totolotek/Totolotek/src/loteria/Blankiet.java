package loteria;

import wyjątki.NiepoprawnyArgument;
import wyjątki.ŹleWypełnionyBlankiet;

import java.util.ArrayList;
import java.util.TreeSet;

public class Blankiet {
    private final Zakład[] zakłady;
    private int liczbaLosowań;

    private final static int MAX_LICZBA_ZAKŁADÓW = 8;
    private final static int LICZBA_PÓL_LOSOWAŃ = 10;


    public Blankiet(Zakład[] zakłady, boolean[] liczbaLosowań) throws ŹleWypełnionyBlankiet {
        if (zakłady == null) {
            throw new NullPointerException();
        }

        if (zakłady.length == 0 || zakłady.length > MAX_LICZBA_ZAKŁADÓW) {
            throw new ŹleWypełnionyBlankiet("Za mało lub za dużo zakładów na jeden blankiet.");
        }

        if (liczbaLosowań.length != LICZBA_PÓL_LOSOWAŃ) {
            throw new ŹleWypełnionyBlankiet("Dorysowano pola w liczbie losowań.");
        }

        for (Zakład zakład : zakłady) {
            if (zakład == null) {
                throw new ŹleWypełnionyBlankiet("Zły format, w tej konwencji pomijamy NULLe przy tworzeniu");
            }
        }

        this.zakłady = zakłady;

        this.liczbaLosowań = 1;
        for (int i = 0; i < liczbaLosowań.length; i++) {
            if (liczbaLosowań[i]) {
                this.liczbaLosowań = i + 1;
            }
        }
    }

    public Blankiet(TreeSet<Integer> wybraneLiczby, int liczbaLosowań) throws NiepoprawnyArgument, ŹleWypełnionyBlankiet {
        if (wybraneLiczby == null || wybraneLiczby.size() != 6) {
            throw new ŹleWypełnionyBlankiet("Niepoprawnie wybrano liczby do zakładu.");
        }

        if (liczbaLosowań <= 0 || liczbaLosowań > LICZBA_PÓL_LOSOWAŃ) {
            throw new ŹleWypełnionyBlankiet("Liczba losowań nie mieści się w oczekiwanym przedziale");
        }

        this.zakłady = new Zakład[] {new Zakład(wybraneLiczby)};

        this.liczbaLosowań = liczbaLosowań;
    }


    public long wyceńBlankiet() {
        long wynik = 0;
        for (var zakład : this.zakłady) {
            if (zakład != null) {
                wynik += zakład.wyceńZakład();
            }
        }

        return wynik * this.liczbaLosowań;
    }

    public ArrayList<TreeSet<Integer>> dajWytypowaneLiczby() {
        ArrayList<TreeSet<Integer>> WytypowaneLiczby = new ArrayList<>();
        for (var zakład : this.zakłady) {
            if (zakład != null && zakład.dajWytypowaneLiczby() != null) {
                WytypowaneLiczby.add(zakład.dajWytypowaneLiczby());
            }
        }

        // WytypowaneLiczby mogą być puste np. gdy wszystkie zakłady są niepoprawnie wypełnione.
        // W takim przypadku kupon będzie składał się z jednego zakładu "1, 2, 3, 4, 5, 6".
        return WytypowaneLiczby;
    }

    public int dajLiczbęLosowań() {
        return this.liczbaLosowań;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();

        for (int i = 0; i < this.zakłady.length; i++) {
            sb.append(i + 1).append("\n");

            sb.append(this.zakłady[i]);
        }

        sb.append("Liczba losowań: ");
        for (int i = 0; i < LICZBA_PÓL_LOSOWAŃ; i++) {
            sb.append(" [ ");
            if (i + 1 == liczbaLosowań) {
                sb.append("--");
            } else {
                sb.append(i + 1);
            }
            sb.append(" ] ");
        }
        sb.append("\n");

        return sb.toString();
    }
}
