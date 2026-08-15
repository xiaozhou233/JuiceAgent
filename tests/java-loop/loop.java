public class loop {
    public static void main(String[] args) {
        while (true) {
            try {
                System.out.println("Hello, World!");
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}