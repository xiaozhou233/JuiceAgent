import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class RedefineClass {
    public static void main(String[] args) {
        TestSupport.run("RedefineClass", () -> {
            TestSupport.bootstrap();

            Target.targetTest();

            byte[] bytes = TestSupport.readClass("Target");
            TestSupport.require(JuiceAgent.redefineClass(Target.class, bytes, bytes.length),
                    "redefineClass returned false");

            String out = TestSupport.capture(Target::targetTest);
            TestSupport.require(out.contains("Modified!"), "redefinition had no effect: " + out);
        });
    }
}
