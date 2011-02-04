import nordugrid.arc.*;

/**
 * Regression test.
 * Garbage collection in Java does not respect scope, but rather tries to find
 * out when an object is not used any more. Thus a object might be garbage
 * collected before it goes out of scope.
 * For Java proxy classes wrapping a C++ class containing a method which returns
 * a C++ reference the issue above is important. When the java method, wrapping
 * the C++ method returning a C++ reference, returns an object (R) the memory of
 * that object (R) (C++ memory) is managed by the object (A) of the called
 * method. If object A is not used further inside the scope A can be garbage
 * collected, and if that happens object R becomes invalid (C++ memory has been
 * freed).
 *
 * This test checks whether memory management with respect to returning C++
 * references is done properly, such that objects still in use will not be
 * deleted.
 */
public class ReturnLocalReferenceRGTest
{
  static {
    System.loadLibrary("jarc");
  }

  public ReturnLocalReferenceRGTest() {}

  public static void main(String argv[]) throws InterruptedException
  {
    for (int i = 0; i < 10000; i++) {
      // Use a class for which there is a C++ method returning a reference to a private member.
      JobDescription jobdesc = new JobDescription();
      jobdesc.AddAlternative(new JobDescription());
      JobDescriptionList jlist = jobdesc.GetAlternatives(); // The C++ method jobdesc.GetAlternatives returns a reference to a private member.
      // Object jobdesc is not used further below, therefore it can be garbage collected if not referenced inside the JobDescriptionList object.

      // Fill memory to increase chances of garbage collection.
      for (int j = 0; j < 5000; j++) {
        new ReturnLocalReferenceRGTest();
      }

      //Thread.sleep(5);

      // Looping over the jlist must not produce a segmentation fault.
      for (JobDescriptionListIteratorHandler it = new JobDescriptionListIteratorHandler(jlist.begin());
           !it.equal(jlist.end()); it.next()) {
        // Fill memory to increase chances of garbage collection.
        for (int j = 0; j < 5000; j++) {
          new ReturnLocalReferenceRGTest();
        }
      }
    }

    return;
  }
}
