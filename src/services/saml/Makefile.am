noinst_PROGRAMS = login_tests saml_tests

login_tests_SOURCES = login_tests.cc
login_tests_CXXFLAGS = $(LASSO_CFLAGS) -DTESTSDATADIR=\"$(srcdir)/data/\"
login_tests_LDADD = $(LASSO_LIBS) 

saml_tests_SOURCES = saml_tests.cc
saml_tests_CXXFLAGS = $(LASSO_CFLAGS) -DTESTSDATADIR=\"$(srcdir)/data/\"
saml_tests_LDADD = $(LASSO_LIBS)
