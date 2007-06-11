import org.nordugrid.arc.*;

public class EchoService {
    private NS ns_;
    static {
            // load libjarc.so
            System.loadLibrary("jarc");
    }

    public EchoService() {
        System.out.println("EchoService constructor called");
        NS ns_ = new NS();
        ns_.set("echo", "urn:echo");
    }

    public int process() {
        System.out.println("EchoService process called");
        return 10;
    }

    public MCC_Status process(SOAPMessage inmsg, SOAPMessage outmsg) {
        System.out.println("EchoService process with messages called");
        // XXX: error handling
        XMLNode echo_op = new XMLNode(inmsg.Get("echo"));
        String say = new String(echo_op.Get("say").toString());
        String hear = new String(say);
        outmsg = new SOAPMessage(this.ns_);
        outmsg.NewChild("echo:echoResponse").NewChild("echo:hear").Set(hear);
        return new MCC_Status(StatusKind.STATUS_OK);
    }
}
