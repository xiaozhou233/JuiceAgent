import cn.xiaozhou233.juiceagent.api.JuiceAgent;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.PrintStream;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Shared plumbing for the integration tests.
 *
 * <p>Every test prints exactly one verdict line: {@code RESULT: PASS <name>} or
 * {@code RESULT: FAIL <name>: <reason>}. The runner (run-tests.ps1 / run-tests.sh)
 * keys off those lines, so the tests can assert on native return values and on
 * captured program output instead of a brittle marker in the runner's own table.
 *
 * <p>These are still plain {@code main()} programs - no test framework.
 */
public final class TestSupport {
    private TestSupport() {}

    // Windows ships libagent.dll, Linux ships libagent.so; pick per OS.
    private static final String NATIVE_LIB =
            System.getProperty("os.name", "").toLowerCase().contains("win")
                    ? "libagent.dll"
                    : "libagent.so";

    private static boolean bootstrapped = false;

    /** Loads the native agent and initialises JuiceAgent (once per process). */
    public static void bootstrap() {
        if (bootstrapped) {
            return;
        }
        System.load(new File("resources", NATIVE_LIB).getAbsolutePath());
        require(JuiceAgent.init(""), "JuiceAgent.init returned false");
        bootstrapped = true;
    }

    /** Runs a test body and prints the single PASS/FAIL verdict line. */
    public static void run(String name, Body body) {
        try {
            body.run();
            System.out.println("RESULT: PASS " + name);
        } catch (Throwable t) {
            System.out.println("RESULT: FAIL " + name + ": " + t);
            t.printStackTrace(System.err);
        }
    }

    public static void require(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }

    /** Absolute path of the injection.jar the AddTo* tests append. */
    public static String injectionJar() {
        return new File("injection.jar").getAbsolutePath();
    }

    /** Reads a class file produced by the injection module build. */
    public static byte[] readClass(String name) {
        Path path = Path.of("out", "production", "injection", name.replace('.', '/') + ".class");
        try {
            return Files.readAllBytes(path);
        } catch (Exception e) {
            throw new AssertionError("cannot read " + path + ": " + e);
        }
    }

    /** A compiled class starts with the JVM magic number 0xCAFEBABE. */
    public static boolean looksLikeClassFile(byte[] bytes) {
        return bytes != null && bytes.length >= 4
                && bytes[0] == (byte) 0xCA && bytes[1] == (byte) 0xFE
                && bytes[2] == (byte) 0xBA && bytes[3] == (byte) 0xBE;
    }

    /** Invokes a public static no-arg method, failing the test on error. */
    public static void invokeStatic(Class<?> clazz, String method) {
        try {
            clazz.getMethod(method).invoke(null);
        } catch (Exception e) {
            throw new AssertionError("invoking " + clazz.getName() + "." + method + " failed", e);
        }
    }

    /** Captures everything {@code body} writes to {@code System.out}. */
    public static String capture(Runnable body) {
        PrintStream original = System.out;
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        try (PrintStream capture = new PrintStream(buffer, true)) {
            System.setOut(capture);
            body.run();
        } finally {
            System.setOut(original);
        }
        return buffer.toString();
    }

    @FunctionalInterface
    public interface Body {
        void run() throws Exception;
    }
}
