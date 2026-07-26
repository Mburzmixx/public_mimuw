package typy_graczy;

import loteria.Blankiet;
import loteria.Gracz;
import loteria.Kolektura;
import loteria.Media;
import wyjątki.NiepoprawnyArgument;
import wyjątki.ŹleWypełnionyBlankiet;

import java.util.Random;
import java.util.TreeSet;

public class Losowy extends Gracz {
    private final Media media;

    public Losowy(String imię, String nazwisko, String pesel, long środkiPieniężne, Media media) {
        super(imię, nazwisko, pesel, środkiPieniężne);

        this.media = media;
    }

    @Override
    public void usłyszInformację(TreeSet<Integer> zwycięskaSzóstka, int numerLosowania) {
        this.sprawdźKupony();

        Random r = new Random();
        int liczbaKuponów = 1 + r.nextInt(100);
        int liczbaLosowań = 1 + r.nextInt(10);

        TreeSet<Integer> losoweLiczby = new TreeSet<>();
        while (losoweLiczby.size() != 6) {
            losoweLiczby.add(1 + r.nextInt(49));
        }

        try {
            for (int i = 0; i < liczbaKuponów; i++) {
                Kolektura kolektura = this.media.dajLosowąKolekturę();

                if (this.czyMaConajmniej(Kolektura.wyceńKupon(1, liczbaLosowań))) {
                    this.kupKupon(new Blankiet(losoweLiczby, liczbaLosowań), kolektura);
                }
            }
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet e) {
            System.exit(1);
        }
    }
}
