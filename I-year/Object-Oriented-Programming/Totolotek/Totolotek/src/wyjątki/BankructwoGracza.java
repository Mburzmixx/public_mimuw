package wyjątki;

public class BankructwoGracza extends RuntimeException {
    public BankructwoGracza(String message) {
        super(message);
    }
}
