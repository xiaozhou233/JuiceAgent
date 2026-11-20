import cn.xiaozhou233.juiceagent.api.JuiceAgent;

public class RetransformClassByName {
    public static void main(String[] args) {
        TestSupport.run("RetransformClassByName", () -> {
            TestSupport.bootstrap();

            Target.targetTest();

            byte[] bytes = TestSupport.readClass("Target");
            TestSupport.require(JuiceAgent.retransformClassByName("Target", bytes, bytes.length),
                    "retransformClassByName returned false");

            String out = TestSupport.capture(Target::targetTest);
            TestSupport.require(out.contains("Modified!"), "retransformation had no effect: " + out);
        });
    }
}
