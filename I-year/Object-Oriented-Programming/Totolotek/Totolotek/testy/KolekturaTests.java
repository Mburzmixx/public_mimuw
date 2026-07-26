import loteria.*;
import typy_graczy.Losowy;
import wyjątki.BankructwoGracza;
import wyjątki.NiepoprawnyArgument;
import wyjątki.NieznanyIndeks;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

public class KolekturaTests {
    @Test
    public void sprzedanieKuponuZNiepoprawnegoBlankietu() {

    }

    @Test
    public void graczNieMaPieniędzyNaKupienie() {
        // given
        Media media = new Media();
        BudżetPaństwa budżetPaństwa = new BudżetPaństwa();

        CentralaTotolotka centralaTotolotka = new CentralaTotolotka(budżetPaństwa, media);

        Kolektura kolektura = new Kolektura(centralaTotolotka);

        Losowy ochotnik = new Losowy("Mateo", "Stanisławski", "02421945678",
                1_00L, media);

        // then
        try {
            ochotnik.kupKupon(1,1, kolektura);
            fail();
        } catch (NiepoprawnyArgument e) {
            fail();
        } catch (BankructwoGracza ignored) {
        }
    }

    @Test
    public void sprzedanieKuponu() {
        // given
        Media media = new Media();
        BudżetPaństwa budżetPaństwa = new BudżetPaństwa();

        CentralaTotolotka centralaTotolotka = new CentralaTotolotka(budżetPaństwa, media);

        Kolektura kolektura = new Kolektura(centralaTotolotka);

        Losowy ochotnik = new Losowy("Mateo", "Stanisławski", "02421945678",
                4_00L, media);
        long oczekiwanyBudżet = 2_40L;
        long oczekiwanyPodatek = 60L;
        long oczekiwanyStanKonta = 1_00L;

        // when
        try {
            ochotnik.kupKupon(1, 1, kolektura);

            long zwróconyBudżet = centralaTotolotka.dajBudżetLosowania(1);
            long zwróconyPodatek = budżetPaństwa.dajPobranePodatki();
            long zwróconyStanKonta = ochotnik.dajStanKonta();

            // then
            assertEquals(oczekiwanyBudżet, zwróconyBudżet);
            assertEquals(oczekiwanyPodatek, zwróconyPodatek);
            assertEquals(oczekiwanyStanKonta, zwróconyStanKonta);
        } catch (NiepoprawnyArgument | NieznanyIndeks e) {
            fail();
        }
    }
}
