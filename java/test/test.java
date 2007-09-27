import nordugrid.arc.*;

public class test {
    static {
        // load libjarc.so
        System.loadLibrary("jarc");
    }
    public static void main(String argv[]) {
       /* Config c = new Config("../../src/tests/echo/service.xml");
        c.print(); */
        // System.out.println(StatusKind.STATUS_OK);
        /* MCC_Status s = new MCC_Status(); */
        MCC_Status s = new MCC_Status(StatusKind.STATUS_OK);
        System.out.println(s.getKind());
    }
}
