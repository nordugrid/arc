from __future__ import print_function

class A(object):
    def __init__(self):
        pass
    def funca(self):
        print('funca')

class B(object):
    def __init__(self):
        pass
    def funcb(self):
        print('funcb')

class C(A,B):
    def __init__(self):
        pass
    
    def funcc(self):
        self.funca()
        self.funcb()
        print('funcc')

c = C()
print(type(c), dir(c), c.__class__)
