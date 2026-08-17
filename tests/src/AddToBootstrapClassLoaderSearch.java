import java.io.File;
import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class AddToBootstrapClassLoaderSearch {
    public static void main(String[] args) {
        System.load(new File("resources/libagent.dll").getAbsolutePath());
        JuiceAgent.init("");

        System.out.println("========= Before =========");
        invokeTest();
        System.out.println("========= After =========");
        JuiceAgent.addToBootstrapClassLoaderSearch(new File("Injection.jar").getAbsolutePath());
        invokeTest();
        System.out.println("========= Done =========");

    }

    public static void invokeTest() {
        try {
            Class<?> clazz = Class.forName("Injection");
            ClassLoader loader = clazz.getClassLoader();
            System.out.println("ClassLoader: " + (loader == null ? "BootstrapClassLoader" : loader));
            clazz.getMethod("test").invoke(null);
        } catch (ClassNotFoundException e) {
            System.err.println("Injection class not found: " + e.getMessage());

        } catch (ReflectiveOperationException e) {
            System.err.println("Invoke failed: " + e.getMessage());
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
