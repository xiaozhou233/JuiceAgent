import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class AddToSystemClassLoaderSearch {
    public static void main(String[] args) {
        TestSupport.run("AddToSystemClassLoaderSearch", () -> {
            TestSupport.bootstrap();

            TestSupport.require(JuiceAgent.addToSystemClassLoaderSearch(TestSupport.injectionJar()),
                    "addToSystemClassLoaderSearch returned false");

            Class<?> clazz = Class.forName("Injection");
            String out = TestSupport.capture(() -> TestSupport.invokeStatic(clazz, "test"));
            TestSupport.require(out.contains("Injection Invoked!"),
                    "Injection.test() did not run: " + out);

            System.out.println("ClassLoader: " + clazz.getClassLoader());
        });
    }
}
