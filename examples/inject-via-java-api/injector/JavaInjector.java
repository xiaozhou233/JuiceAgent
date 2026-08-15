import cn.xiaozhou233.juiceagent.injector.Injector;
import cn.xiaozhou233.juiceagent.injector.WindowInfo;

import java.util.Scanner;
import java.io.File;

public class JavaInjector {
    public static void main(String[] args) {
        // Load the native library
        System.load(new File("./libinject.dll").getAbsolutePath());

        // Find windows by title
        for (WindowInfo window : Injector.findWindowsByTitle("cmd")) {
            System.out.println(window.toString());
        }
        System.out.println("----------------");
        
        Scanner scanner = new Scanner(System.in);

        // Input
        System.out.println("PID: ");
        String pid = scanner.nextLine();
        System.out.println("Dll: ");
        String dll = scanner.nextLine();
        System.out.println("config_path: ");
        String config_path = scanner.nextLine();

        if (config_path == null || config_path.isEmpty()) {
            // config_path is empty
            Injector.inject(Integer.parseInt(pid), dll);
        } else {
            Injector.inject(Integer.parseInt(pid), dll, config_path);
        }
    }
}