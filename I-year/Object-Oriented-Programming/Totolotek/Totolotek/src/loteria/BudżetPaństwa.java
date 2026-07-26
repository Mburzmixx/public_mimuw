package loteria;

public class BudżetPaństwa {
    private long pobranePodatki = 0;
    private long przekazaneSubwencje = 0;


    void udzielSubwencji(long kwota) {
        this.przekazaneSubwencje += kwota;
    }

    void przyjmijPodatek(long kwota) {
        this.pobranePodatki += kwota;
    }

    public long dajPobranePodatki() {
        return this.pobranePodatki;
    }

    public long dajPrzekazaneSubwencje() {
        return this.przekazaneSubwencje;
    }
}
