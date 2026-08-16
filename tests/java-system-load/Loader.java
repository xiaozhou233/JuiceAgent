import java.io.File;
import java.io.IOException;

public class Loader {
    public static void main(String[] args) {
        System.load(new File("./libinject.dll").getAbsolutePath());
        System.out.println("Loaded libinject.dll");
        System.out.println("====================================");
        System.load(new File("./libloader.dll").getAbsolutePath());
        System.out.println("Loaded libloader.dll");
        System.out.println("====================================");
        System.load(new File("./libagent.dll").getAbsolutePath());
        System.out.println("Loaded libagent.dll");
        
    }
}