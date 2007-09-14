#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>

#include "DelegationInterface.h"

int main(void) {
  std::string credentials(
"-----BEGIN CERTIFICATE-----\n"
"MIIDQDCCAqmgAwIBAgICCjswDQYJKoZIhvcNAQEFBQAwTzENMAsGA1UEChMER3Jp\n"
"ZDESMBAGA1UEChMJTm9yZHVHcmlkMSowKAYDVQQDEyFOb3JkdUdyaWQgQ2VydGlm\n"
"aWNhdGlvbiBBdXRob3JpdHkwHhcNMDcwNTIxMTE0NjE0WhcNMDgwNTIwMTE0NjE0\n"
"WjBVMQ0wCwYDVQQKEwRHcmlkMRIwEAYDVQQKEwlOb3JkdUdyaWQxDzANBgNVBAsT\n"
"BnVpby5ubzEfMB0GA1UEAxMWQWxla3NhbmRyIEtvbnN0YW50aW5vdjCBnzANBgkq\n"
"hkiG9w0BAQEFAAOBjQAwgYkCgYEA1gaxDCuATqDJ9fGdPJfUs4LS37fnyV6XhsQU\n"
"SyXbbrTc1XEkYI/goWlR3HwkSlG5RF35r5bk/9zGtcOg3MF9CppqWdRGPTGPl9OS\n"
"l60wY+o9d5CyRo4xAUnUey60cWenUtuZAVPWqfPOeBrVCuX0qNQNPgMErJdTKm/G\n"
"30kKXBsCAwEAAaOCASMwggEfMAkGA1UdEwQCMAAwEQYJYIZIAYb4QgEBBAQDAgWg\n"
"MAsGA1UdDwQEAwIF4DAsBglghkgBhvhCAQ0EHxYdT3BlblNTTCBHZW5lcmF0ZWQg\n"
"Q2VydGlmaWNhdGUwHQYDVR0OBBYEFKYX6Nyfr6acgM+u+MKvIMjmhKgiMHcGA1Ud\n"
"IwRwMG6AFBgFwPwL0bc69GWSCftZoV/HiMTwoVOkUTBPMQ0wCwYDVQQKEwRHcmlk\n"
"MRIwEAYDVQQKEwlOb3JkdUdyaWQxKjAoBgNVBAMTIU5vcmR1R3JpZCBDZXJ0aWZp\n"
"Y2F0aW9uIEF1dGhvcml0eYIBADAsBgNVHREEJTAjgSFhbGVrc2FuZHIua29uc3Rh\n"
"bnRpbm92QGZ5cy51aW8ubm8wDQYJKoZIhvcNAQEFBQADgYEAEPzQ1JvLZ/viCC/U\n"
"YVD5pqMhBvcoTxO3vTFXfNJH0pRKtxHZ4Ib0Laef+7A0nX2Ct9j7npx0ekrIxwZR\n"
"8FUF0gGhoSrtQ1ciSYZmp+lkOT3dmWWWR8Gw72EByWH354F7Bpk0sOE4Ctc8bWtg\n"
"ggm88pXJ31uRDOFqP2S4VMB3vtA=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIICXAIBAAKBgQDWBrEMK4BOoMn18Z08l9SzgtLft+fJXpeGxBRLJdtutNzVcSRg\n"
"j+ChaVHcfCRKUblEXfmvluT/3Ma1w6DcwX0KmmpZ1EY9MY+X05KXrTBj6j13kLJG\n"
"jjEBSdR7LrRxZ6dS25kBU9ap8854GtUK5fSo1A0+AwSsl1Mqb8bfSQpcGwIDAQAB\n"
"AoGANKI+piSIkE2gfThnF8CrEV5p55S9jtsRXpYX+4ca2LXn3SHO9WRMtMVG2Xc6\n"
"IYDJlBOcVN9B/95Wi9rJU6DN0/uPG9hQ11a64aPnURK/FwBFjZk/N8xKTWOg1yuR\n"
"e3qTksn5msUmedmQGnUqc2U9hNyb7qUoJeszoRUKsRR69gkCQQDvUGHEs6PVr/ua\n"
"zlXpMiCATEGPEgzKG4CNFfZrzJHjMgF8sebYheETa+XXjiabMTRpckWFZL0CySw0\n"
"3h/zJOpdAkEA5PLvmpfUkJamW+sT0AfUVRPzdMllV9mP9II+Gn6scy0CPprJHNWj\n"
"HrbJhLhHFB/Zd6pHVjROQh/60f/7yWco1wJAedibbuNoD2zC1lcoNstm8OvilU3D\n"
"ZUQLd8ou5UQLI3pad3q85pGDv7e4FsAxt+KdpPKhowFfmwOClohiDBJHoQJBAJ7G\n"
"w9hId4gWkiSo8MKSy3R9M5fIm9nC7gy5zmv9cYcmranRGqw+lLOWPEcorVKNi/lr\n"
"Q7HK8IL3PrEof+t6+V0CQARmHrN3Ow6Wzh3Tt2+qXrf+200rdThimfYW9EDG6K27\n"
"D2d54PTDX0AryhtgIBaKBd/Gk3kJZvwj8erT+sPMHUk=\n"
"-----END RSA PRIVATE KEY-----"
  );
  std::string s;
  Arc::DelegationConsumer c;
  c.Backup(s);
  std::cerr<<"Private key:\n"<<s<<std::endl;
  c.Request(s);
  std::cerr<<"Request:\n"<<s<<std::endl;
  Arc::DelegationProvider p(credentials);
  s=p.Delegate(s);
  std::cerr<<"Delegation:\n"<<s<<std::endl;
  return 0;
}

