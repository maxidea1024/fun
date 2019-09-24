#ifndef TOMCRYPT_H_
#define TOMCRYPT_H_

#ifdef _WIN32
#define LTC_WINTHREAD
#else
#define LTC_PTHREAD
#endif

#define LTM_DESC
#define LTC_SOURCE
#define USE_LTM

#define LTC_NO_PROTOTYPES
#define LTC_NO_TEST
#define LTC_NO_CIPHERS
#define LTC_NO_MODES
#define LTC_NO_HASHES
#define LTC_NO_PRNGS
#define LTC_NO_PK
#define LTC_NO_MACS

#define LTC_RIJNDAEL	// FORTUNA required
#define LTC_HMAC		// RSA required
#define LTC_OMAC		// RSA required
#define LTC_CCM_MODE	// RSA required
#define LTC_SHA1		// RSA required
#define LTC_SHA256		// FORTUNA required
#define LTC_FORTUNA		// RSA required

/* reseed every N calls to the read function */
#define LTC_FORTUNA_WD    10
/* number of pools (4..32) can save a bit of ram by lowering the count */
#define LTC_FORTUNA_POOLS 32

/* Include RSA support */
#define LTC_MRSA

#define ARGTYPE			4	// arg null check


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#include "FunTomCrypto.h" // custom defs
/* use configuration data */
#include "tomcrypt_custom.h"

#ifdef __cplusplus
extern "C" {
#endif

	/* version */
#define CRYPT   0x0117
#define SCRYPT  "1.17"

	/* max size of either a cipher/hash block or symmetric key [largest of the two] */
#define MAXBLOCKSIZE  128

	/* descriptor table size */
#define TAB_SIZE      32

	/* error codes [will be expanded in future releases] */
	enum {
		CRYPT_OK=0,             /* result OK */
		CRYPT_ERROR,            /* Generic Error */
		CRYPT_NOP,              /* Not a failure but no operation was performed */

		CRYPT_INVALID_KEYSIZE,  /* Invalid key size given */
		CRYPT_INVALID_ROUNDS,   /* Invalid number of rounds */
		CRYPT_FAIL_TESTVECTOR,  /* Algorithm failed test vectors */

		CRYPT_BUFFER_OVERFLOW,  /* Not enough space for output */
		CRYPT_INVALID_PACKET,   /* Invalid input packet given */

		CRYPT_INVALID_PRNGSIZE, /* Invalid number of bits for a PRNG */
		CRYPT_ERROR_READPRNG,   /* Could not read enough from PRNG */

		CRYPT_INVALID_CIPHER,   /* Invalid cipher specified */
		CRYPT_INVALID_HASH,     /* Invalid hash specified */
		CRYPT_INVALID_PRNG,     /* Invalid PRNG specified */

		CRYPT_MEM,              /* Out of memory */

		CRYPT_PK_TYPE_MISMATCH, /* Not equivalent types of PK keys */
		CRYPT_PK_NOT_PRIVATE,   /* Requires a private PK key */

		CRYPT_INVALID_ARG,      /* Generic invalid argument */
		CRYPT_FILE_NOTFOUND,    /* File Not Found */

		CRYPT_PK_INVALID_TYPE,  /* Invalid type of PK key */
		CRYPT_PK_INVALID_SYSTEM,/* Invalid PK system specified */
		CRYPT_PK_DUP,           /* Duplicate key already in key ring */
		CRYPT_PK_NOT_FOUND,     /* Key not found in keyring */
		CRYPT_PK_INVALID_SIZE,  /* Invalid size input for PK parameters */

		CRYPT_INVALID_PRIME_SIZE,/* Invalid size of prime requested */
		CRYPT_PK_INVALID_PADDING /* Invalid padding on input */
	};

#include "tomcrypt_cfg.h"
#include "tomcrypt_macros.h"
#include "tomcrypt_cipher.h"
#include "tomcrypt_hash.h"
#include "tomcrypt_mac.h"
#include "tomcrypt_prng.h"
#include "tomcrypt_pk.h"
#include "tomcrypt_math.h"
#include "tomcrypt_misc.h"
#include "tomcrypt_argchk.h"
#include "tomcrypt_pkcs.h"

#ifdef __cplusplus
}
#endif

#endif /* TOMCRYPT_H_ */


/* $source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt.h,v $ */
/* $Revision: 1.21 $ */
/* $Date: 2006/12/16 19:34:05 $ */