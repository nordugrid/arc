#include "CheckSum.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

// ----------------------------------------------------------------------------
// This is CRC(32bit) implementation as in 'cksum' utility
// ----------------------------------------------------------------------------

//  g=0x0000000104C11DB7LL;

static uint32_t gtable[256] = {
0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 
0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005, 
0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 
0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 
0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 
0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 
0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 
0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD, 
0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 
0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 
0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 
0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D, 
0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 
0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95, 
0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 
0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 
0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 
0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 
0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 
0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 
0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 
0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 
0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066, 
0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA, 
0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 
0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692, 
0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 
0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 
0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E, 
0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 
0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 
0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A, 
0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 
0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 
0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 
0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 
0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 
0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B, 
0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF, 
0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 
0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 
0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 
0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 
0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3, 
0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 
0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 
0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 
0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 
0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 
0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 
0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 
0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 
0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30, 
0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC, 
0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 
0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 
0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 
0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 
0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 
0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 
0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 
0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C, 
0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 
0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};

namespace Arc {

CRC32Sum::CRC32Sum(void) {
  start();
}

void CRC32Sum::start(void) {
  r=0;
  count=0;
  computed=false;
}

void CRC32Sum::add(void* buf,unsigned long long int len) { 
  for(unsigned long long int i=0;i<len;i++) {
    unsigned char c = (r >> 24);
    r=((r<<8) | ((unsigned char *)buf)[i]) ^ gtable[c];
  };
  count+=len;
}

void CRC32Sum::end(void) {
  if(computed) return;
  unsigned long long l = count;
  for(;l;) {
    unsigned char c = (l & 0xFF);
    ((CheckSum*)this)->add(&c,1);
    l>>=8;
  };
  uint32_t u = 0;
  ((CheckSum*)this)->add(&u,4);
  r=((~r) & 0xFFFFFFFF);
  computed=true;
}

int CRC32Sum::print(char* buf,int len) const {
  if(!computed) { if(len>0) buf[0]=0; return 0; };
  return snprintf(buf,len,"cksum: %08x",r);
}

void CRC32Sum::scan(const char* buf) {
  computed=false;
  int l;
  if(strncasecmp("cksum:",buf,6) == 0) {
    unsigned long long rr; // for compatibilty with bug in 0.4
    l=sscanf(buf+6,"%Lx",&rr);
    r=rr;
  } else {
    int i;
    l=0;
    for(i = 0;buf[i];i++) if(!isdigit(buf[i])) break;
    if(!(buf[i])) { l=sscanf(buf,"%u",&r); }
    else {
      for(i = 0;buf[i];i++) if(!isxdigit(buf[i])) break;
      if(!(buf[i])) { 
        unsigned long long rr;
        l=sscanf(buf,"%Lx",&rr);
        r=rr;
      };
    };
  };
  if(l != 1) return;
  computed=true;
  return;
}


// ----------------------------------------------------------------------------
// This is MD5 implementation for LOW-ENDIAN machines derived directly from RFC
// ----------------------------------------------------------------------------

#define F(X,Y,Z) (((X) & (Y)) | ((~(X)) & (Z)))
#define G(X,Y,Z) (((X) & (Z)) | ((Y) & (~(Z))))
#define H(X,Y,Z) ((X) ^ (Y) ^ (Z))
#define I(X,Y,Z) ((Y) ^ ((X) | (~(Z))))

#define OP1(a,b,c,d,k,s,i) { \
  uint32_t t = ((a) + F(b,c,d) + X[k] + T[i-1]); \
  (a) = (b) + (((t) << (s))|((t) >> (32-s))); \
}

#define OP2(a,b,c,d,k,s,i) { \
  uint32_t t = ((a) + G(b,c,d) + X[k] + T[i-1]); \
  (a) = (b) + (((t) << (s))|((t) >> (32-s))); \
}

#define OP3(a,b,c,d,k,s,i) { \
  uint32_t t = ((a) + H(b,c,d) + X[k] + T[i-1]); \
  (a) = (b) + (((t) << (s))|((t) >> (32-s))); \
}

#define OP4(a,b,c,d,k,s,i) { \
  uint32_t t = ((a) + I(b,c,d) + X[k] + T[i-1]); \
  (a) = (b) + (((t) << (s))|((t) >> (32-s))); \
}

#define A_INIT (0x67452301)
#define B_INIT (0xefcdab89)
#define C_INIT (0x98badcfe)
#define D_INIT (0x10325476)

static uint32_t T[64] = {
  3614090360U, 3905402710U,  606105819U, 3250441966U,
  4118548399U, 1200080426U, 2821735955U, 4249261313U,
  1770035416U, 2336552879U, 4294925233U, 2304563134U,
  1804603682U, 4254626195U, 2792965006U, 1236535329U,
  4129170786U, 3225465664U,  643717713U, 3921069994U,
  3593408605U,   38016083U, 3634488961U, 3889429448U,
   568446438U, 3275163606U, 4107603335U, 1163531501U,
  2850285829U, 4243563512U, 1735328473U, 2368359562U,
  4294588738U, 2272392833U, 1839030562U, 4259657740U,
  2763975236U, 1272893353U, 4139469664U, 3200236656U,
   681279174U, 3936430074U, 3572445317U,   76029189U,
  3654602809U, 3873151461U,  530742520U, 3299628645U,
  4096336452U, 1126891415U, 2878612391U, 4237533241U,
  1700485571U, 2399980690U, 4293915773U, 2240044497U,
  1873313359U, 4264355552U, 2734768916U, 1309151649U,
  4149444226U, 3174756917U,  718787259U, 3951481745U
};


MD5Sum::MD5Sum(void) {
  // for(u_int i = 1;i<=64;i++) T[i-1]=(uint32_t)(4294967296LL*fabs(sin(i)));
  start();
}

void MD5Sum::start(void) {
  A=A_INIT; B=B_INIT; C=C_INIT; D=D_INIT;
  count=0; Xlen=0;
  computed=false;
}

void MD5Sum::add(void* buf,unsigned long long int len) { 
  u_char* buf_ = (u_char*)buf;
  for(;len;) {
    if(Xlen<64) { // 16 words = 64 bytes
      u_int l = 64-Xlen;
      if(len < l) l=len;
      memcpy(((u_char*)X)+Xlen,buf_,l);
      Xlen+=l; count+=l; len-=l; buf_+=l;
    };
    if(Xlen<64) return;

    uint32_t AA = A;
    uint32_t BB = B;
    uint32_t CC = C;
    uint32_t DD = D;


    OP1(A,B,C,D,0,7,1);    OP1(D,A,B,C,1,12,2);
    OP1(C,D,A,B,2,17,3);   OP1(B,C,D,A,3,22,4);

    OP1(A,B,C,D,4,7,5);    OP1(D,A,B,C,5,12,6);
    OP1(C,D,A,B,6,17,7);   OP1(B,C,D,A,7,22,8);

    OP1(A,B,C,D,8,7,9);    OP1(D,A,B,C,9,12,10);
    OP1(C,D,A,B,10,17,11); OP1(B,C,D,A,11,22,12);

    OP1(A,B,C,D,12,7,13);  OP1(D,A,B,C,13,12,14);
    OP1(C,D,A,B,14,17,15); OP1(B,C,D,A,15,22,16);


    OP2(A,B,C,D,1,5,17);   OP2(D,A,B,C,6,9,18);
    OP2(C,D,A,B,11,14,19); OP2(B,C,D,A,0,20,20);

    OP2(A,B,C,D,5,5,21);   OP2(D,A,B,C,10,9,22);  
    OP2(C,D,A,B,15,14,23); OP2(B,C,D,A,4,20,24);

    OP2(A,B,C,D,9,5,25);   OP2(D,A,B,C,14,9,26);
    OP2(C,D,A,B,3,14,27);  OP2(B,C,D,A,8,20,28);

    OP2(A,B,C,D,13,5,29);  OP2(D,A,B,C,2,9,30);
    OP2(C,D,A,B,7,14,31);  OP2(B,C,D,A,12,20,32);


    OP3(A,B,C,D,5,4,33);   OP3(D,A,B,C,8,11,34);
    OP3(C,D,A,B,11,16,35); OP3(B,C,D,A,14,23,36);

    OP3(A,B,C,D,1,4,37);   OP3(D,A,B,C,4,11,38);
    OP3(C,D,A,B,7,16,39);  OP3(B,C,D,A,10,23,40);

    OP3(A,B,C,D,13,4,41);  OP3(D,A,B,C,0,11,42);
    OP3(C,D,A,B,3,16,43);  OP3(B,C,D,A,6,23,44);

    OP3(A,B,C,D,9,4,45);   OP3(D,A,B,C,12,11,46);
    OP3(C,D,A,B,15,16,47); OP3(B,C,D,A,2,23,48);


    OP4(A,B,C,D,0,6,49);   OP4(D,A,B,C,7,10,50);
    OP4(C,D,A,B,14,15,51); OP4(B,C,D,A,5,21,52);

    OP4(A,B,C,D,12,6,53);  OP4(D,A,B,C,3,10,54);
    OP4(C,D,A,B,10,15,55); OP4(B,C,D,A,1,21,56);

    OP4(A,B,C,D,8,6,57);   OP4(D,A,B,C,15,10,58);
    OP4(C,D,A,B,6,15,59);  OP4(B,C,D,A,13,21,60);

    OP4(A,B,C,D,4,6,61);   OP4(D,A,B,C,11,10,62);
    OP4(C,D,A,B,2,15,63);  OP4(B,C,D,A,9,21,64);


    A+=AA; B+=BB; C+=CC; D+=DD;
    Xlen=0;
  };
}

void MD5Sum::end(void) {
  if(computed) return;
  // pad
  uint64_t l = 8*count; // number of bits
  u_char c = 0x80;
  add(&c,1);
  c=0;
  for(;Xlen!=56;) add(&c,1);
  add(&l,8);
  computed=true;
}

int MD5Sum::print(char* buf,int len) const {
  if(!computed) { if(len>0) buf[0]=0; return 0; };
  return snprintf(buf,len,
      "md5: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
      ((u_char*)&A)[0],((u_char*)&A)[1],((u_char*)&A)[2],((u_char*)&A)[3],
      ((u_char*)&B)[0],((u_char*)&B)[1],((u_char*)&B)[2],((u_char*)&B)[3],
      ((u_char*)&C)[0],((u_char*)&C)[1],((u_char*)&C)[2],((u_char*)&C)[3],
      ((u_char*)&D)[0],((u_char*)&D)[1],((u_char*)&D)[2],((u_char*)&D)[3]
  );
}

void MD5Sum::scan(const char* buf) {
  computed=false;
  if(strncasecmp("md5:",buf,4) != 0) return;
  int l = sscanf(buf+4,
      "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
      ((u_char*)&A)+0,((u_char*)&A)+1,((u_char*)&A)+2,((u_char*)&A)+3,
      ((u_char*)&B)+0,((u_char*)&B)+1,((u_char*)&B)+2,((u_char*)&B)+3,
      ((u_char*)&C)+0,((u_char*)&C)+1,((u_char*)&C)+2,((u_char*)&C)+3,
      ((u_char*)&D)+0,((u_char*)&D)+1,((u_char*)&D)+2,((u_char*)&D)+3
  );
  if(l!=16) return;
  computed=true;
  return;
}

// ----------------------------------------------------------------------------
// This is a wrapper for any supported checksum
// ----------------------------------------------------------------------------

CheckSumAny::CheckSumAny(const char* type):cs(NULL),tp(CheckSumAny::none) {
  if(!type) return;
  if(strncasecmp("cksum",type,5) == 0) { cs=new CRC32Sum; tp=cksum; return; };
  if(strncasecmp("md5",type,3) == 0) { cs=new MD5Sum; tp=md5; return; };
}

CheckSumAny::CheckSumAny(type type) {
  if(type == cksum) { cs=new CRC32Sum; tp=type; return; }; 
  if(type == md5) { cs=new MD5Sum; tp=type; return; };
}

CheckSumAny::type CheckSumAny::Type(const char* crc) {
  if(!crc) return none;
  if(!crc[0]) return none;
  const char* p = strchr(crc,':');
  if(!p) {
    p=crc+strlen(crc);
    int i;
    for(i = 0;crc[i];i++) if(!isxdigit(crc[i])) break;
    if(!(crc[i])) return cksum;
  };
  if(((p-crc)==5) && (strncasecmp(crc,"cksum",5)==0)) return cksum;
  if(((p-crc)==3) && (strncasecmp(crc,"md5",3)==0)) return md5;
  if(((p-crc)==9) && (strncasecmp(crc,"undefined",9)==0)) return undefined;
  return unknown;
}

void CheckSumAny::operator=(const char* type) {
  if(cs) delete cs; cs=NULL; tp=none;
  if(!type) return;
  if(strncasecmp("cksum",type,5) == 0) { cs=new CRC32Sum; tp=cksum; return; };
  if(strncasecmp("md5",type,3) == 0) { cs=new MD5Sum; tp=md5; return; };
}

bool CheckSumAny::operator==(const char* s) {
  if(!cs) return false;
  if(!(*cs)) return false;
  if(!s) return false;
  CheckSumAny c(s);
  if(!(c.cs)) return false;
  c.cs->scan(s);
  if(!(*(c.cs))) return false;
  if(c.tp != tp) return false;
  unsigned char* res;
  unsigned char* res_;
  unsigned int len;
  unsigned int len_;
  cs->result(res,len);
  c.cs->result(res_,len_);
  if(len != len_) return false;
  if(memcmp(res,res_,len) != 0) return false;
  return true;
}

bool CheckSumAny::operator==(const CheckSumAny& c) {
  if(!cs) return false;
  if(!(*cs)) return false;
  if(!c) return false;
  unsigned char* res;
  unsigned char* res_;
  unsigned int len;
  unsigned int len_;
  cs->result(res,len);
  c.cs->result(res_,len_);
  if(len != len_) return false;
  if(memcmp(res,res_,len) != 0) return false;
  return true;
}

} // namespace Arc
