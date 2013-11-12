'''
Regression test
Checks whether the time_t and uint32_t types are mapped to a number (long, int,
float).
'''

import testutils, arc, unittest, time, sys
if sys.version > '3':
    long = int
    number = 123456
else:
    number = 123456L

class MappingOf_time_t_and_uint32_t_CTypesToPythonRegressionTest(testutils.ARCClientTestCase):
    def setUp(self):
        pass
    
    def tearDown(self):
        pass
    
    def test_checkMappedTypeOf_time_t_CType(self):
        self.expect(arc.Time().GetTime()).to_be_an_instance_of((long, int, float))
        self.expect(arc.Time(number)).to_not_throw(NotImplementedError)

        self.expect(arc.Period().GetPeriod()).to_be_an_instance_of((long, int, float))
        self.expect(arc.Period(number)).to_not_throw(NotImplementedError)

    def test_checkMappedTypeOf_uint32_t_CType(self):
        self.expect(arc.Time().GetTimeNanoseconds()).to_be_an_instance_of((long, int, float))
        self.expect(arc.Time(number, 123456)).to_not_throw(NotImplementedError)

        self.expect(arc.Period().GetPeriodNanoseconds()).to_be_an_instance_of((long, int, float))
        self.expect(arc.Period(number, 123456)).to_not_throw(NotImplementedError)

if __name__ == '__main__':
    unittest.main()
