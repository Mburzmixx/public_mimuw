import loteria.*;
import typy_graczy.*;
import wyjątki.NiepoprawnyArgument;
import wyjątki.NieznanyIndeks;
import wyjątki.ŹleWypełnionyBlankiet;

import java.util.Random;
import java.util.TreeSet;

public class PrezentacjaKlasWDziałaniu {
    private final static long PIENIĄDZE = 150_000_00L;

    public static void  main(String[] args) {
        try {
            BudżetPaństwa budżetPaństwa = new BudżetPaństwa();
            Media media = new Media();

            CentralaTotolotka centralaTotolotka = new CentralaTotolotka(budżetPaństwa, media);

            Kolektura[] kolektury = new Kolektura[10];
            for (int i = 0; i < 10; i++) {
                kolektury[i] = new Kolektura(centralaTotolotka);
            }

            Losowy[] losowi = new Losowy[200];
            Minimalista[] minimaliści = new Minimalista[200];
            Stałoblankietowy[] stałoblankietowi = new Stałoblankietowy[200];
            Stałoliczbowy[] stałoliczbowi = new Stałoliczbowy[200];

            // przydział losowych
            for (int i = 0; i < 200; i++) {
                String imię = "Losowniś" + i;
                String nazwisko = "Nieprzewidywalny" + i;
                String pesel = PrezentacjaKlasWDziałaniu.dajLosowyPesel();

                // Wszelkie podobieństwo do osób i zdarzeń rzeczywistych, jak i w treści,
                // jest zupełnie przypadkowe.
                losowi[i] = new Losowy(imię, nazwisko, pesel, PIENIĄDZE, media);
                // a niech mają trochę kasy, więcej będzie dla centrali ;)

                media.dodajNowegoGracza(losowi[i]);
            }

            // przydział minimalistów
            for (int i = 0; i < 200; i++) {
                String imię = "Minimaliś" + i;
                String nazwisko = "Malutki" + i;
                String pesel = PrezentacjaKlasWDziałaniu.dajLosowyPesel();

                minimaliści[i] = new Minimalista(imię, nazwisko, pesel, PIENIĄDZE, media.dajLosowąKolekturę());

                media.dodajNowegoGracza(minimaliści[i]);
            }

            // przydział stałoblankietowych
            for (int i = 0; i < 100; i++) {
                String imię = "Stały" + i;
                String nazwisko = "Blankieciarz" + i;
                String pesel = PrezentacjaKlasWDziałaniu.dajLosowyPesel();

                Random r = new Random();
                TreeSet<Integer> wybraneLiczby = new TreeSet<>();
                while (wybraneLiczby.size() < 6) {
                    wybraneLiczby.add(1 + r.nextInt(49));
                }

                Blankiet ulubionyBlankiet = new Blankiet(wybraneLiczby, 1 + (i % 6));

                stałoblankietowi[i] = new Stałoblankietowy(imię, nazwisko, pesel, PIENIĄDZE,
                        1, ulubionyBlankiet, new Kolektura[]{media.dajLosowąKolekturę()}, 1 + (i % 4));

                media.dodajNowegoGracza(stałoblankietowi[i]);
            }

            for (int i = 100; i < 150; i++) {
                String imię = "Średniak" + i;
                String nazwisko = "Blankieciarz" + i;
                String pesel = PrezentacjaKlasWDziałaniu.dajLosowyPesel();

                Random r = new Random();
                TreeSet<Integer> wybraneLiczby = new TreeSet<>();
                while (wybraneLiczby.size() < 6) {
                    wybraneLiczby.add(1 + r.nextInt(49));
                }

                Blankiet ulubionyBlankiet = new Blankiet(wybraneLiczby, 1 + (i % 7));

                Kolektura[] ulubioneKolektury = new Kolektura[3];
                ulubioneKolektury[0] = media.dajLosowąKolekturę();
                ulubioneKolektury[1] = media.dajLosowąKolekturę();
                ulubioneKolektury[2] = media.dajLosowąKolekturę();

                stałoblankietowi[i] = new Stałoblankietowy(imię, nazwisko, pesel, PIENIĄDZE,
                        1, ulubionyBlankiet, ulubioneKolektury, 1 + i % 3);

                media.dodajNowegoGracza(stałoblankietowi[i]);
            }

            for (int i = 150; i < 200; i++) {
                String imię = "Koneser" + i;
                String nazwisko = "Blankieciarski" + i;
                String pesel = PrezentacjaKlasWDziałaniu.dajLosowyPesel();

                Kolektura[] ulubioneKolektury = new Kolektura[5];
                for (int j = 0; j < 5; j++) {
                    ulubioneKolektury[j] = media.dajLosowąKolekturę();
                }

                int tajemniczyCyklLosowań = 1 + i % 7;


                Zakład[] zakłady = new Zakład[8];
                zakłady[0] = new Zakład(new boolean[][]{
                        {false, false, true, false, false, false, false, true, false, false},
                        {false, false, false, false, false, false, false, false, true, false},
                        {false, true, false, true, false, false, false, false, false, false},
                        {false, false, false, false, false, false, false, false, false, false},
                        {true, false, false, false, false, false, false, false, false, false}
                });
                // ;)

                zakłady[1] = new Zakład(new boolean[][]{
                        {false, false, true, false, false, false, false, true, false, false},
                        {false, false, false, false, false, false, false, false, true, false},
                        {false, false, false, true, false, false, false, false, false, false},
                        {true, false, false, false, false, false, false, false, false, false},
                        {false, false, true, false, false, false, false, false, false, false}
                });

                zakłady[2] = new Zakład(new boolean[][]{
                        {false, false, true, false, true, false, true, false, false, false},
                        {false, false, false, false, false, false, false, true, false, false},
                        {false, false, false, false, false, false, false, false, false, false},
                        {true, false, false, false, false, false, false, false, false, false},
                        {false, false, true, false, false, false, false, false, false, false}
                });

                zakłady[3] = new Zakład(new boolean[][]{
                        {false, false, false, false, false, false, true, false, false, false},
                        {false, false, false, false, false, false, false, true, false, false},
                        {false, false, false, false, false, false, false, false, false, false},
                        {true, false, false, false, false, false, false, false, false, false},
                        {false, false, true, true, true, false, false, false, false, false}
                });

                zakłady[4] = zakłady[0];
                zakłady[5] = zakłady[1];
                zakłady[6] = zakłady[2];
                zakłady[7] = zakłady[3];

                boolean[] liczbaLosowań = new boolean[10];
                liczbaLosowań[i % 10] = true;

                Blankiet ulubionyBlankiet = new Blankiet(zakłady, liczbaLosowań);

                stałoblankietowi[i] = new Stałoblankietowy(imię, nazwisko, pesel, PIENIĄDZE,
                        1, ulubionyBlankiet, ulubioneKolektury, tajemniczyCyklLosowań);

                media.dodajNowegoGracza(stałoblankietowi[i]);
            }

            // przydział stałoliczbowych
            for (int i = 0; i < 100; i++) {
                String imię = "Zwyczajny" + i;
                String nazwisko = "Stałoliczbowy" + i;
                String pesel = PrezentacjaKlasWDziałaniu.dajLosowyPesel();

                Random r = new Random();
                TreeSet<Integer> wybraneLiczby = new TreeSet<>();
                while (wybraneLiczby.size() < 6) {
                    wybraneLiczby.add(1 + r.nextInt(49));
                }

                stałoliczbowi[i] = new Stałoliczbowy(imię, nazwisko, pesel, PIENIĄDZE,
                        new Kolektura[]{media.dajLosowąKolekturę()}, wybraneLiczby);

                media.dodajNowegoGracza(stałoliczbowi[i]);
            }

            for (int i = 100; i < 150; i++) {
                String imię = "JużJakiś" + i;
                String nazwisko = "Stałoliczbowiec" + i;
                String pesel = PrezentacjaKlasWDziałaniu.dajLosowyPesel();

                Random r = new Random();
                TreeSet<Integer> wybraneLiczby = new TreeSet<>();
                while (wybraneLiczby.size() < 6) {
                    wybraneLiczby.add(1 + r.nextInt(49));
                }

                stałoliczbowi[i] = new Stałoliczbowy(imię, nazwisko, pesel, PIENIĄDZE,
                        new Kolektura[]{media.dajLosowąKolekturę(), media.dajLosowąKolekturę()}, wybraneLiczby);

                media.dodajNowegoGracza(stałoliczbowi[i]);
            }

            for (int i = 150; i < 200; i++) {
                String imię = "Najwyższy" + i;
                String nazwisko = "Stałoliczbowy" + i;
                String pesel = PrezentacjaKlasWDziałaniu.dajLosowyPesel();

                Random r = new Random();
                TreeSet<Integer> wybraneLiczby = new TreeSet<>();
                while (wybraneLiczby.size() < 6) {
                    wybraneLiczby.add(1 + r.nextInt(49));
                }

                Kolektura[] ulubioneKolektury = new Kolektura[4];
                for (int j = 0; j < 4; j++) {
                    ulubioneKolektury[j] = media.dajLosowąKolekturę();
                }

                stałoliczbowi[i] = new Stałoliczbowy(imię, nazwisko, pesel, PIENIĄDZE,
                        ulubioneKolektury, wybraneLiczby);

                media.dodajNowegoGracza(stałoliczbowi[i]);
            }

            for (int i = 0; i < 20; i++) {
                centralaTotolotka.przeprowadźLosowanie();
            }


            System.out.println(centralaTotolotka.wynikiWszystkichLosowań());

            System.out.println(centralaTotolotka.wypiszStanŚrodkówFinansowych());
            System.out.println("Wpływy do budżetu państwa: " + budżetPaństwa.dajPobranePodatki());
            System.out.println("Przekazane subwencje przez budżet państwa: " + budżetPaństwa.dajPrzekazaneSubwencje());
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet | NieznanyIndeks e) {
            System.exit(1);
        }
    }

    public static String dajLosowyPesel() {
        Random r = new Random();
        StringBuilder sb = new StringBuilder();

        for (int i = 0; i < 11; i++) {
            sb.append(r.nextInt(10));
        }

        return sb.toString();
    }
}
