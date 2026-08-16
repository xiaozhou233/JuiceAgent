public class Loop {
    public static void echo() {
        System.out.println("Loop!");
    }

    public static void main(String[] args) throws InterruptedException {
        while(true) {
            echo();
            Thread.sleep(1000);
        }
    }
}