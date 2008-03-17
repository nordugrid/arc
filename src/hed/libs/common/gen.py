#!/usr/bin/python

from Numeric import *
import sys

k = zeros(8, Int)

ntypes = 2;

sys.stdout.write ("    IString(const std::string& m")
sys.stdout.write (")\n      : p(new PrintF<")
sys.stdout.write (">(m")
sys.stdout.write (")) {}\n\n")

for k[0] in range (0, ntypes):
    if (k[0] == 0):
        sys.stdout.write ("    template <")
        sys.stdout.write ("class T0")
        sys.stdout.write (">\n")
    sys.stdout.write ("    IString(const std::string& m")
    sys.stdout.write (", ")
    if (k[0] == 0):
        sys.stdout.write ("T0 t0")
    elif (k[0] == 1):
        sys.stdout.write ("const std::string& t0")
    sys.stdout.write (")\n      : p(new PrintF<")
    if (k[0] == 0):
        sys.stdout.write ("T0")
    elif (k[0] == 1):
        sys.stdout.write ("const char*")
    sys.stdout.write (">(m")
    sys.stdout.write (", ")
    if (k[0] == 0):
        sys.stdout.write ("t0")
    elif (k[0] == 1):
        sys.stdout.write ("t0.c_str()")
    sys.stdout.write (")) {}\n\n")

nargs = 2

for k[0] in range (0, ntypes):
    for k[1] in range (0, ntypes):
        if (k[0] == 0 or k[1] == 0):
            sys.stdout.write ("    template <")
            comma = False;
            for i in range (0, nargs):
                if (k[i] == 0):
                    if (comma):
                        sys.stdout.write (", ")
                    sys.stdout.write ("class T"+str(i))
                    comma = True;
            sys.stdout.write (">\n")
        sys.stdout.write ("    IString(const std::string& m")
        for i in range (0, nargs):
            sys.stdout.write (", ")
            if (k[i] == 0):
                sys.stdout.write ("T"+str(i)+" t"+str(i))
            elif (k[i] == 1):
                sys.stdout.write ("const std::string& t"+str(i))
        sys.stdout.write (")\n      : p(new PrintF<")
        for i in range (0, nargs):
            if (i != 0):
                sys.stdout.write (", ")
            if (k[i] == 0):
                sys.stdout.write ("T"+str(i))
            elif (k[i] == 1):
                sys.stdout.write ("const char*")
        sys.stdout.write (">(m")
        for i in range (0, nargs):
            sys.stdout.write (", ")
            if (k[i] == 0):
                sys.stdout.write ("t"+str(i))
            elif (k[i] == 1):
                sys.stdout.write ("t"+str(i)+".c_str()")
        sys.stdout.write (")) {}\n\n")

nargs = 3

for k[0] in range (0, ntypes):
    for k[1] in range (0, ntypes):
        for k[2] in range (0, ntypes):
            if (k[0] == 0 or k[1] == 0 or k[2] == 0):
                sys.stdout.write ("    template <")
                comma = False;
                for i in range (0, nargs):
                    if (k[i] == 0):
                        if (comma):
                            sys.stdout.write (", ")
                        sys.stdout.write ("class T"+str(i))
                        comma = True;
                sys.stdout.write (">\n")
            sys.stdout.write ("    IString(const std::string& m")
            for i in range (0, nargs):
                sys.stdout.write (", ")
                if (k[i] == 0):
                    sys.stdout.write ("T"+str(i)+" t"+str(i))
                elif (k[i] == 1):
                    sys.stdout.write ("const std::string& t"+str(i))
            sys.stdout.write (")\n      : p(new PrintF<")
            for i in range (0, nargs):
                if (i != 0):
                    sys.stdout.write (", ")
                if (k[i] == 0):
                    sys.stdout.write ("T"+str(i))
                elif (k[i] == 1):
                    sys.stdout.write ("const char*")
            sys.stdout.write (">(m")
            for i in range (0, nargs):
                sys.stdout.write (", ")
                if (k[i] == 0):
                    sys.stdout.write ("t"+str(i))
                elif (k[i] == 1):
                    sys.stdout.write ("t"+str(i)+".c_str()")
            sys.stdout.write (")) {}\n\n")

nargs = 4

for k[0] in range (0, ntypes):
    for k[1] in range (0, ntypes):
        for k[2] in range (0, ntypes):
            for k[3] in range (0, ntypes):
                if (k[0] == 0 or k[1] == 0 or k[2] == 0 or k[3] == 0):
                    sys.stdout.write ("    template <")
                    comma = False;
                    for i in range (0, nargs):
                        if (k[i] == 0):
                            if (comma):
                                sys.stdout.write (", ")
                            sys.stdout.write ("class T"+str(i))
                            comma = True;
                    sys.stdout.write (">\n")
                sys.stdout.write ("    IString(const std::string& m")
                for i in range (0, nargs):
                    sys.stdout.write (", ")
                    if (k[i] == 0):
                        sys.stdout.write ("T"+str(i)+" t"+str(i))
                    elif (k[i] == 1):
                        sys.stdout.write ("const std::string& t"+str(i))
                sys.stdout.write (")\n      : p(new PrintF<")
                for i in range (0, nargs):
                    if (i != 0):
                        sys.stdout.write (", ")
                    if (k[i] == 0):
                        sys.stdout.write ("T"+str(i))
                    elif (k[i] == 1):
                        sys.stdout.write ("const char*")
                sys.stdout.write (">(m")
                for i in range (0, nargs):
                    sys.stdout.write (", ")
                    if (k[i] == 0):
                        sys.stdout.write ("t"+str(i))
                    elif (k[i] == 1):
                        sys.stdout.write ("t"+str(i)+".c_str()")
                sys.stdout.write (")) {}\n\n")

nargs = 5

for k[0] in range (0, ntypes):
    for k[1] in range (0, ntypes):
        for k[2] in range (0, ntypes):
            for k[3] in range (0, ntypes):
                for k[4] in range (0, ntypes):
                    if (k[0] == 0 or k[1] == 0 or k[2] == 0 or k[3] == 0 or
                        k[4] == 0):
                        sys.stdout.write ("    template <")
                        comma = False;
                        for i in range (0, nargs):
                            if (k[i] == 0):
                                if (comma):
                                    sys.stdout.write (", ")
                                sys.stdout.write ("class T"+str(i))
                                comma = True;
                        sys.stdout.write (">\n")
                    sys.stdout.write ("    IString(const std::string& m")
                    for i in range (0, nargs):
                        sys.stdout.write (", ")
                        if (k[i] == 0):
                            sys.stdout.write ("T"+str(i)+" t"+str(i))
                        elif (k[i] == 1):
                            sys.stdout.write ("const std::string& t"+str(i))
                    sys.stdout.write (")\n      : p(new PrintF<")
                    for i in range (0, nargs):
                        if (i != 0):
                            sys.stdout.write (", ")
                        if (k[i] == 0):
                            sys.stdout.write ("T"+str(i))
                        elif (k[i] == 1):
                            sys.stdout.write ("const char*")
                    sys.stdout.write (">(m")
                    for i in range (0, nargs):
                        sys.stdout.write (", ")
                        if (k[i] == 0):
                            sys.stdout.write ("t"+str(i))
                        elif (k[i] == 1):
                            sys.stdout.write ("t"+str(i)+".c_str()")
                    sys.stdout.write (")) {}\n\n")

nargs = 6

for k[0] in range (0, ntypes):
    for k[1] in range (0, ntypes):
        for k[2] in range (0, ntypes):
            for k[3] in range (0, ntypes):
                for k[4] in range (0, ntypes):
                    for k[5] in range (0, ntypes):
                        if (k[0] == 0 or k[1] == 0 or k[2] == 0 or k[3] == 0 or
                            k[4] == 0 or k[5] == 0):
                            sys.stdout.write ("    template <")
                            comma = False;
                            for i in range (0, nargs):
                                if (k[i] == 0):
                                    if (comma):
                                        sys.stdout.write (", ")
                                    sys.stdout.write ("class T"+str(i))
                                    comma = True;
                            sys.stdout.write (">\n")
                        sys.stdout.write ("    IString(const std::string& m")
                        for i in range (0, nargs):
                            sys.stdout.write (", ")
                            if (k[i] == 0):
                                sys.stdout.write ("T"+str(i)+" t"+str(i))
                            elif (k[i] == 1):
                                sys.stdout.write ("const std::string& t"+str(i))
                        sys.stdout.write (")\n      : p(new PrintF<")
                        for i in range (0, nargs):
                            if (i != 0):
                                sys.stdout.write (", ")
                            if (k[i] == 0):
                                sys.stdout.write ("T"+str(i))
                            elif (k[i] == 1):
                                sys.stdout.write ("const char*")
                        sys.stdout.write (">(m")
                        for i in range (0, nargs):
                            sys.stdout.write (", ")
                            if (k[i] == 0):
                                sys.stdout.write ("t"+str(i))
                            elif (k[i] == 1):
                                sys.stdout.write ("t"+str(i)+".c_str()")
                        sys.stdout.write (")) {}\n\n")

nargs = 7

for k[0] in range (0, ntypes):
    for k[1] in range (0, ntypes):
        for k[2] in range (0, ntypes):
            for k[3] in range (0, ntypes):
                for k[4] in range (0, ntypes):
                    for k[5] in range (0, ntypes):
                        for k[6] in range (0, ntypes):
                            if (k[0] == 0 or k[1] == 0 or k[2] == 0 or k[3] == 0 or
                                k[4] == 0 or k[5] == 0 or k[6] == 0):
                                sys.stdout.write ("    template <")
                                comma = False;
                                for i in range (0, nargs):
                                    if (k[i] == 0):
                                        if (comma):
                                            sys.stdout.write (", ")
                                        sys.stdout.write ("class T"+str(i))
                                        comma = True;
                                sys.stdout.write (">\n")
                            sys.stdout.write ("    IString(const std::string& m")
                            for i in range (0, nargs):
                                sys.stdout.write (", ")
                                if (k[i] == 0):
                                    sys.stdout.write ("T"+str(i)+" t"+str(i))
                                elif (k[i] == 1):
                                    sys.stdout.write ("const std::string& t"+str(i))
                            sys.stdout.write (")\n      : p(new PrintF<")
                            for i in range (0, nargs):
                                if (i != 0):
                                    sys.stdout.write (", ")
                                if (k[i] == 0):
                                    sys.stdout.write ("T"+str(i))
                                elif (k[i] == 1):
                                    sys.stdout.write ("const char*")
                            sys.stdout.write (">(m")
                            for i in range (0, nargs):
                                sys.stdout.write (", ")
                                if (k[i] == 0):
                                    sys.stdout.write ("t"+str(i))
                                elif (k[i] == 1):
                                    sys.stdout.write ("t"+str(i)+".c_str()")
                            sys.stdout.write (")) {}\n\n")

nargs = 8

for k[0] in range (0, ntypes):
    for k[1] in range (0, ntypes):
        for k[2] in range (0, ntypes):
            for k[3] in range (0, ntypes):
                for k[4] in range (0, ntypes):
                    for k[5] in range (0, ntypes):
                        for k[6] in range (0, ntypes):
                            for k[7] in range (0, ntypes):
                                if (k[0] == 0 or k[1] == 0 or k[2] == 0 or k[3] == 0 or
                                    k[4] == 0 or k[5] == 0 or k[6] == 0 or k[7] == 0):
                                    sys.stdout.write ("    template <")
                                    comma = False;
                                    for i in range (0, nargs):
                                        if (k[i] == 0):
                                            if (comma):
                                                sys.stdout.write (", ")
                                            sys.stdout.write ("class T"+str(i))
                                            comma = True;
                                    sys.stdout.write (">\n")
                                sys.stdout.write ("    IString(const std::string& m")
                                for i in range (0, nargs):
                                    sys.stdout.write (", ")
                                    if (k[i] == 0):
                                        sys.stdout.write ("T"+str(i)+" t"+str(i))
                                    elif (k[i] == 1):
                                        sys.stdout.write ("const std::string& t"+str(i))
                                sys.stdout.write (")\n      : p(new PrintF<")
                                for i in range (0, nargs):
                                    if (i != 0):
                                        sys.stdout.write (", ")
                                    if (k[i] == 0):
                                        sys.stdout.write ("T"+str(i))
                                    elif (k[i] == 1):
                                        sys.stdout.write ("const char*")
                                sys.stdout.write (">(m")
                                for i in range (0, nargs):
                                    sys.stdout.write (", ")
                                    if (k[i] == 0):
                                        sys.stdout.write ("t"+str(i))
                                    elif (k[i] == 1):
                                        sys.stdout.write ("t"+str(i)+".c_str()")
                                sys.stdout.write (")) {}\n\n")
