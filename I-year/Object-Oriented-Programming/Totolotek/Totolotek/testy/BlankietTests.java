import loteria.Blankiet;
import loteria.Zakład;
import wyjątki.NiepoprawnyArgument;
import wyjątki.ŹleWypełnionyBlankiet;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

import java.util.ArrayList;
import java.util.Random;
import java.util.TreeSet;

public class BlankietTests {
    @Test
    public void poprawnyBlankietZZakładów() {
        try {
            // given
            Random r = new Random();

            ArrayList<TreeSet<Integer>> ArchiwumStrzałów = new ArrayList<>();
            Zakład[] Zakłady = new Zakład[8];

            for (int i = 0; i < 8; i++) {
                TreeSet<Integer> Strzały = new TreeSet<>();

                while (Strzały.size() != 6) {
                    Strzały.add(1 + r.nextInt(49));
                }

                Zakład zakład = new Zakład(Strzały);
                Zakłady[i] = zakład;

                ArchiwumStrzałów.add(Strzały);
            }

            boolean[] liczbaLosowań = new boolean[]
                {false, false, false, false, true, false, false, false, false, false};

            Blankiet blankiet = new Blankiet(Zakłady, liczbaLosowań);

            // when
            ArrayList<TreeSet<Integer>> Strzały = blankiet.dajWytypowaneLiczby();

            // then
            assertEquals(ArchiwumStrzałów, Strzały);
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet e) {
            fail();
        }
    }

    @Test
    public void poprawnyBlankietZWybranychLiczb() {
        try {
            // given
            Random r = new Random();
            ArrayList<TreeSet<Integer>> ArchiwumStrzałów = new ArrayList<>();

            TreeSet<Integer> Liczby = new TreeSet<>();

            while (Liczby.size() != 6) {
                Liczby.add(1 + r.nextInt(49));
            }

            ArchiwumStrzałów.add(Liczby);
            Blankiet blankiet = new Blankiet(Liczby, 4);

            // when
            ArrayList<TreeSet<Integer>> Strzały = blankiet.dajWytypowaneLiczby();

            // then
            assertEquals(ArchiwumStrzałów, Strzały);
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet e) {
            fail();
        }
    }

    @Test
    public void zaznaczenieParuKomórekLosowań() {
        try {
            // given
            Random r = new Random();

            Zakład[] Zakłady = new Zakład[8];

            for (int i = 0; i < 8; i++) {
                TreeSet<Integer> Strzały = new TreeSet<>();

                while (Strzały.size() != 6) {
                    Strzały.add(1 + r.nextInt(49));
                }

                Zakład zakład = new Zakład(Strzały);
                Zakłady[i] = zakład;
            }

            boolean[] liczbaLosowań = new boolean[]
                {true, false, false, false, true, false, true, false, true, false};
            int oczekiwanaLiczbaLosowań = 9;

            Blankiet blankiet = new Blankiet(Zakłady, liczbaLosowań);

            // when
            int zwróconaLiczbaLosowań = blankiet.dajLiczbęLosowań();

            // then
            assertEquals(oczekiwanaLiczbaLosowań, zwróconaLiczbaLosowań);
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet e) {
            fail();
        }
    }

    @Test
    public void prezentacjaTekstowa() {
        try {
            // given
            TreeSet<Integer> Strzały0 = new TreeSet<>();
            Strzały0.add(1);
            Strzały0.add(2);
            Strzały0.add(3);
            Strzały0.add(4);
            Strzały0.add(5);
            Strzały0.add(6);

            TreeSet<Integer> Strzały1 = new TreeSet<>();
            Strzały1.add(1);
            Strzały1.add(15);
            Strzały1.add(22);
            Strzały1.add(23);
            Strzały1.add(46);
            Strzały1.add(49);

            TreeSet<Integer> Strzały2 = new TreeSet<>();
            Strzały2.add(4);
            Strzały2.add(9);
            Strzały2.add(16);
            Strzały2.add(27);
            Strzały2.add(32);
            Strzały2.add(43);

            TreeSet<Integer> Strzały3 = new TreeSet<>();
            Strzały3.add(5);
            Strzały3.add(7);
            Strzały3.add(10);
            Strzały3.add(12);
            Strzały3.add(16);
            Strzały3.add(32);

            TreeSet<Integer> Strzały4 = new TreeSet<>();
            Strzały4.add(1);
            Strzały4.add(2);
            Strzały4.add(17);
            Strzały4.add(33);
            Strzały4.add(43);
            Strzały4.add(47);

            TreeSet<Integer> Strzały5 = new TreeSet<>();
            Strzały5.add(4);
            Strzały5.add(15);
            Strzały5.add(19);
            Strzały5.add(25);
            Strzały5.add(33);
            Strzały5.add(43);

            TreeSet<Integer> Strzały6 = new TreeSet<>();
            Strzały6.add(7);
            Strzały6.add(13);
            Strzały6.add(23);
            Strzały6.add(33);
            Strzały6.add(39);
            Strzały6.add(43);

            TreeSet<Integer> Strzały7 = new TreeSet<>();
            Strzały7.add(10);
            Strzały7.add(13);
            Strzały7.add(23);
            Strzały7.add(26);
            Strzały7.add(33);
            Strzały7.add(39);

            Zakład[] Zakłady = new Zakład[8];

            Zakłady[0] = new Zakład(Strzały0);
            Zakłady[1] = new Zakład(Strzały1);
            Zakłady[2] = new Zakład(Strzały2);
            Zakłady[3] = new Zakład(Strzały3);
            Zakłady[4] = new Zakład(Strzały4);
            Zakłady[5] = new Zakład(Strzały5);
            Zakłady[6] = new Zakład(Strzały6);
            Zakłady[7] = new Zakład(Strzały7);


            boolean[] liczbaLosowań = new boolean[]
                    {false, false, true, false, false, false, false, false, false, false};

            // Jest to sporo pisania, ale alternatywa to napisać kod generujący tekst,
            // lecz byłby on taki sam jak w kodzie, który testuję.

            String oczekiwany =
              "1" + "\n"
            + " [ -- ]  [ -- ]  [ -- ]  [ -- ]  [ -- ]  [ -- ]  [  7 ]  [  8 ]  [  9 ]  [ 10 ] " + "\n"
            + " [ 11 ]  [ 12 ]  [ 13 ]  [ 14 ]  [ 15 ]  [ 16 ]  [ 17 ]  [ 18 ]  [ 19 ]  [ 20 ] " + "\n"
            + " [ 21 ]  [ 22 ]  [ 23 ]  [ 24 ]  [ 25 ]  [ 26 ]  [ 27 ]  [ 28 ]  [ 29 ]  [ 30 ] " + "\n"
            + " [ 31 ]  [ 32 ]  [ 33 ]  [ 34 ]  [ 35 ]  [ 36 ]  [ 37 ]  [ 38 ]  [ 39 ]  [ 40 ] " + "\n"
            + " [ 41 ]  [ 42 ]  [ 43 ]  [ 44 ]  [ 45 ]  [ 46 ]  [ 47 ]  [ 48 ]  [ 49 ] " + "\n"
            + " [    ] anuluj" + "\n"
            + "2" + "\n"
            + " [ -- ]  [  2 ]  [  3 ]  [  4 ]  [  5 ]  [  6 ]  [  7 ]  [  8 ]  [  9 ]  [ 10 ] " + "\n"
            + " [ 11 ]  [ 12 ]  [ 13 ]  [ 14 ]  [ -- ]  [ 16 ]  [ 17 ]  [ 18 ]  [ 19 ]  [ 20 ] " + "\n"
            + " [ 21 ]  [ -- ]  [ -- ]  [ 24 ]  [ 25 ]  [ 26 ]  [ 27 ]  [ 28 ]  [ 29 ]  [ 30 ] " + "\n"
            + " [ 31 ]  [ 32 ]  [ 33 ]  [ 34 ]  [ 35 ]  [ 36 ]  [ 37 ]  [ 38 ]  [ 39 ]  [ 40 ] " + "\n"
            + " [ 41 ]  [ 42 ]  [ 43 ]  [ 44 ]  [ 45 ]  [ -- ]  [ 47 ]  [ 48 ]  [ -- ] " + "\n"
            + " [    ] anuluj" + "\n"
            + "3" + "\n"
            + " [  1 ]  [  2 ]  [  3 ]  [ -- ]  [  5 ]  [  6 ]  [  7 ]  [  8 ]  [ -- ]  [ 10 ] " + "\n"
            + " [ 11 ]  [ 12 ]  [ 13 ]  [ 14 ]  [ 15 ]  [ -- ]  [ 17 ]  [ 18 ]  [ 19 ]  [ 20 ] " + "\n"
            + " [ 21 ]  [ 22 ]  [ 23 ]  [ 24 ]  [ 25 ]  [ 26 ]  [ -- ]  [ 28 ]  [ 29 ]  [ 30 ] " + "\n"
            + " [ 31 ]  [ -- ]  [ 33 ]  [ 34 ]  [ 35 ]  [ 36 ]  [ 37 ]  [ 38 ]  [ 39 ]  [ 40 ] " + "\n"
            + " [ 41 ]  [ 42 ]  [ -- ]  [ 44 ]  [ 45 ]  [ 46 ]  [ 47 ]  [ 48 ]  [ 49 ] " + "\n"
            + " [    ] anuluj" + "\n"
            + "4" + "\n"
            + " [  1 ]  [  2 ]  [  3 ]  [  4 ]  [ -- ]  [  6 ]  [ -- ]  [  8 ]  [  9 ]  [ -- ] " + "\n"
            + " [ 11 ]  [ -- ]  [ 13 ]  [ 14 ]  [ 15 ]  [ -- ]  [ 17 ]  [ 18 ]  [ 19 ]  [ 20 ] " + "\n"
            + " [ 21 ]  [ 22 ]  [ 23 ]  [ 24 ]  [ 25 ]  [ 26 ]  [ 27 ]  [ 28 ]  [ 29 ]  [ 30 ] " + "\n"
            + " [ 31 ]  [ -- ]  [ 33 ]  [ 34 ]  [ 35 ]  [ 36 ]  [ 37 ]  [ 38 ]  [ 39 ]  [ 40 ] " + "\n"
            + " [ 41 ]  [ 42 ]  [ 43 ]  [ 44 ]  [ 45 ]  [ 46 ]  [ 47 ]  [ 48 ]  [ 49 ] " + "\n"
            + " [    ] anuluj" + "\n"
            + "5" + "\n"
            + " [ -- ]  [ -- ]  [  3 ]  [  4 ]  [  5 ]  [  6 ]  [  7 ]  [  8 ]  [  9 ]  [ 10 ] " + "\n"
            + " [ 11 ]  [ 12 ]  [ 13 ]  [ 14 ]  [ 15 ]  [ 16 ]  [ -- ]  [ 18 ]  [ 19 ]  [ 20 ] " + "\n"
            + " [ 21 ]  [ 22 ]  [ 23 ]  [ 24 ]  [ 25 ]  [ 26 ]  [ 27 ]  [ 28 ]  [ 29 ]  [ 30 ] " + "\n"
            + " [ 31 ]  [ 32 ]  [ -- ]  [ 34 ]  [ 35 ]  [ 36 ]  [ 37 ]  [ 38 ]  [ 39 ]  [ 40 ] " + "\n"
            + " [ 41 ]  [ 42 ]  [ -- ]  [ 44 ]  [ 45 ]  [ 46 ]  [ -- ]  [ 48 ]  [ 49 ] " + "\n"
            + " [    ] anuluj" + "\n"
            + "6" + "\n"
            + " [  1 ]  [  2 ]  [  3 ]  [ -- ]  [  5 ]  [  6 ]  [  7 ]  [  8 ]  [  9 ]  [ 10 ] " + "\n"
            + " [ 11 ]  [ 12 ]  [ 13 ]  [ 14 ]  [ -- ]  [ 16 ]  [ 17 ]  [ 18 ]  [ -- ]  [ 20 ] " + "\n"
            + " [ 21 ]  [ 22 ]  [ 23 ]  [ 24 ]  [ -- ]  [ 26 ]  [ 27 ]  [ 28 ]  [ 29 ]  [ 30 ] " + "\n"
            + " [ 31 ]  [ 32 ]  [ -- ]  [ 34 ]  [ 35 ]  [ 36 ]  [ 37 ]  [ 38 ]  [ 39 ]  [ 40 ] " + "\n"
            + " [ 41 ]  [ 42 ]  [ -- ]  [ 44 ]  [ 45 ]  [ 46 ]  [ 47 ]  [ 48 ]  [ 49 ] " + "\n"
            + " [    ] anuluj" + "\n"
            + "7" + "\n"
            + " [  1 ]  [  2 ]  [  3 ]  [  4 ]  [  5 ]  [  6 ]  [ -- ]  [  8 ]  [  9 ]  [ 10 ] " + "\n"
            + " [ 11 ]  [ 12 ]  [ -- ]  [ 14 ]  [ 15 ]  [ 16 ]  [ 17 ]  [ 18 ]  [ 19 ]  [ 20 ] " + "\n"
            + " [ 21 ]  [ 22 ]  [ -- ]  [ 24 ]  [ 25 ]  [ 26 ]  [ 27 ]  [ 28 ]  [ 29 ]  [ 30 ] " + "\n"
            + " [ 31 ]  [ 32 ]  [ -- ]  [ 34 ]  [ 35 ]  [ 36 ]  [ 37 ]  [ 38 ]  [ -- ]  [ 40 ] " + "\n"
            + " [ 41 ]  [ 42 ]  [ -- ]  [ 44 ]  [ 45 ]  [ 46 ]  [ 47 ]  [ 48 ]  [ 49 ] " + "\n"
            + " [    ] anuluj" + "\n"
            + "8" + "\n"
            + " [  1 ]  [  2 ]  [  3 ]  [  4 ]  [  5 ]  [  6 ]  [  7 ]  [  8 ]  [  9 ]  [ -- ] " + "\n"
            + " [ 11 ]  [ 12 ]  [ -- ]  [ 14 ]  [ 15 ]  [ 16 ]  [ 17 ]  [ 18 ]  [ 19 ]  [ 20 ] " + "\n"
            + " [ 21 ]  [ 22 ]  [ -- ]  [ 24 ]  [ 25 ]  [ -- ]  [ 27 ]  [ 28 ]  [ 29 ]  [ 30 ] " + "\n"
            + " [ 31 ]  [ 32 ]  [ -- ]  [ 34 ]  [ 35 ]  [ 36 ]  [ 37 ]  [ 38 ]  [ -- ]  [ 40 ] " + "\n"
            + " [ 41 ]  [ 42 ]  [ 43 ]  [ 44 ]  [ 45 ]  [ 46 ]  [ 47 ]  [ 48 ]  [ 49 ] " + "\n"
            + " [    ] anuluj" + "\n"
            + "Liczba losowań:  [ 1 ]  [ 2 ]  [ -- ]  [ 4 ]  [ 5 ]  [ 6 ]  [ 7 ]  [ 8 ]  [ 9 ]  [ 10 ] " + "\n";

            Blankiet blankiet = new Blankiet(Zakłady, liczbaLosowań);

            // when
            String zwrócony = blankiet.toString();

            // then
            assertEquals(oczekiwany, zwrócony);
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet e) {
            fail();
        }
    }

    @Test
    public void wycenaBlankietuWypełnianego() {
        try {
            // given
            Random r = new Random();

            Zakład[] Zakłady = new Zakład[8];
            for (int i = 0; i < 8; i++) {
                TreeSet<Integer> Strzały = new TreeSet<>();

                while (Strzały.size() != 6) {
                    Strzały.add(1 + r.nextInt(49));
                }

                Zakład zakład = new Zakład(Strzały);

                Zakłady[i] = zakład;
            }
            boolean[] liczbaLosowań = new boolean[]
                {false, false, false, false, false, false, false, false, true, false};

            Blankiet blankiet = new Blankiet(Zakłady, liczbaLosowań);

            long oczekiwany = 8 * 9 * 3_00L;

            // when
            long zwrócony = blankiet.wyceńBlankiet();

            // then
            assertEquals(oczekiwany, zwrócony);
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet e) {
            fail();
        }
    }

    @Test
    public void wycenaBlankietuStrzelanego() {
        try {
            // given
            Random r = new Random();

            TreeSet<Integer> Liczby = new TreeSet<>();

            while (Liczby.size() != 6) {
                Liczby.add(1 + r.nextInt(49));
            }

            Blankiet blankiet = new Blankiet(Liczby, 4);
            long oczekiwany = 4 * 3_00L;

            // when
            long zwrócony = blankiet.wyceńBlankiet();

            // then
            assertEquals(oczekiwany, zwrócony);
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet e) {
            fail();
        }
    }

    @Test
    public void wycenaBlankietuBłędnego() {
        try {
            // given
            Random r = new Random();

            Zakład[] Zakłady = new Zakład[8];

            for (int i = 0; i < 8; i++) {
                TreeSet<Integer> Strzały = new TreeSet<>();

                while (Strzały.size() != 10) {
                    Strzały.add(1 + r.nextInt(49));
                }

                Zakłady[i] = new Zakład(Strzały);
            }

            boolean[] liczbaLosowań = new boolean[]
                {false, true, true, true, false, false, false, false, true, false};

            Blankiet blankiet = new Blankiet(Zakłady, liczbaLosowań);
            long oczekiwany = 0L;

            // when
            long zwrócony = blankiet.wyceńBlankiet();

            // then
            assertEquals(oczekiwany, zwrócony);
        } catch (NiepoprawnyArgument | ŹleWypełnionyBlankiet e) {
            fail();
        }
    }

    @Test
    public void zaDużoZakładówWBlankiecie() {
        try {
            // given
            Zakład[] Zakłady = new Zakład[10];
            Random r = new Random();
            for (int i = 0; i < 10; i++) {
                TreeSet<Integer> Strzały = new TreeSet<>();

                while (Strzały.size() != 6) {
                    Strzały.add(1 + r.nextInt(49));
                }

                Zakład zakład = new Zakład(Strzały);
                Zakłady[i] = zakład;
            }
            boolean[] liczbaLosowań = new boolean[]
                {false, false, false, false, false, false, false, false, true, false};

            // then
            Blankiet blankiet = new Blankiet(Zakłady, liczbaLosowań);
            fail();
        } catch (NiepoprawnyArgument e) {
            fail();
        } catch (ŹleWypełnionyBlankiet ignored) {
        }
    }

    @Test
    public void nadmiaroweNulleWZakładach() {
        try {
            // given
            Zakład[] Zakłady = new Zakład[8];
            Random r = new Random();
            for (int i = 0; i < 6; i++) {
                TreeSet<Integer> Strzały = new TreeSet<>();

                while (Strzały.size() != 6) {
                    Strzały.add(1 + r.nextInt(49));
                }

                Zakład zakład = new Zakład(Strzały);
                Zakłady[i] = zakład;
            }

            Zakłady[6] = null;
            Zakłady[7] = null;
            boolean[] liczbaLosowań = new boolean[]
                    {false, true, false, false, false, false, false, false, true, false};

            // then
            Blankiet blankiet = new Blankiet(Zakłady, liczbaLosowań);
            fail();
        } catch (NiepoprawnyArgument e) {
            fail();
        } catch (ŹleWypełnionyBlankiet ignored) {
        }
    }

    @Test
    public void zaDużaTablicaLosowań() {
        try {
            // given
            Zakład[] Zakłady = new Zakład[8];
            Random r = new Random();
            for (int i = 0; i < 8; i++) {
                TreeSet<Integer> Strzały = new TreeSet<>();

                while (Strzały.size() != 6) {
                    Strzały.add(1 + r.nextInt(49));
                }

                Zakład zakład = new Zakład(Strzały);
                Zakłady[i] = zakład;
            }
            boolean[] liczbaLosowań = new boolean[]
                    {false, false, false, false, false, false, false, false, true, false, true, true, true};

            // then
            Blankiet blankiet = new Blankiet(Zakłady, liczbaLosowań);
            fail();
        } catch (NiepoprawnyArgument e) {
            fail();
        } catch (ŹleWypełnionyBlankiet ignored) {
        }
    }

    @Test
    public void zaDużoLiczbWZbiorze() {
        try {
            // given
            Random r = new Random();

            TreeSet<Integer> Liczby = new TreeSet<>();

            while (Liczby.size() != 10) {
                Liczby.add(1 + r.nextInt(49));
            }

            // then
            Blankiet blankiet = new Blankiet(Liczby, 4);
            fail();
        } catch (NiepoprawnyArgument e) {
            fail();
        } catch (ŹleWypełnionyBlankiet ignored) {
        }
    }

    @Test
    public void liczbyZeZłegoZakresu() {
        try {
            // given
            Random r = new Random();

            TreeSet<Integer> Liczby = new TreeSet<>();

            while (Liczby.size() != 5) {
                Liczby.add(1 + r.nextInt(49));
            }

            Liczby.add(1000);

            // then
            Blankiet blankiet = new Blankiet(Liczby, 4);
            fail();
        } catch (ŹleWypełnionyBlankiet e){
            fail();
        } catch (NiepoprawnyArgument ignored) {
        }
    }
}
