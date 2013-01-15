import java.lang.reflect.Method;
import java.lang.reflect.Constructor;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import junit.framework.Assert.*;

import nordugrid.arc.Time;
import nordugrid.arc.Period;

public class MappingOf_time_t_and_uint32_t_CTypesToJavaRegressionTest extends TestCase
{
    public void checkMappedTypeOf_time_t_CType() {
        try {
          Class[] types = new Class[1];
          types[0] = long.class;
          Time.class.getConstructor(types);
        } catch (Exception e) {
          fail("In class Time: Constructor taking type \"long\" as argument was expected to exist but it was not found.");
        }
        try {
          assertEquals("In class Time: Method \"GetTime\" was expected to return a value of type \"long\".",
                       Long.TYPE, Time.class.getMethod("GetTime", new Class[0]).getReturnType());
        } catch (Exception e) {
          fail("In class Time: Method \"GetTime\" was expected to exist but it was not found.");
        }

        try {
          Class[] types = new Class[1];
          types[0] = long.class;
          Period.class.getConstructor(types);
        } catch (Exception e) {
          fail("In class Period: Constructor taking type \"long\" as argument was expected to exist but it was not found.");
        }
        try {
          assertEquals("In class Period: Method \"GetPeriod\" was expected to return a value of type \"long\".",
                       Long.TYPE, Period.class.getMethod("GetPeriod", new Class[0]).getReturnType());
        } catch (Exception e) {
          fail("In class Period: Method \"GetPeriod\" was expected to exist but it was not found.");
        }
    }
    
    public void checkMappedTypeOf_uint32_t_CType() {
        try {
          Class[] types = new Class[2];
          types[0] = long.class; types[1] = int.class;
          Time.class.getConstructor(types);
        } catch (Exception e) {
          fail("In class Time: Constructor taking types \"(long, int)\" as argument was expected to exist but it was not found.");
        }
        try {
          assertEquals("In class Time: Method \"GetTimeNanoseconds\" was expected to return a value of type \"long\".",
                       Long.TYPE, Time.class.getMethod("GetTimeNanoseconds", new Class[0]).getReturnType());
        } catch (Exception e) {
          fail("In class Time: Method \"GetTimeNanoseconds\" was expected to exist but it was not found.");
        }

        try {
          Class[] types = new Class[2];
          types[0] = long.class; types[1] = int.class;
          Period.class.getConstructor(types);
        } catch (Exception e) {
          fail("In class Period: Constructor taking types \"(long, int)\" as argument was expected to exist but it was not found.");
        }
        try {
          assertEquals("In class Period: Method \"GetPeriodNanoseconds\" was expected to return a value of type \"long\".",
                       Long.TYPE, Period.class.getMethod("GetPeriodNanoseconds", new Class[0]).getReturnType());
        } catch (Exception e) {
          fail("In class Period: Method \"GetPeriodNanoseconds\" was expected to exist but it was not found.");
        }
    }

    
    public static Test suite() {
        TestSuite suite = new TestSuite();

        suite.addTest(
            new MappingOf_time_t_and_uint32_t_CTypesToJavaRegressionTest() {
                protected void runTest() {
                  checkMappedTypeOf_time_t_CType();
                  checkMappedTypeOf_uint32_t_CType();
                }
            }
        );
        
        return suite;
    }
}
