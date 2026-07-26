package loteria;

import wyjątki.NiepoprawnyArgument;

import java.util.TreeSet;

public class Zakład {
    private final boolean[][] czySkreślone;

    private final static long CENA_ZAKŁADU_BRUTTO = 3_00;
    private final static int LICZBA_ZAKREŚLEŃ = 6;
    private final static int WYSOKOŚĆ_ZAKŁADU = 5;
    private final static int SZEROKOŚĆ_ZAKŁADU = 10;


    public Zakład(boolean[][] czySkreślone) {
        if (czySkreślone == null) {
            throw new NullPointerException();
        }

        if (czySkreślone.length != WYSOKOŚĆ_ZAKŁADU || czySkreślone[0].length != SZEROKOŚĆ_ZAKŁADU) {
            throw new IllegalArgumentException();
        }

        this.czySkreślone = czySkreślone;
    }

    public Zakład(TreeSet<Integer> wytypowaneLiczby) throws NiepoprawnyArgument {
        if (wytypowaneLiczby == null) {
            throw new NullPointerException();
        }

        this.czySkreślone = new boolean[WYSOKOŚĆ_ZAKŁADU][SZEROKOŚĆ_ZAKŁADU];

        for (var liczba : wytypowaneLiczby) {
            int pomocnicza1 = (liczba - 1) / SZEROKOŚĆ_ZAKŁADU;
            int pomocnicza2 = (liczba - 1) % SZEROKOŚĆ_ZAKŁADU;

            if (pomocnicza1 > 5) {
                throw new NiepoprawnyArgument("Gracz podał zbyt duży numer.");
            }

            this.czySkreślone[pomocnicza1][pomocnicza2] = true;
        }
    }

    public long wyceńZakład() {
        int zaznaczone = 0;
        for (int i = 0; i < WYSOKOŚĆ_ZAKŁADU; i++) {
            for (int j = 0; j < SZEROKOŚĆ_ZAKŁADU; j++) {
                if (this.czySkreślone[i][j]) {
                    zaznaczone++;
                }
            }
        }

        if (zaznaczone == LICZBA_ZAKREŚLEŃ) {
            return CENA_ZAKŁADU_BRUTTO;
        } else {
            return 0;
        }

    }

    TreeSet<Integer> dajWytypowaneLiczby() {
        TreeSet<Integer> WytypowaneLiczby = new TreeSet<>();

        for (int i = 0; i < WYSOKOŚĆ_ZAKŁADU; i++) {
            for (int j = 0; j < SZEROKOŚĆ_ZAKŁADU; j++) {
                if (this.czySkreślone[i][j]) {
                    if (i == 4 && j == 9) {
                        return null;
                        // wtedy kupon jest anulowany
                    } else {
                        WytypowaneLiczby.add(i * SZEROKOŚĆ_ZAKŁADU + j + 1);
                    }
                }
            }
        }

        if (WytypowaneLiczby.size() != LICZBA_ZAKREŚLEŃ) {
            return null;
        }

        return WytypowaneLiczby;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();

        for (int i = 0; i < WYSOKOŚĆ_ZAKŁADU; i++) {
            for (int j = 0; j < SZEROKOŚĆ_ZAKŁADU; j++) {
                if (i != 4 || j != 9) {
                    sb.append(" [ ");
                    if (this.czySkreślone[i][j]) {
                        sb.append("--");
                    } else {
                        if (i * SZEROKOŚĆ_ZAKŁADU + j + 1 < SZEROKOŚĆ_ZAKŁADU) {
                            sb.append(" ").append(j + 1);
                        } else {
                            sb.append(i * SZEROKOŚĆ_ZAKŁADU + j + 1);
                        }
                    }

                    sb.append(" ] ");
                } else {
                    sb.append("\n [    ] anuluj");
                }
            }

            sb.append("\n");
        }

        return sb.toString();
    }
}
