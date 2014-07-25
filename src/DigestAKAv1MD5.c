/*
  The UCT IMS Client
  Copyright (C) 2006 - University of Cape Town
  David Waiting <david@crg.ee.uct.ac.za>
  Richard Good <rgood@crg.ee.uct.ac.za>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
 

#include "includes.h"
#include "base64.c"
#include "milenage.c"
#include "rfc2617.c"
#include "md5.c"


/** length of the Secret-Key */
#define KLEN 16
/** Secret-Key type */
typedef u_char K[KLEN];
/** length of the Random */
#define RANDLEN 16
/** Random type */
typedef u_char RAND[RANDLEN];
/** length of the Authentication Nonce */
#define AUTNLEN 16
/** Authentication Nonce type */
typedef u_char AUTN[AUTNLEN];

/** length of the Anonymity Key */
#define AKLEN 6
/** Anonymity Key type */
typedef u_char AK[AKLEN];
/** length of the AMF */
#define AMFLEN 2
/** AMF type */
typedef u_char AMF[AMFLEN];
/** length of the Media Authorization C */
#define MACLEN 8
/** Media Authorization C type */
typedef u_char MAC[MACLEN];
/** length of the Cypher Key */
#define CKLEN 16
/** Cypher Key type */
typedef u_char CK[CKLEN];
/** length of the Integrity Key */
#define IKLEN 16
/** Integrity Key type */
typedef u_char IK[IKLEN];

/** length of the Authorization Synchronization */
#define AUTSLEN 14
/** Authorization Synchronization type */
typedef char AUTS[AUTSLEN];
/** length of the Authorization Synchronization in base 64*/
#define AUTS64LEN 21
/** Authorization Synchronization in base64 type */
typedef char AUTS64[AUTS64LEN];
/** length of the Respones */
#define RESLEN 8
/** Response type */
typedef unsigned char RES[RESLEN];
/** Length of the Response in base16 */
#define RESHEXLEN 17
/** Response in base16 type */
typedef char RESHEX[RESHEXLEN];

#define IN
#define OUT
/** the result is a Response */
#define AKAV1MD5_RESPONSE 	0
/** the result is a Authorization Syncrhonization */
#define AKAV1MD5_AUTS 		1
/** if to use Anonimity Key to decrypt/encrypt the SeQuence Number */
#define HSS3G_USE_ANONIMITY_KEY 0

/** base16 conversion constants */
char hexa[16]="0123456789abcdef";

static str empty_s={0,0};
/**
 * Calculate the MD5 response.
 * @param msg - the SIP message to calculate for
 * @param aka_response - the AKA response to feed as password for MD5
 * @param private_identity - the authentication username
 * @param realm - the authentication realm
 * @param nonce - the nonce as received
 * @param uri - the uri as received
 * @returns the MD5 response or an empty string on error 
 */
str MD5(str aka_response, str private_identity,str realm,str nonce,str uri)
{
	HASHHEX expected,ha1,hbody;
	str response={0,0};

	str method;
	method.s = "REGISTER";
	method.len = 8;
	
	calc_HA1(HA_MD5,&private_identity,&realm,&(aka_response),&(nonce),0,ha1);

	calc_response(ha1,&nonce,
			&empty_s,&empty_s,&empty_s,0,
			&method,&uri,hbody,expected);

	response.s = malloc(HASHHEXLEN+1);

	if (!response.s) {
	 	printf("MD5: error allocating %d bytes\n",HASHHEXLEN+1);
		return response;
	}

	response.len = HASHHEXLEN;

	memcpy(response.s,expected,HASHHEXLEN);

	response.s[HASHHEXLEN]=0;

	return response;


}

/**
 * Calculate the AKA response.
 * Works for v1 and v2. Ignores SQN desynchronizations.
 * @param version - version of the algorithm to apply
 * @param nonce64 - nonce as received
 * @param k_s - secret key
 * @returns the AKA response in base 64 or an empty string on error 
  */
str AKA(int version,str nonce64, str k_s) 
{
	int i,j,noncelen;
	char *nonce=0;
	RAND rand;
	K k;
	RES res;
	RESHEX response;
	str rsp={0,0};
	CK ck;
	IK ik;
	AK ak;

	/* init stuff */
	memset(response,0,RESHEXLEN);

	/* Extract nonce -> rand,autn */
//    fprintf(stderr, "DigestAKAv1CalcMilenage: Nonce64 is %s\n",nonce64);
	nonce = base64_decode_string(nonce64.s,nonce64.len,&noncelen);

	if (noncelen<RANDLEN+AUTNLEN) {
		fprintf(stderr,"DigestAKAv1CalcMilenage: Nonce is too short %d < %d expected \n",
			noncelen,RANDLEN+AUTNLEN);
		goto done;

	}
	for(i=0;i<RANDLEN;i++)
		rand[i]=(u_char)nonce[i];

	j = k_s.len;
	for(i=0;i<j&&i<KLEN;i++)
		k[i]=(u_char)k_s.s[i];
/* HACK TO PUT ALWAYS secret key 0 */		
//	j=0;
/* HACK TO PUT ALWAYS secret key 0 */	
	
	for(i=j;i<KLEN;i++)
		k[i]=0;

	/* Calculate Response and keys */
	f2345(k,rand,res,ck,ik,ak);

	/* Get the SQN_HE */
//	if (HSS3G_USE_ANONIMITY_KEY)
//		for(i=0;i<SQNLEN;i++)
//			sqn_he[i]=((u_char)nonce[RANDLEN+i])^ak[i];
//	else
//		for(i=0;i<SQNLEN;i++)
//			sqn_he[i]=((u_char)nonce[RANDLEN+i]);


	/* Check SQN and compute AUTS if needed*/
//	if (sequence_in_range(sqn_he)){
//		sequence_set_last(sqn_he);
//		result = AKAV1MD5_RESPONSE;
//	} else {
//		sequence_get_sqn(sqn_ms);
//		if (HSS3G_USE_ANONIMITY_KEY){
//			f5star(k,rand,ak);
//			for(i=0;i<SQNLEN;i++)
//				auts_bin[i]=sqn_ms[i]^ak[i];
//		}
//		else
//			for(i=0;i<SQNLEN;i++)
//				auts_bin[i]=((u_char)sqn_ms[i]);
//
//		f1star(k,rand,sqn_ms,amfstar,auts_bin+SQNLEN);
//		result = AKAV1MD5_AUTS;
//	}

	/* Format data for output in the SIP message */

/*
	for(i=0;i<RESLEN;i++){
		response[2*i]=hexa[(res[i]&0xF0)>>4];
		response[2*i+1]=hexa[res[i]&0x0F];
	}


	response[RESHEXLEN-1]=0;
*/

				
//	/*for(i=0;i<AUTSLEN;i++){
//		auts[2*i]=hexa[(auts_bin[i]&0xF0)>>4];
//		auts[2*i+1]=hexa[auts_bin[i]&0x0F];
//	}
//	auts[AUTSHEXLEN-1]=0;*/
//		
//	auts64 = base64_encode_string2(auts_bin,AUTSLEN,&auts64len);
//
//	memcpy(auts,auts64,auts64len);	
//	auts[AUTS64LEN-1]=0;
//	free(auts64);
	
done:
	if (nonce) free(nonce);
	switch (version){
		case 1:
			/* AKA v1 */
			rsp.s = malloc(RESLEN);
			if (!rsp.s){
				fprintf(stderr, "AKA: Error allocating %d bytes\n",RESLEN);
				return rsp;
			}
			rsp.len=RESLEN;
			memcpy(rsp.s,res,RESLEN);
		    return rsp;
		case 2:
			/* AKA v2 */
			rsp.len=RESLEN+IKLEN+CKLEN;
			rsp.s = malloc(rsp.len);

			if (!rsp.s){
				fprintf(stderr, "AKA: Error allocating %d bytes\n",rsp.len);
				rsp.len=0;
				return rsp;
			}

			memcpy(rsp.s,res,RESLEN);
			memcpy(rsp.s+RESLEN,ik,IKLEN);
			memcpy(rsp.s+RESLEN+IKLEN,ck,CKLEN);
			
		    return rsp;
		default:
			fprintf(stderr, "AKA: version %d not supported.\n",version);
			return rsp;
	}
}

