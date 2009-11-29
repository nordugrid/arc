# order is important!
SUBDIRS = libs acc mcc dmc shc daemon identitymap
DIST_SUBDIRS = libs acc mcc dmc shc daemon identitymap

profiledir = $(pkgdatadir)/profiles
profile_DATA = \
    profiles/CheloniaAllServicesCentralizedAHash/CheloniaAllServicesCentralizedAHash.xml \
    profiles/CheloniaAllServicesCentralizedAHashWithISIS/CheloniaAllServicesCentralizedAHashWithISIS.xml \
    profiles/CheloniaAllServicesReplicatedAHash/CheloniaAllServicesReplicatedAHash.xml \
    profiles/CheloniaAllServicesReplicatedAHashGatewayVOMSWithISIS/CheloniaAllServicesReplicatedAHashGatewayVOMSWithISIS.xml \
    profiles/CheloniaAllServicesReplicatedAHashWithISIS/CheloniaAllServicesReplicatedAHashWithISIS.xml \
    profiles/CheloniaReplicatedAHash/CheloniaReplicatedAHash.xml \
    profiles/CheloniaShepherdWithHopi/CheloniaShepherdWithHopi.xml \
    profiles/EchoService/EchoService.xml \
    profiles/EchoServiceNonSecure/EchoServiceNonSecure.xml \
    profiles/EchoServicePython/EchoServicePython.xml \
    profiles/EchoServicePythonNonSecure/EchoServicePythonNonSecure.xml \
    profiles/EchoServiceSAMLToken/EchoServiceSAMLToken.xml \
    profiles/EchoServiceUsernameToken/EchoServiceUsernameToken.xml \
    profiles/EchoServiceX509Token/EchoServiceX509Token.xml \
    profiles/HopiService/HopiService.xml \
    profiles/HopiServiceNonSecure/HopiServiceNonSecure.xml \
    profiles/HopiServiceWithPlexer/HopiServiceWithPlexer.xml \
    profiles/HopiServiceWithPlexerNonSecure/HopiServiceWithPlexerNonSecure.xml \
    profiles/NonSecureNonDistributedStorageElement/NonSecureNonDistributedStorageElement.xml \
    profiles/NonSecureP2PIIS/NonSecureP2PIIS.xml \
    profiles/NonSecureStandaloneIIS/NonSecureStandaloneIIS.xml \
    profiles/SecureP2PIIS/SecureP2PIIS.xml \
    profiles/SecureStandaloneIIS/SecureStandaloneIIS.xml

profileexampledir = $(pkgdatadir)/examples/config
profileexample_DATA = profiles/*/*.xml profiles/*/*.ini profiles/*/*.conf profiles/*/*.txt

install-profileexampleDATA: $(profile_DATA)
	for i in profiles/*; do \
          profile=`basename $$i` ; \
          mkdir -p "$(DESTDIR)$(profileexampledir)/$$profile" ; \
          ( cd profiles/$$profile ; for i in *.xml *.ini *.conf *.txt ; do \
            test -f $$i && cp -p $$i "$(DESTDIR)$(profileexampledir)/$$profile/" || : ; \
          done ) ; \
        done

EXTRA_DIST = profiles/*/*
