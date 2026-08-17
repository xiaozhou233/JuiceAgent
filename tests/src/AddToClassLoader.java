import cn.xiaozhou233.juiceagent.api.JuiceAgent;

import java.io.File;
import java.lang.reflect.InvocationTargetException;

public class AddToClassLoader {
    public static void main(String[] args) throws ClassNotFoundException, NoSuchMethodException, InvocationTargetException, IllegalAccessException {
        System.load(new File("resources/libagent.dll").getAbsolutePath());
        JuiceAgent.init("");

        ClassLoader cl = new ClassLoader(){};

        invokeTest(cl);

        JuiceAgent.addToClassLoader(new File("Injection.jar").getAbsolutePath(), cl);

        invokeTest(cl);

    }

    public static void invokeTest(ClassLoader cl) {
        try {
            Class<?> clz = cl.loadClass("Injection");
            System.out.println("ClassLoader: " + clz.getClassLoader().toString());
            clz.getMethod("test").invoke(null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
