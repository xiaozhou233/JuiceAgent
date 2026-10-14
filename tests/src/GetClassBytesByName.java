import cn.xiaozhou233.juiceagent.api.JuiceAgent;

import java.io.File;
import java.io.FileOutputStream;

public class GetClassBytesByName {
    public static void main(String[] args) throws Exception {
        System.load(new File("resources/libagent.dll").getAbsolutePath());
        JuiceAgent.init("");

        // getClassBytesByName only resolves classes that are already loaded,
        // so make sure the target class is loaded before looking it up.
        Class.forName("Target");

        String className = "Target";
        byte[] bytes = JuiceAgent.getClassBytesByName(className);

        if (bytes == null || bytes.length == 0) {
            System.out.println("Failed to get class bytes: " + className);
            return;
        }

        // Convert JVM internal class name to a file path.
        String fileName = className.replace('.', '/').replace('$', '_') + ".class";
        File output = new File("tmp", fileName);

        File parent = output.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }

        try (FileOutputStream out = new FileOutputStream(output)) {
            out.write(bytes);
        }

        System.out.println("Class: " + className);
        System.out.println("Bytes: " + bytes.length);
        System.out.println("Saved: " + output.getAbsolutePath());
    }
}