package typy_graczy;

import loteria.Blankiet;
import loteria.Gracz;
import loteria.Kolektura;
import wyjątki.NiepoprawnyArgument;
import wyjątki.ŹleWypełnionyBlankiet;

import java.util.TreeSet;

public class Stałoliczbowy extends Gracz {
    private final TreeSet<Integer> UlubioneLiczby;
    private final Kolektura[] UlubioneKolektury;
    private int licznik = 0;


    public Stałoliczbowy(String imię, String nazwisko, String pesel, long środkiPieniężne,
                         Kolektura[] UlubioneKolektury, TreeSet<Integer> UlubioneLiczby) throws NiepoprawnyArgument {
        super(imię, nazwisko, pesel, środkiPieniężne);

        if (UlubioneKolektury == null || UlubioneLiczby == null
            || UlubioneKolektury.length == 0 || UlubioneLiczby.size() != 6) {
            throw new NiepoprawnyArgument("Taki gracz musi się zdecydować.");
        }

        for (var liczba : UlubioneLiczby) {
            if (liczba < 1 || 49 < liczba) {
                throw new NiepoprawnyArgument("Gracz wybrał liczby spoza zakresu.");
            }
        }

        this.UlubioneLiczby = UlubioneLiczby;
        this.UlubioneKolektury = UlubioneKolektury;
    }

    @Override
    public void usłyszInformację(TreeSet<Integer> zwycięskaSzóstka, int numerLosowania) {
        this.sprawdźKupony();

        try {
            for (int i = 0; i < 3; i++) {
                Blankiet blankiet = new Blankiet(this.UlubioneLiczby, Kolektura.MAX_LICZBA_LOSOWAŃ);
                if (this.czyMaConajmniej(blankiet.wyceńBlankiet())) {
                    this.kupKupon(blankiet, this.UlubioneKolektury[this.licznik]);

                    this.licznik++;
                    this.licznik %= this.UlubioneKolektury.length;
                }
            }
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet e) {
            System.exit(1);
        }
    }
}
