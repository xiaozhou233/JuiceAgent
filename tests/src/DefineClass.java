import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class DefineClass {
    public static void main(String[] args) {
        TestSupport.run("DefineClass", () -> {
            TestSupport.bootstrap();

            ClassLoader cl = new ClassLoader() {};

            byte[] bytes = TestSupport.readClass("Injection");
            System.out.println("Read bytes: " + bytes.length);

            Class<?> clazz = JuiceAgent.defineClass(cl, bytes);
            TestSupport.require(clazz != null, "defineClass returned null");
            TestSupport.require(clazz.getClassLoader() == cl, "defined class has the wrong class loader");

            String out = TestSupport.capture(() -> TestSupport.invokeStatic(clazz, "test"));
            TestSupport.require(out.contains("Injection Invoked!"),
                    "Injection.test() did not run: " + out);

            System.out.println("ClassLoader: " + clazz.getClassLoader());
        });
    }
}
