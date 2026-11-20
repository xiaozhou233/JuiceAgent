import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class RetransformClass {
    public static void main(String[] args) {
        TestSupport.run("RetransformClass", () -> {
            TestSupport.bootstrap();

            Target.targetTest();

            byte[] bytes = TestSupport.readClass("Target");
            TestSupport.require(JuiceAgent.retransformClass(Target.class, bytes, bytes.length),
                    "retransformClass returned false");

            String out = TestSupport.capture(Target::targetTest);
            TestSupport.require(out.contains("Modified!"), "retransformation had no effect: " + out);
        });
    }
}
