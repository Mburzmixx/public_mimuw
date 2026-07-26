package loteria;

import java.util.ArrayList;
import java.util.TreeSet;

public class Media {
    private final ArrayList<Gracz> WszyscyGracze = new ArrayList<>();
    private CentralaTotolotka centrala;

    public void powiadomOWynikach(TreeSet<Integer> zwycięskaSzóstka, int numerLosowania) {
        for (var gracz : WszyscyGracze) {
            gracz.usłyszInformację(zwycięskaSzóstka, numerLosowania);
        }
    }

    public Kolektura dajLosowąKolekturę() {
        return centrala.dajLosowąKolekturę();
    }

    void nawiążKontaktZCentraląTotolotka(CentralaTotolotka centrala) {
        this.centrala = centrala;
    }

    public void dodajNowegoGracza(Gracz gracz) {
        this.WszyscyGracze.add(gracz);
    }
}
