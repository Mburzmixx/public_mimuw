package typy_graczy;

import loteria.Blankiet;
import loteria.Gracz;
import loteria.Kolektura;
import loteria.Kupon;
import wyjątki.NiepoprawnyArgument;

public class Minimalista extends Gracz {
    private final Kolektura ulubionaKolektura;

    private static final int LICZBA_LOSOWAŃ = 1;
    private static final int LICZBA_ZAKŁADÓW = 1;

    public Minimalista(String imię, String nazwisko, String pesel, long środkiPieniężne, Kolektura ulubionaKolektura) {
        super(imię, nazwisko, pesel, środkiPieniężne);

        this.ulubionaKolektura = ulubionaKolektura;
    }

    @Override
    public void kupKupon(Blankiet blankiet, Kolektura kolektura) {
        this.kupKupon();
    }

    @Override
    public void kupKupon(int liczbaZakładów, int liczbaLosowań, Kolektura kolektura) {
        this.kupKupon();
    }

    public void kupKupon() {
        try {
            Kupon kupon = this.ulubionaKolektura.sprzedajKupon(LICZBA_ZAKŁADÓW, LICZBA_LOSOWAŃ, this);
            this.ZakupioneKupony.add(kupon);
        } catch (NiepoprawnyArgument e) {
            System.exit(1);
        }
    }
}
