import org.nordugrid.arc.*;

public class test {
    static {
        // load libjarc.so
        System.loadLibrary("jarc");
    }
    public static void main(String argv[]) {
        Config c = new Config("../../src/tests/echo/service.xml");
        c.print();
    }
}
