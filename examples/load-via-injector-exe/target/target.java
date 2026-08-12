import java.lang.management.ManagementFactory;

public class target {
    public static void main(String[] args) {
        String pid = ManagementFactory.getRuntimeMXBean().getName().split("@")[0];
        try {
            while (true) {
                System.out.println("[Target] I am the target JVM (PID: " + pid + ")");
                Thread.sleep(1000);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
