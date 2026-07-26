package loteria;

import wyjątki.NieznanyIndeks;

import java.util.Random;
import java.util.TreeSet;

public class Losowanie {
    private final int numerPorządkowy;

    private long[] KwotyWygranych;
    private long[] PulaNagród;
    private long[] ZwycięskieZakłady;
    private final TreeSet<Integer> WynikLosowania;

    private final static int PRZEDZIAŁ_STOPNI_LK = 1;
    private final static int PRZEDZIAŁ_STOPNI_PK = 4;



    Losowanie(int numerPorządkowy){
        this.numerPorządkowy = numerPorządkowy;

        TreeSet<Integer> wynik = new TreeSet<>();

        Random r = new Random();
        while (wynik.size() < 6) {
            wynik.add(1 + r.nextInt(49));
        }

        this.WynikLosowania = wynik;

    }


    TreeSet<Integer> dajZwycięskąSzóstkę() {
        return this.WynikLosowania;
    }

    void ustawZwycięskieZakłady(long[] ZwycięskieZakłady) {
        this.ZwycięskieZakłady = ZwycięskieZakłady;
    }

    void ustawPulęNagród(long[] PulaNagród) {
        this.PulaNagród = PulaNagród;
    }

    void ustawKwotyWygranych(long[] KwotyWygranych) {
        this.KwotyWygranych = KwotyWygranych;
    }

    long dajKwotęWygranej(int stopień) throws NieznanyIndeks {
        if (stopień < PRZEDZIAŁ_STOPNI_LK || PRZEDZIAŁ_STOPNI_PK < stopień) {
            throw new NieznanyIndeks("Podejrzany stopień nagrody");
        }

        return this.KwotyWygranych[stopień];
    }

    long dajLiczbęWygranych(int stopnień) {
        // Używane tylko do testów.
        return this.ZwycięskieZakłady[stopnień];
    }

    int dajNumerLosowania() {
        return this.numerPorządkowy;
    }

    public String dajWynikiLosowania() {
        StringBuilder sb = new StringBuilder();
        sb.append(this);
        sb.append("Łączna pula nagród po stopniach: ");
        sb.append("\n");

        for (var pula : this.PulaNagród) {
            sb.append(pula / 100);
            sb.append(" złoty, ");
            sb.append(pula % 100);
            sb.append(" groszy.");
            sb.append("\n");
        }

        sb.append("Liczba zwycięskich zakładów po stopniach: ");

        for (var liczba : this.ZwycięskieZakłady) {
            sb.append(liczba);
            sb.append(", ");
        }
        sb.append("\n");
        sb.append("Wiodące zera reprezentują kupon który nie trafił nawet 3 liczb.").append("\n");

        return sb.toString();
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Losowanie nr ").append(this.numerPorządkowy).append("\n");
        sb.append("Wyniki: ");
        for (var liczba: this.WynikLosowania) {
            if (liczba < 10) {
                sb.append(" ").append(liczba);
            } else {
                sb.append(liczba);
            }
            sb.append(" ");
        }

        sb.append("\n");

        return sb.toString();
    }
}
