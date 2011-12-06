
import glob
import os
from M2Crypto import BIO, Rand, SMIME, X509
from subprocess import Popen, PIPE
import tempfile
import logging

# logging configuration
log = logging.getLogger('SSM')

################################################################################
# Convenience function to read entire file into string
#

def from_file(filename):
    f = open(filename, 'r')
    s = f.read()
    f.close()
    return s


###############################################################################
# Check that a certificate and a key match, using openssl directly to fetch
# the modulus of each, which must be the same.
#

def check_cert_key(certificate, key):
    
    # Two things the same have the same modulus.
    if certificate == key:
        return False
    
    p1 = Popen(["openssl", "x509", "-noout", "-modulus"], 
               stdin = PIPE, stdout = PIPE, stderr = PIPE)
    modulus1, error = p1.communicate(certificate)
    
    if (error != ''):
        log.error(error)
    
    p2 = Popen(["openssl", "rsa", "-noout", "-modulus"], 
               stdin = PIPE, stdout = PIPE, stderr = PIPE)
    modulus2, error = p2.communicate(key)
    
    if (error != ''):
        log.error(error)
        
    # Each returns a tuple of which we want the first item.
    return (modulus1 == modulus2)

###################d#############################################################
# Sign the specified message using the certificate and key
#
# Returns the signed message as an SMIME string, suitable for transmission

def sign_message(text, certificate, key):
    p1 = Popen(["openssl", "smime", "-sign", "-inkey", key, "-signer", certificate, "-text"], 
               stdin = PIPE, stdout = PIPE, stderr = PIPE)
    
    signed_msg, error = p1.communicate(text)
    
    if (error != ''):
        log.error(error)
        print error

    return signed_msg

    # Using M2Crypto...
    # This signature code with v0.16 of m2crypto doesn't work as expected, in
    # that it generates a signature the same size as the original message,
    # rather than a constant-size signature 'blob' as in the original OpenSSL
    # command. This results in a message doubling in size, which is OK in cases
    # of small (<100k) messages.

    # Make a MemoryBuffer of the message.
    buf = BIO.MemoryBuffer(text)
    #buf = BIO.MemoryBuffer('atest')

    # Seed the PRNG.
    Rand.load_file('randpool.dat', -1)

    # Instantiate an SMIME object; set it up; sign the buffer.
    s = SMIME.SMIME()
    s.load_key(key, certificate)
    p7 = s.sign(buf)

    # buf gets stomped during signing, create another one
    buf = BIO.MemoryBuffer(text)

    # headers- optional
    out=BIO.MemoryBuffer()
    #header.write('blah')

    #write out the signature and the buffer
    s.write(out,p7,buf)
    #s.write(out,p7)
    return out.read()


################################################################################
# Encrypt the specified message using the certificate
# 
# Returns the encrypted SMIME text suitable for transmission

def encrypt_message(text, certificate):
    # store the certificate in a file
    tmpfd,tmpname = tempfile.mkstemp(prefix='cert')
    os.write(tmpfd,certificate)
    os.close(tmpfd)

    # encrypt
    p1 = Popen(["openssl", "smime", "-encrypt", "-des3", tmpname], 
               stdin = PIPE, stdout = PIPE, stderr = PIPE)
    
    enc_txt, error = p1.communicate(text)
    
    if (error != ''):
        log.error(error)

    # tidy
    os.remove(tmpname)
    return enc_txt

    # Using M2Crypto...

    # The reason not to use this code is again to do with the size of the
    # message, in that it is much faster to call out to OpenSSL rather than use
    # the m2crypto library for anything larger than 100k.

    buf = BIO.MemoryBuffer(text)
    Rand.load_file('randpool.dat',-1)
    s = SMIME.SMIME()

    x509 = X509.load_cert_string(certificate)
    sk = X509.X509_Stack()
    sk.push(x509)

    s.set_x509_stack(sk)

    s.set_cipher(SMIME.Cipher('des_ede3_cbc'))
    #s.set_cipher(SMIME.Cipher('aes_128_cbc'))

    p7 = s.encrypt(buf)

    out = BIO.MemoryBuffer()
    s.write(out,p7)
    return out.read()



################################################################################
# Verify the signed message has been signed by the certificate (attached to the
# supplied SMIME message) it claims to have, by one of the accepted CAs in
# capath.
# 
# Returns a tuple including the signer's certificate and the plain-text of the
# message if it has been verified

def verify_message(signed_text, capath, check_crl):
    
    signer = get_signer_cert(signed_text)
    
    if not verify_certificate(signer, capath, check_crl):
        raise RuntimeError("Unverified signer")

    # The -noverify flag removes the certificate verification.  The certificate 
    # is verified above; this check would also check that the certificate
    # is allowed to sign with SMIME, which host certificates sometimes aren't.
    p1 = Popen(["openssl", "smime", "-verify", "-CApath", capath, "-noverify", "-text"], 
               stdin = PIPE, stdout = PIPE, stderr = PIPE)
    
    message, error = p1.communicate(signed_text)
    
    # Interesting problem here - we get a message 'Verification successful'
    # to standard error.  We don't want to log this as an error each time,
    # but we do want to see if there's a genuine error...
    log.info(str(error).strip())

    signer_x509 = X509.load_cert_string(signer)
    return str(signer_x509.get_subject()), message

    # Using M2Crypto...

    # (you can only use this code if you're also using m2crypto to sign the
    # message)

    s=SMIME.SMIME()

    # Read the signer's certificate from the message, and verify it. Note, if
    # we don't do this first, then the verify process will segfault below.

    signer = get_signer_cert(signed_text)
    
    
    if not verify_certificate(signer, capath, check_crl):
        raise RuntimeError("Unverified signer")
    
        
    # Create X509 stack including just the signer certificate (which we will
    # read from the message)

    sk=X509.X509_Stack()
    sk.push(signer)
    s.set_x509_stack(sk)

    # Create X509 certificate store, including all the certificates that
    # we might need in the chain to verify the signer

    st = load_certificate_store(capath)
    s.set_x509_store(st)

    blob = BIO.MemoryBuffer(signed_text)

    # See note against other write_close call below for reasons for this
    blob.write_close()  

    p7,data=SMIME.smime_load_pkcs7_bio(blob)
    v = s.verify(p7,data)

    signer_x509 = X509.load_cert_string(signer)
    return str(signer_x509.get_subject()),v



################################################################################
# Decrypt the specified message using the certificate and key contained in the
# named PEM files. The capath should point to a directory holding all the
# CAs that we accept

def decrypt_message(encryptedText, certificate, key, capath):

    # This decryption function can be used whether or not OpenSSL is used to
    # encrypt the data

    s=SMIME.SMIME()

    blob = BIO.MemoryBuffer(encryptedText)
    
    # m2Crypto v0.17? Then need to add this line to smime_load_pkcs7_bio,
    # in SMIME.py, at line 98
    #      m2.bio_set_mem_eof_return(p7_bio._ptr(),0)
    #
    # Otherwise you get a 'not enough data' error from SMIME module

    # Alternatively, blob.write_close() also seems to fix this, but this 
    # might need to be revisited if a later M2Crypto is used

    blob.write_close()
    s.load_key(key, certificate)
    p7, data = SMIME.smime_load_pkcs7_bio(blob)
    
    ###########
    # Write data to a temporary file, then decrypt it
    # Workaround because the above doesn't work with M2Crypto v0.17
    #tmpfd,tmpname = tempfile.mkstemp()
    #os.write(tmpfd,encryptedText)
    #os.close(tmpfd)
    #p7, data = SMIME.smime_load_pkcs7(tmpname)
    #os.remove(tmpname)
    ##########

    out = s.decrypt(p7)

    return out

################################################################################
# Verify that the certificate is signed by a CA whose certificate is stored in
# capath. 
#
# Return True if the certificate is valid

#def verify_certificate(certificate, capath):
#    x509 = X509.load_cert(certificate)
#    st = load_certificate_store(capath)
#    if x509.verify() == 1:
#        return True
#    else:
#        return False
#

################################################################################
# Load all the certificates in the specified directory into a certificate store
# object.
#
# Returns the certificate store 

def load_certificate_store(capath):
    st = X509.X509_Store()
    #st.load_locations(capath) -- doesn't work; possible bug in M2Crypto

    for cert in glob.glob(capath+'/*.0'):
        st.load_info(cert)

    return st


################################################################################
# Verify that the certificate is signed by a CA whose certificate is stored in
# capath.  There are two variants of this function, one will load the certificate 
# from a string, the other will use an X509 object.
#
# Returns True if verified

def verify_certificate(certificate, capath, check_crls=True):
    
    verified = False
    
    if check_crls:
        # Use openssl
        verified = verify_cert_and_crls(certificate, capath)
    else:
        # Use m2crypto
        x509 = X509.load_cert_string(certificate)
        verified = verify_certificate_x509(x509, capath)

    return verified



################################################################################
# Verify the certificate against the CA certs in capath.  Note that this uses
# openssl directly because the python libraries don't offer this.
# Note also that I've had to compare strings in the output of openssl to check
# for verification, which may make this brittle.
#
# Returns True if the certificate is verified.
#

def verify_cert_and_crls(certificate, capath):
    
    p1 = Popen(["openssl", "verify", "-CApath", capath, "-crl_check_all"], 
               stdin = PIPE, stdout = PIPE, stderr = PIPE)
    
    message, error = p1.communicate(certificate)
        
    # I think this is unlikely ever to happen
    if (error != ''):
        log.error(error)
    
    # There was a sticky problem here.  
    #
    # None of the python openssl libraries go as far as checking CRLs, 
    # so I had to resort to calling openssl directly.
    # However, 'openssl verify' returns 0 whatever happens, so we can't 
    # use the return code to determine whether the verificatation was 
    # successful.  
    # If it is successful, openssl prints 'OK'
    # If it fails, openssl prints 'error'
    # So:
    log.info("Certificate verification: " + str(message).strip())
      
    return ("OK" in message and not "error" in message)



def verify_certificate_x509(x509, capath):
    
    count=0
    for cert in glob.glob(capath+'/*.0'):
        ca = X509.load_cert(cert)
        pkey=ca.get_pubkey()
        if x509.verify(pkey): 
            count += 1
            break
    
    return (count > 0)


################################################################################
# Get the subject from the certificate
#
def get_certificate_subject(certificate):
    x509 = X509.load_cert_string(certificate)
    return str(x509.get_subject())

################################################################################
# Read the signer's certificate from the specified message, and return the
# certificate object.
#
# Returns an X509 object for the signer's certificate

def get_signer_cert(signed_text):
    
    # have to resort to calling out to the openssl command line client in order
    # to extract the certificate from the signed message, because I can't
    # figure out how to achieve this using the M2Crypto API

    p1 = Popen(["openssl", "smime", "-pk7out"], 
               stdin = PIPE, stdout = PIPE, stderr = PIPE)
    p2 = Popen(["openssl", "pkcs7", "-print_certs"], 
               stdin = p1.stdout, stdout = PIPE, stderr = PIPE)
    
    p1.stdin.write(signed_text)
    cert_string, error = p2.communicate()
    
    
    if (error != ''):
        log.error(error)
        
    return cert_string


def get_signer_cert_x509(signed_text):

    cert_string = get_signer_cert(signed_text)
    
    return (X509.load_cert_string(cert_string))


