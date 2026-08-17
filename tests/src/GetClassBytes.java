import cn.xiaozhou233.juiceagent.api.JuiceAgent;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

public class GetClassBytes {
    public static void main(String[] args) throws IOException {
        System.load(new File("resources/libagent.dll").getAbsolutePath());
        JuiceAgent.init("");

        byte[] bytes = JuiceAgent.getClassBytes(Target.class);

        File output = new File("tmp", Target.class.getName().replace('.', '/') + ".class");

        File parent = output.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }

        try (FileOutputStream fos = new FileOutputStream(output)) {
            fos.write(bytes);
        }

        System.out.println("Class: " + Target.class.getName());
        System.out.println("Bytes: " + bytes.length);
        System.out.println("Saved: " + output.getAbsolutePath());
    }
}