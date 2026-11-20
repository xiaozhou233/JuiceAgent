import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class AddToBootstrapClassLoaderSearch {
    public static void main(String[] args) {
        TestSupport.run("AddToBootstrapClassLoaderSearch", () -> {
            TestSupport.bootstrap();

            TestSupport.require(JuiceAgent.addToBootstrapClassLoaderSearch(TestSupport.injectionJar()),
                    "addToBootstrapClassLoaderSearch returned false");

            Class<?> clazz = Class.forName("Injection");
            TestSupport.require(clazz.getClassLoader() == null,
                    "Injection was not loaded by the bootstrap loader");

            String out = TestSupport.capture(() -> TestSupport.invokeStatic(clazz, "test"));
            TestSupport.require(out.contains("Injection Invoked!"),
                    "Injection.test() did not run: " + out);

            System.out.println("ClassLoader: BootstrapClassLoader");
        });
    }
}
