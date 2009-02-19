
# CREATES config.h
autoheader
touch NEWS README AUTHORS ChangeLog

# You need a stamp-h file in your project to ensure that automake regenerates config.h from config.h.in. 
# Type 'touch stamp-h' to add this file to your project.
touch stamp-h

# Adds aclocal.m4 to directory. Defines some m4 macros used by the auto tools.
# These tools use the m4 programming language. aclocal adds aclocal.m4 to your project directory,
# which contains some m4 macros which are neede 
touch aclocal.m4
aclocal

# ltconfig runs a series of configuration tests, then creates a system-specific libtool in the
# current directory. 
#ltconfig

# Creates configure from configure.ac 
autoconf

# Creates Makefile.in from Makefile.am 
automake  --add-missing

# Creates Makefile from Makefile.in
./configure  --prefix=/tmp/arcservices/
 
#make
