public class loop {
    public static void main(String[] args) {
        try {
            while(true) {
                System.out.println("Loop!");
                Thread.sleep(1000);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}