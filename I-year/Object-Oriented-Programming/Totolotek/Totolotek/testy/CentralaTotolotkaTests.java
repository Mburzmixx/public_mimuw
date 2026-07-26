import loteria.*;
import typy_graczy.*;
import wyjątki.*;
import org.junit.jupiter.api.Test;

import java.util.Random;
import java.util.TreeSet;

import static org.junit.jupiter.api.Assertions.*;

public class CentralaTotolotkaTests {
    @Test
    public void wyliczanieNagródZaStopniePodstawowe() {
        try {
            // given

            BudżetPaństwa budżetPaństwa = new BudżetPaństwa();
            Media media = new Media();

            CentralaTotolotka centralaTotolotka = new CentralaTotolotka(budżetPaństwa, media);

            Kolektura[] kolektury = new Kolektura[2];

            kolektury[0] = new Kolektura(centralaTotolotka);
            kolektury[1] = new Kolektura(centralaTotolotka);

            Losowy ochotnik = new Losowy("Mateo", "Stanisławski", "05389123452",
                    1_000_000_000_00L, media);

            for (int i = 0; i < 2_000_000; i++) {
                ochotnik.kupKupon(1, 1, kolektury[0]);
            }

            centralaTotolotka.przeprowadźLosowanie();

            long oczekiwany1 = 2_112_000_00L;
            if (centralaTotolotka.dajLiczbęWygranychStopnia(1, 1) != 0) {
                oczekiwany1 /= centralaTotolotka.dajLiczbęWygranychStopnia(1,1);
            }

            long oczekiwany2 = 384_000_00L;
            if (centralaTotolotka.dajLiczbęWygranychStopnia(1, 2) != 0) {
                oczekiwany2 /= centralaTotolotka.dajLiczbęWygranychStopnia(1, 2);
            }

            long oczekiwany4 = 24_00L;

            long pulaCałkowita = 2_448_000_00L;

            long pomocnicza1 = oczekiwany1 * centralaTotolotka.dajLiczbęWygranychStopnia(1, 1);
            if (centralaTotolotka.dajLiczbęWygranychStopnia(1, 1) == 0) {
                pomocnicza1 = oczekiwany1;
            }

            long pomocnicza2 = oczekiwany2 * centralaTotolotka.dajLiczbęWygranychStopnia(1, 2);
            if (centralaTotolotka.dajLiczbęWygranychStopnia(1, 2) == 0) {
                pomocnicza2 = oczekiwany2;
            }

            long pomocnicza4 = oczekiwany4 * centralaTotolotka.dajLiczbęWygranychStopnia(1, 4);
            if (centralaTotolotka.dajLiczbęWygranychStopnia(1, 4) == 0) {
                pomocnicza4 = oczekiwany4;
            }


            long oczekiwany3 = pulaCałkowita - pomocnicza1 - pomocnicza2 - pomocnicza4;
            if (centralaTotolotka.dajLiczbęWygranychStopnia(1, 3) != 0) {
                oczekiwany3 /= centralaTotolotka.dajLiczbęWygranychStopnia(1, 3);
            }

            oczekiwany3 = Math.max(oczekiwany3, 36_00L);


            // when
            long zwrócony1 = centralaTotolotka.dajNagrodęZaStopień(1, 1);
            long zwrócony2 = centralaTotolotka.dajNagrodęZaStopień(1, 2);
            long zwrócony3 = centralaTotolotka.dajNagrodęZaStopień(1, 3);
            long zwrócony4 = centralaTotolotka.dajNagrodęZaStopień(1, 4);

            // then
            assertEquals(oczekiwany1, zwrócony1);
            assertEquals(oczekiwany2, zwrócony2);
            assertEquals(oczekiwany3, zwrócony3);
            assertEquals(oczekiwany4, zwrócony4);
        } catch (NiepoprawnyArgument | NieznanyIndeks e) {
            fail();
        }
    }

    @Test
    public void wyliczanieNagródZaStopnieZKumulacją() {
        try {
            // given
            Random r = new Random();
            TreeSet<Integer> Liczby = new TreeSet<>();

            while (Liczby.size() < 6) {
                Liczby.add(1 + r.nextInt(49));
            }

            Blankiet blankiet = new Blankiet(Liczby, 1);

            BudżetPaństwa budżetPaństwa = new BudżetPaństwa();
            Media media = new Media();

            CentralaTotolotka centralaTotolotka = new CentralaTotolotka(budżetPaństwa, media);

            Kolektura[] kolektury = new Kolektura[2];

            kolektury[0] = new Kolektura(centralaTotolotka);
            kolektury[1] = new Kolektura(centralaTotolotka);

            Stałoblankietowy ochotnik = new Stałoblankietowy("Mateo", "Stanisławski", "05389123452",
                    2_000_000_000_00L, 1, blankiet, kolektury, 1000);
            // Tutaj na potrzeby testów będę stosował następującą możliwość:
            // pozwalam stałoblankietowym na odejście od swojego blankietu,
            // jeśli będą chcieli kupić bilet nadprogramowo.

            // Aby nie zaburzać obliczeń, dałem duży cykl kupowania.

            for (int i = 0; i < 2_000_000; i++) {
                ochotnik.kupKupon(1, 1, kolektury[0]);
            }

            centralaTotolotka.przeprowadźLosowanie();

            boolean czyKtośRozbiłBank = centralaTotolotka.dajLiczbęWygranychStopnia(1, 1) != 0;
            long pula1 = 2_112_000_00L;


            for (int i = 0; i < 2_000_000; i++) {
                ochotnik.kupKupon(1, 1, kolektury[1]);
            }

            centralaTotolotka.przeprowadźLosowanie();

            long oczekiwany1 = 2_112_000_00L;
            if (centralaTotolotka.dajLiczbęWygranychStopnia(2, 1) != 0) {
                oczekiwany1 /= centralaTotolotka.dajLiczbęWygranychStopnia(2,1);
            }

            long oczekiwany2 = 384_000_00L;
            if (centralaTotolotka.dajLiczbęWygranychStopnia(2, 2) != 0) {
                oczekiwany2 /= centralaTotolotka.dajLiczbęWygranychStopnia(2, 2);
            }

            long oczekiwany4 = 24_00L;

            long pulaCałkowita = 2_448_000_00L;

            long pomocnicza1 = oczekiwany1 * centralaTotolotka.dajLiczbęWygranychStopnia(2, 1);
            if (centralaTotolotka.dajLiczbęWygranychStopnia(2, 1) == 0) {
                pomocnicza1 = oczekiwany1;
            }

            long pomocnicza2 = oczekiwany2 * centralaTotolotka.dajLiczbęWygranychStopnia(2, 2);
            if (centralaTotolotka.dajLiczbęWygranychStopnia(2, 2) == 0) {
                pomocnicza2 = oczekiwany2;
            }

            long pomocnicza4 = oczekiwany4 * centralaTotolotka.dajLiczbęWygranychStopnia(2, 4);
            if (centralaTotolotka.dajLiczbęWygranychStopnia(2, 4) == 0) {
                pomocnicza4 = oczekiwany4;
            }


            long oczekiwany3 = pulaCałkowita - pomocnicza1 - pomocnicza2 - pomocnicza4;
            if (centralaTotolotka.dajLiczbęWygranychStopnia(2, 3) != 0) {
                oczekiwany3 /= centralaTotolotka.dajLiczbęWygranychStopnia(2, 3);
            }

            oczekiwany3 = Math.max(oczekiwany3, 36_00L);

            if (!czyKtośRozbiłBank) {
                oczekiwany1 += pula1;
            }

            // when
            long zwrócony1 = centralaTotolotka.dajNagrodęZaStopień(2, 1);
            long zwrócony2 = centralaTotolotka.dajNagrodęZaStopień(2, 2);
            long zwrócony3 = centralaTotolotka.dajNagrodęZaStopień(2, 3);
            long zwrócony4 = centralaTotolotka.dajNagrodęZaStopień(2, 4);

            // then
            assertEquals(oczekiwany1, zwrócony1);
            assertEquals(oczekiwany2, zwrócony2);
            assertEquals(oczekiwany3, zwrócony3);
            assertEquals(oczekiwany4, zwrócony4);
        } catch (NiepoprawnyArgument | NieznanyIndeks | ŹleWypełnionyBlankiet e) {
            fail();
        }
    }
}
