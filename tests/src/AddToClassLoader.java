import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class AddToClassLoader {
    public static void main(String[] args) {
        TestSupport.run("AddToClassLoader", () -> {
            TestSupport.bootstrap();

            ClassLoader cl = new ClassLoader() {};

            TestSupport.require(JuiceAgent.addToClassLoader(TestSupport.injectionJar(), cl),
                    "addToClassLoader returned false");

            // A plain ClassLoader is not a URLClassLoader, so the API falls back
            // to appending the jar to the system class loader search path.
            Class<?> clazz = cl.loadClass("Injection");
            String out = TestSupport.capture(() -> TestSupport.invokeStatic(clazz, "test"));
            TestSupport.require(out.contains("Injection Invoked!"),
                    "Injection.test() did not run: " + out);

            System.out.println("ClassLoader: " + clazz.getClassLoader());
        });
    }
}
