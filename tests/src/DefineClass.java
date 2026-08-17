import cn.xiaozhou233.juiceagent.api.JuiceAgent;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;

public class DefineClass {
    public static void main(String[] args) throws IOException {
        System.load(new File("resources/libagent.dll").getAbsolutePath());
        JuiceAgent.init("");

        ClassLoader cl = new ClassLoader() {};

        invokeTest(cl);

        byte[] bytes = Files.readAllBytes(new File("out/production/injection/Injection.class").toPath());
        System.out.println("Read bytes: " + bytes.length);

        JuiceAgent.defineClass(cl, bytes);

        invokeTest(cl);

    }

    public static void invokeTest(ClassLoader cl) {
        try {
            Class<?> clazz = cl.loadClass("Injection");
            clazz.getMethod("test").invoke(null);
            System.out.println("ClassLoader: " + clazz.getClassLoader().toString());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
