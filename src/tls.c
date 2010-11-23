/*
 * tls.c -- handles:
 *   TLS support functions
 *   Certificate handling
 *   OpenSSL initialization and shutdown
 *
 * $Id: tls.c,v 1.4 2010/11/23 23:25:24 pseudo Exp $
 */
/*
 * Written by Rumen Stoyanov <pseudo@egg6.net>
 *
 * Copyright (C) 2010 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "main.h"

#ifdef TLS

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

extern int tls_vfydcc;
extern struct dcc_t *dcc;

int tls_maxdepth = 9;         /* Max certificate chain verification depth     */
SSL_CTX *ssl_ctx = NULL;      /* SSL context object                           */
char *tls_randfile = NULL;    /* Random seed file for SSL                     */
char tls_capath[121] = "";    /* Path to trusted CA certificates              */
char tls_cafile[121] = "";    /* File containing trusted CA certificates      */
char tls_certfile[121] = "";  /* Our own digital certificate ;)               */
char tls_keyfile[121] = "";   /* Private key for use with eggdrop             */
char tls_ciphers[121] = "";   /* A list of ciphers for SSL to use             */


/* Count allocated memory for SSL. This excludes memory allocated by OpenSSL's
 * family of malloc functions.
 */
int expmem_tls()
{
  int i, tot;
  struct threaddata *td = threaddata();

  /* currently it's only the appdata structs allocated by ssl_handshake() */
  for (i = 0, tot = 0; i < td->MAXSOCKS; i++)
    if (!(td->socklist[i].flags & (SOCK_UNUSED | SOCK_TCL)))
      if (td->socklist[i].ssl && SSL_get_app_data(td->socklist[i].ssl))
        tot += sizeof(ssl_appdata);
  return tot;
}

/* Seeds the PRNG
 *
 * Only does something if the system doesn't have enough entropy.
 * If there is no random file, one will be created either at
 * $RANDFILE if set or at $HOME/.rnd
 *
 * Return value: 0 on success, !=0 on failure.
 */
static int ssl_seed(void)
{
  char stackdata[1024];
  static char rand_file[120];
  FILE *fh;

#ifdef HAVE_RAND_STATUS
  if (RAND_status())
    return 0;     /* Status OK */
#endif
  /* If '/dev/urandom' is present, OpenSSL will use it by default.
   * Otherwise we'll have to generate pseudorandom data ourselves,
   * using system time, our process ID and some unitialized static
   * storage.
   */
  if ((fh = fopen("/dev/urandom", "r"))) {
    fclose(fh);
    return 0;
  }
  if (RAND_file_name(rand_file, sizeof(rand_file)))
    tls_randfile = rand_file;
  else
    return 1;
  if (!RAND_load_file(rand_file, -1)) {
    /* generate some pseudo random data */
    unsigned int c;
    c = time(NULL);
    RAND_seed(&c, sizeof(c));
    c = getpid();
    RAND_seed(&c, sizeof(c));
    RAND_seed(stackdata, sizeof(stackdata));
  }
#ifdef HAVE_RAND_STATUS
  if (!RAND_status())
    return 2;   /* pseudo random data still not ehough */
#endif
  return 0;
}

/* Prepares and initializes SSL stuff
 *
 * Creates a context object, supporting SSLv2/v3 & TLSv1 protocols;
 * Seeds the Pseudo Random Number Generator;
 * Optionally loads a SSL certifate and a private key.
 * Tell OpenSSL the location of certificate authority certs
 *
 * Return value: 0 on successful initialization, !=0 on failure
 */
int ssl_init()
{
  /* Load SSL and crypto error strings; register SSL algorithms */
  SSL_load_error_strings();
  SSL_library_init();

  if (ssl_seed()) {
    putlog(LOG_MISC, "*", "TLS: unable to seed PRNG. Disabling SSL");
    ERR_free_strings();
    return -2;
  }
  /* A TLS/SSL connection established with this method will understand all
     supported protocols (SSLv2, SSLv3, and TLSv1) */
  if (!(ssl_ctx = SSL_CTX_new(SSLv23_method()))) {
    debug0(ERR_error_string(ERR_get_error(), NULL));
    putlog(LOG_MISC, "*", "TLS: unable to create context. Disabling SSL.");
    ERR_free_strings();
    return -1;
  }
  /* Load our own certificate and private key. Mandatory for acting as
     server, because we don't support anonymous ciphers by default. */
  if (SSL_CTX_use_certificate_chain_file(ssl_ctx, tls_certfile) != 1)
    debug1("TLS: unable to load own certificate: %s",
           ERR_error_string(ERR_get_error(), NULL));
  if (SSL_CTX_use_PrivateKey_file(ssl_ctx, tls_keyfile,
      SSL_FILETYPE_PEM) != 1)
    debug1("TLS: unable to load private key: %s",
           ERR_error_string(ERR_get_error(), NULL));
  if ((tls_capath[0] || tls_cafile[0]) &&
      !SSL_CTX_load_verify_locations(ssl_ctx, tls_cafile[0] ? tls_cafile : NULL,
      tls_capath[0] ? tls_capath : NULL))
    debug1("TLS: unable to set CA certificates location: %s",
           ERR_error_string(ERR_get_error(), NULL));
  /* Let advanced users specify the list of allowed ssl ciphers */
  if (tls_ciphers[0])
    if (!SSL_CTX_set_cipher_list(ssl_ctx, tls_ciphers)) {
      /* this replaces any preset ciphers so an invalid list is fatal */
      putlog(LOG_MISC, "*", "TLS: no valid ciphers found. Disabling SSL.");
      ERR_free_strings();
      SSL_CTX_free(ssl_ctx);
      ssl_ctx = NULL;
      return -3;
    }

  return 0;
}

/* Free the SSL CTX, clean up the mess */
void ssl_cleanup()
{
  if (ssl_ctx) {
    SSL_CTX_free(ssl_ctx);
    ssl_ctx = NULL;
  }
  if (tls_randfile)
    RAND_write_file(tls_randfile);
  ERR_free_strings();
}

char *ssl_fpconv(char *in, char *out)
{
  long len;
  char *fp;
  unsigned char *md5;
  
  if (!in)
    return NULL;
  
  if ((md5 = string_to_hex(in, &len))) {
    fp = hex_to_string(md5, len);
    if (fp) {
      out = user_realloc(out, strlen(fp) + 1);
      strcpy(out, fp);
      OPENSSL_free(md5);
      OPENSSL_free(fp);
      return out;
    }
      OPENSSL_free(md5);
  }
  return NULL;
}

/* Get the certificate, corresponding to the connection
 * identified by sock.
 *
 * Return value: pointer to a X509 certificate or NULL if we couldn't
 * look up the certificate.
 */
static X509 *ssl_getcert(int sock)
{
  int i;
  struct threaddata *td = threaddata();
  
  i = findsock(sock);
  if (i == -1 || !td->socklist[i].ssl)
    return NULL;
  return SSL_get_peer_certificate(td->socklist[i].ssl);
}

/* Get the certificate fingerprint of the connection corresponding
 * to the socket.
 *
 * Return value: ptr to the hexadecimal representation of the fingerprint
 * or NULL if there's no certificate associated with the connection.
 */
char *ssl_getfp(int sock)
{
  char *p;
  unsigned i;
  X509 *cert;
  static char fp[64];
  unsigned char md[EVP_MAX_MD_SIZE];

  if (!(cert = ssl_getcert(sock)))
    return NULL;
  if (!X509_digest(cert, EVP_sha1(), md, &i))
    return NULL;
  if (!(p = hex_to_string(md, i)))
    return NULL;
  strncpyz(fp, p, sizeof fp);
  OPENSSL_free(p);
  return fp;
}

/* Get the UID field from the certificate subject name.
 * The certificate is looked up using the socket of the connection.
 *
 * Return value: Pointer to the uid string or NULL if not found
 */
char *ssl_getuid(int sock)
{
  int idx;
  X509 *cert;
  X509_NAME *subj;
  ASN1_STRING *name;

  if (!(cert = ssl_getcert(sock)))
    return NULL;
  /* Get the subject name */
  if (!(subj = X509_get_subject_name(cert)))
    return NULL;

  /* Get the first UID */
  idx = X509_NAME_get_index_by_NID(subj, NID_userId, -1);
  if (idx == -1)
    return NULL;
  name = X509_NAME_ENTRY_get_data(X509_NAME_get_entry(subj, idx));
  /* Extract the contents, assuming null-terminated ASCII string */
  return (char *) ASN1_STRING_data(name);
}

/* Compare the peer's host with their Common Name or dnsName found in
 * it's certificate. Only the first domain component of cn is allowed to
 * be a wildcard '*'. The non-wildcard characters are compared ignoring
 * case.
 *
 * Return value: 1 if cn matches host, 0 otherwise.
 */
static int ssl_hostmatch(char *cn, char *host)
{
  char *p, *q, *r;

  if ((r = strchr(cn + 1, '.')) && r[-1] == '*' && strchr(r, '.')) {
    for (p = cn, q = host; *p != '*'; p++, q++)
      if (toupper(*p) != toupper(*q))
        return 0;

    if (!(p = strchr(host, '.')) || strcasecmp(p, r))
      return 0;
    return 1;
  }

  /* First domain component is not a wildcard and they aren't allowed
     elsewhere, so just compare the strings. */
  return strcasecmp(cn, host) ? 0 : 1;
}

/* Confirm the peer identity, by checking if the certificate subject 
 * matches the peer's DNS name or IP address. Matching is performed in
 * accordance with RFC 2818:
 *
 * If the certificate has a subjectAltName extension, all names of type
 * IPAddress or dnsName present there, will be compared to data->host,
 * depending on it's contents.
 * In case there's no subjectAltName extension, commonName (CN) parts
 * of the certificate subject field will be used instead of IPAddress
 * and dnsName entries. For IP addresses, common names must contain IPs
 * in presentation format (1.2.3.4 or 2001:DB8:15:dead::)
 * Finally, if no subjectAltName or common names are present, the
 * certificate is considered to not match the peer.
 *
 * The structure of X509 certificates and all fields referenced above
 * are described in RFC 5280.
 *
 * The certificate must be pointed by cert and the peer's host must be
 * placed in data->host. The format is a regular DNS name or an IP in
 * presentation format (see above).
 * 
 * Return value: 1 if the certificate matches the peer, 0 otherwise.
 */
static int ssl_verifycn(X509 *cert, ssl_appdata *data)
{
  char *cn;
  int crit = 0, match = 0;
  ASN1_OCTET_STRING *ip;
  GENERAL_NAMES *altname; /* SubjectAltName ::= GeneralNames */
  
  ip = a2i_IPADDRESS(data->host); /* check if it's an IP or a hostname */
  if ((altname = X509_get_ext_d2i(cert, NID_subject_alt_name, &crit, NULL))) {
    GENERAL_NAME *gn;

    /* Loop through the general names in altname and pick these
       of type ip address or dns name */
    while (!match && (gn = sk_GENERAL_NAME_pop(altname))) {
      /* if the peer's host is an IP, we're only interested in
         matching against iPAddress general names, otherwise
         we'll only look for dnsName's */
      if (ip) {
        if (gn->type == GEN_IPADD)
          match = !ASN1_STRING_cmp(gn->d.ip, ip);
      } else if (gn->type == GEN_DNS) {
        /* IA5string holds ASCII data */
        cn = (char *) ASN1_STRING_data(gn->d.ia5);
        match = ssl_hostmatch(cn, data->host);
      }
    }
    sk_GENERAL_NAME_free(altname);
  } else { /* no subjectAltName, try to match against the subject CNs */
    X509_NAME *subj; /* certificate subject */

    /* the following is just for information */
    switch (crit) {
      case 0:
        debug0("TLS: X509 subjectAltName cannot be decoded");
        break;
      case -1:
        debug0("TLS: X509 has no subjectAltName extension");
        break;
      case -2:
        debug0("TLS: X509 has multiple subjectAltName extensions");
    }
    /* no subject name either? A completely broken certificate :) */
    if (!(subj = X509_get_subject_name(cert))) {
      putlog(data->loglevel, "*", "TLS: peer certificate has no subject: %s",
             data->host);
      match = 0;
    } else { /* we have a subject name, look at it */
      int pos = -1;
      ASN1_STRING *name;
      
      /* Look for commonName attributes in the subject name */
      pos = X509_NAME_get_index_by_NID(subj, NID_commonName, pos);
      if (pos == -1) /* sorry */
        putlog(data->loglevel, "*", "TLS: Peer has no common names and "
              "no subjectAltName extension. Verification failed.");
      /* Loop through all common names which may be present in the subject
         name until we find a match. */
      while (!match && pos != -1) {
        name = X509_NAME_ENTRY_get_data(X509_NAME_get_entry(subj, pos));
        cn = (char *) ASN1_STRING_data(name);
        if (ip)
          match = a2i_IPADDRESS(cn) ? (ASN1_STRING_cmp(ip, a2i_IPADDRESS(cn)) ? 0 : 1) : 0;
        else
          match = ssl_hostmatch(cn, data->host);
        pos = X509_NAME_get_index_by_NID(subj, NID_commonName, pos);
      }
    }
  }

  if (ip)
    ASN1_OCTET_STRING_free(ip);
  return match;
}

/* Extract a human readable version of a X509_NAME and put the result
 * into a nmalloc'd buffer.
 * The X509_NAME structure is used for example in certificate subject
 * and issuer names.
 *
 * You need to nfree() the returned pointer.
 */
static char *ssl_printname(X509_NAME *name)
{
  int len;
  char *data, *buf;
  BIO *bio = BIO_new(BIO_s_mem());
  
  /* X509_NAME_oneline() is easier and shorter, but is deprecated and
     the manual discourages it's usage, so let's not be lazy ;) */
  X509_NAME_print_ex(bio, name, 0, XN_FLAG_ONELINE & ~XN_FLAG_SPC_EQ);
  len = BIO_get_mem_data(bio, &data) + 1;
  buf = nmalloc(len);
  strncpyz(buf, data, len);
  BIO_free(bio);
  return buf;
}

/* Print the time from a ASN1_UTCTIME object in standard format i.e.
 * Nov 21 23:59:00 1996 GMT and store it in a nmalloc'd buffer.
 * The ASN1_UTCTIME structure is what's used for example with
 * certificate validity dates.
 *
 * You need to nfree() the returned pointer.
 */
static char *ssl_printtime(ASN1_UTCTIME *t)
{
  int len;
  char *data, *buf;
  BIO *bio = BIO_new(BIO_s_mem());
  
  ASN1_UTCTIME_print(bio, t);
  len = BIO_get_mem_data(bio, &data) + 1;
  buf = nmalloc(len);
  strncpyz(buf, data, len);
  BIO_free(bio);
  return buf;
}

/* Print the value of an ASN1_INTEGER in hexadecimal format.
 * A typical use for this is to display certificate serial numbers.
 * As usual, we use a memory BIO.
 *
 * You need to nfree() the returned pointer.
 */
static char *ssl_printnum(ASN1_INTEGER *i)
{
  int len;
  char *data, *buf;
  BIO *bio = BIO_new(BIO_s_mem());
  
  i2a_ASN1_INTEGER(bio, i);
  len = BIO_get_mem_data(bio, &data) + 1;
  buf = nmalloc(len);
  strncpyz(buf, data, len);
  BIO_free(bio);
  return buf;
}

/* Show the user all relevant information about a certificate: subject,
 * issuer, validity dates and fingerprints.
 */
static void ssl_showcert(X509 *cert, int loglev)
{
  char *buf, *from, *to;
  X509_NAME *name;
  unsigned int len;
  unsigned char md[EVP_MAX_MD_SIZE];
  
  /* Subject and issuer names */
  if ((name = X509_get_subject_name(cert))) {
    buf = ssl_printname(name);
    putlog(loglev, "*", "TLS: certificate subject: %s", buf);
    nfree(buf);
  } else
    putlog(loglev, "*", "TLS: cannot get subject name from certificate!");
  if ((name = X509_get_issuer_name(cert))) {
    buf = ssl_printname(name);
    putlog(loglev, "*", "TLS: certificate issuer: %s", buf);
    nfree(buf);
  } else
    putlog(loglev, "*", "TLS: cannot get issuer name from certificate!");
  
  /* Fingerprints */
  X509_digest(cert, EVP_md5(), md, &len); /* MD5 hash */
  if (len <= sizeof(md)) {
    buf = hex_to_string(md, len);
    putlog(loglev, "*", "TLS: certificate MD5 Fingerprint: %s", buf);
    OPENSSL_free(buf);
  }
  X509_digest(cert, EVP_sha1(), md, &len); /* SHA-1 hash */
  if (len <= sizeof(md)) {
    buf = hex_to_string(md, len);
    putlog(loglev, "*", "TLS: certificate SHA1 Fingerprint: %s", buf);
    OPENSSL_free(buf);
  }

  /* Validity time */
  from = ssl_printtime(X509_get_notBefore(cert));
  to = ssl_printtime(X509_get_notAfter(cert));
  putlog(loglev, "*", "TLS: certificate valid from %s to %s", from, to);
  nfree(from);
  nfree(to);
}

/* Certificate validation callback
 *
 * Check if the certificate given is valid with respect to the
 * ssl-verify config variable. This makes it possible to allow
 * self-signed certificates and is also a convenient place to
 * extract a certificate summary.
 *
 * Return value: 1 - validation passed, 0 - invalid cert
 */
int ssl_verify(int ok, X509_STORE_CTX *ctx)
{
  SSL *ssl;
  X509 *cert;
  ssl_appdata *data;
  int err, depth;
  
  /* get cert, callbacks, error codes, etc. */
  depth = X509_STORE_CTX_get_error_depth(ctx);
  cert = X509_STORE_CTX_get_current_cert(ctx);
  ssl = X509_STORE_CTX_get_ex_data(ctx,
                          SSL_get_ex_data_X509_STORE_CTX_idx());
  data = (ssl_appdata *) SSL_get_app_data(ssl);
  err = X509_STORE_CTX_get_error(ctx);

  /* OpenSSL won't explicitly generate this error; instead it will
   * report missing certificates. Refer to SSL_CTX_set_verify(3)
   * manual for details
   */
  if (depth > tls_maxdepth) {
    ok = 0;
    err = X509_V_ERR_CERT_CHAIN_TOO_LONG;

  /* depth 0 is actually the peer certificate. We do all custom
   * verification here and leave the rest of the certificate chain
   * to OpenSSL's built in procedures.
   */
  } else if (!depth) {
    /* OpenSSL doesn't perform subject name verification. We need to do
     * it ourselves. We check here for validity even if it's not requested
     * in order to be able to warn the user.
     */
    if (!(data->flags & TLS_DEPTH0) && !ssl_verifycn(cert, data) &&
        (data->verify & TLS_VERIFYCN)) {
        putlog(data->loglevel, "*", "TLS: certificate validation failed. "
               "Certificate subject does not match peer.");
        return 0;
    }
    data->flags |= TLS_DEPTH0;
    /* Allow exceptions for certain common verification errors, if the
     * caller requested so. A lot of servers provide completely invalid
     * certificates unuseful for any authentication.
     */
    if (!ok || data->verify)
      if (((err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) &&
          (data->verify & TLS_VERIFYISSUER)) ||
          ((err == X509_V_ERR_CERT_REVOKED) &&
          (data->verify & TLS_VERIFYREV)) ||
          ((err == X509_V_ERR_CERT_NOT_YET_VALID) &&
          (data->verify & TLS_VERIFYFROM)) ||
          ((err == X509_V_ERR_CERT_HAS_EXPIRED) &&
          (data->verify & TLS_VERIFYTO))) {
        debug1("TLS: peer certificate warning: %s",
               X509_verify_cert_error_string(err));
        ok = 1;
      }
  }
  if (ok || !data->verify)
    return 1;
  putlog(data->loglevel, "*",
         "TLS: certificate validation failed at depth %d: %s",
         depth, X509_verify_cert_error_string(err));
  return 0;
}

/* SSL info callback, this is used to trace engine state changes
 * and to check when the handshake is finished, so we can display
 * some cipher and session information and process callbacks.
 */
void ssl_info(SSL *ssl, int where, int ret)
{
  int sock;
  X509 *cert;
  char buf[256];
  ssl_appdata *data;
  SSL_CIPHER *cipher;
  int secret, processed;
  
  /* We're doing non-blocking IO, so we check here if the handshake has
     finished */
  if (where & SSL_CB_HANDSHAKE_DONE) {
    if (!(data = (ssl_appdata *) SSL_get_app_data(ssl)))
      return;
    /* Callback for completed handshake. Cheaper and more convenient than
       using H_tls */
    sock = SSL_get_fd(ssl);    
    if (data->cb)
      data->cb(sock);
    /* Call TLS binds. We allow scripts to take over or disable displaying of
       certificate information. */
    if (check_tcl_tls(sock))
      return;

    putlog(data->loglevel, "*", "TLS: handshake successful. Secure connection "
           "established.");

    if ((cert = SSL_get_peer_certificate(ssl))) 
      ssl_showcert(cert, data->loglevel);
    else
      putlog(data->loglevel, "*", "TLS: peer did not present a certificate");

    /* Display cipher information */
    cipher = SSL_get_current_cipher(ssl);
    processed = SSL_CIPHER_get_bits(cipher, &secret);
    putlog(data->loglevel, "*", "TLS: cipher used: %s %s; %d bits (%d secret)",
           SSL_CIPHER_get_name(cipher), SSL_CIPHER_get_version(cipher),
           processed, secret);
    /* secret are the actually secret bits. If processed and secret differ,
       the rest of the bits are fixed, i.e. for limited export ciphers */

    /* More verbose information, for debugging only */
    SSL_CIPHER_description(cipher, buf, sizeof buf);
    debug1("TLS: cipher details: %s", buf);
  }

  /* Display the state of the engine for debugging purposes */
  /* debug1("TLS: state change: %s", SSL_state_string_long(ssl)); */
}
    
/* Switch a socket to SSL communication
 *
 * Creates a SSL data structure for the connection;
 * Sets up callbacks and initiates a SSL handshake with the peer;
 * Reports error conditions and performs cleanup upon failure.
 *
 * flags: ssl flags, i.e connect or listen
 * verify: peer certificate verification flags
 * loglevel: is the level to output information about the connection
 * and certificates.
 * host: contains the dns name or ip address of the peer. Used for
 * verification.
 * cb: optional callback, this function will be called after the
 * handshake completes.
 *
 * Return value: 0 on success, !=0 on failure.
 */
int ssl_handshake(int sock, int flags, int verify, int loglevel, char *host,
                  IntFunc cb)
{
  int i, err, ret;
  ssl_appdata *data;
  struct threaddata *td = threaddata();

  debug0("TLS: attempting SSL negotiation...");
  if (!ssl_ctx && ssl_init()) {
    debug0("TLS: Failed. OpenSSL not initialized properly.");
    return -1;
  }
  /* find the socket in the list */
  i = findsock(sock);
  if (i == -1) {
    debug0("TLS: socket not in socklist");
    return -2;
  }
  if (td->socklist[i].ssl) {
    debug0("TLS: handshake not required - SSL session already established");
    return 0;
  }
  td->socklist[i].ssl = SSL_new(ssl_ctx);
  if (!td->socklist[i].ssl ||
      !SSL_set_fd(td->socklist[i].ssl, td->socklist[i].sock)) {
    debug1("TLS: cannot initiate SSL session - %s",
           ERR_error_string(ERR_get_error(), 0));
    return -3;
  }

  /* Prepare a ssl appdata struct for the verify callback */
  data = nmalloc(sizeof(ssl_appdata));
  egg_bzero(data, sizeof(ssl_appdata));
  data->flags = flags & (TLS_LISTEN | TLS_CONNECT);
  data->verify = flags & ~(TLS_LISTEN | TLS_CONNECT);
  data->loglevel = loglevel;
  data->cb = cb;
  strncpyz(data->host, host ? host : "", sizeof(data->host));
  SSL_set_app_data(td->socklist[i].ssl, data);
  SSL_set_info_callback(td->socklist[i].ssl, (void *) ssl_info);
  /* We set this +1 to be able to report extra long chains properly.
   * Otherwise, OpenSSL will break the verification reporting about
   * missing certificates instead. The rest of the fix is in
   * ssl_verify()
   */
  SSL_set_verify_depth(td->socklist[i].ssl, tls_maxdepth + 1);

  SSL_set_mode(td->socklist[i].ssl, SSL_MODE_ENABLE_PARTIAL_WRITE);
  if (data->flags & TLS_CONNECT) {
    SSL_set_verify(td->socklist[i].ssl, SSL_VERIFY_PEER, ssl_verify);
    ret = SSL_connect(td->socklist[i].ssl);
    if (!ret)
      debug0("TLS: connect handshake failed.");
  } else {
    if (data->flags & TLS_VERIFYPEER)
      SSL_set_verify(td->socklist[i].ssl, SSL_VERIFY_PEER |
                     SSL_VERIFY_FAIL_IF_NO_PEER_CERT, ssl_verify);
    else
      SSL_set_verify(td->socklist[i].ssl, SSL_VERIFY_PEER, ssl_verify);
    ret = SSL_accept(td->socklist[i].ssl);
    if (!ret)
      debug0("TLS: accept handshake failed");
  }

  err = SSL_get_error(td->socklist[i].ssl, ret);
  /* Normal condition for async I/O, similar to EAGAIN */
  if (ret > 0 || err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
    debug0("TLS: handshake in progress");
    return 0;
  }
  if (ERR_peek_error())
    debug0("TLS: handshake failed due to the following errors: ");
  while ((err = ERR_get_error()))
    debug1("TLS: %s", ERR_error_string(err, NULL));

  /* Attempt failed, cleanup and abort */
  SSL_shutdown(td->socklist[i].ssl);
  SSL_free(td->socklist[i].ssl);
  td->socklist[i].ssl = NULL;
  nfree(data);
  return -4;
}

/* Tcl functions */

/* Is the connection secure? */
static int tcl_istls STDVAR
{
  int j;

  BADARGS(2, 2, " idx");

  j = findidx(atoi(argv[1]));
  if (j < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[j].ssl)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

/* Perform a SSL handshake over an existing plain text
 * connection.
 */
static int tcl_starttls STDVAR
{
  int j;
  struct threaddata *td = threaddata();

  BADARGS(2, 2, " idx");

  j = findidx(atoi(argv[1]));
  if (j < 0 || (dcc[j].type != &DCC_SCRIPT)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[j].ssl) {
    Tcl_AppendResult(irp, "already started", NULL);
    return TCL_ERROR;
  }
  /* Determine if we're playing a client or a server */
  j = findsock(dcc[j].sock);
  if (ssl_handshake(dcc[j].sock, (td->socklist[j].flags & SOCK_CONNECT) ?
      TLS_CONNECT : TLS_LISTEN, tls_vfydcc, LOG_MISC, NULL, NULL))
    Tcl_AppendResult(irp, "0", NULL);
  else
    Tcl_AppendResult(irp, "1", NULL);
  return TCL_OK;
}

/* Get all relevant information about an established ssl connection.
 * This includes certificate subject and issuer, serial number,
 * expiry date, protocol version and cipher information.
 * All data is presented as a flat list consisting of name-value pairs.
 */
static int tcl_tlsstatus STDVAR
{
  char *p;
  int i, j;
  X509 *cert;
  SSL_CIPHER *cipher;
  struct threaddata *td = threaddata();
  Tcl_DString ds;

  BADARGS(2, 2, " idx");

  /* Allow it to be used for any connection, not just scripted
   * ones. This makes it possible for a script to display the
   * server certificate.
   */
  i = findanyidx(atoi(argv[1]));
  if (i < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  j = findsock(dcc[i].sock);
  if (!j || !dcc[i].ssl || !td->socklist[j].ssl) {
    Tcl_AppendResult(irp, "not a TLS connection", NULL);
    return TCL_ERROR;
  }
  
  Tcl_DStringInit(&ds);
  /* Try to get a cert, clients aren't required to send a
   * certificate, so this is optional
   */
  cert = SSL_get_peer_certificate(td->socklist[j].ssl);
  /* The following information is certificate dependent */
  if (cert) {
    p = ssl_printname(X509_get_subject_name(cert));
    Tcl_DStringAppendElement(&ds, "subject");
    Tcl_DStringAppendElement(&ds, p);
    nfree(p);
    p = ssl_printname(X509_get_issuer_name(cert));
    Tcl_DStringAppendElement(&ds, "issuer");
    Tcl_DStringAppendElement(&ds, p);
    nfree(p);
    p = ssl_printtime(X509_get_notBefore(cert));
    Tcl_DStringAppendElement(&ds, "notBefore");
    Tcl_DStringAppendElement(&ds, p);
    nfree(p);
    p = ssl_printtime(X509_get_notAfter(cert));
    Tcl_DStringAppendElement(&ds, "notAfter");
    Tcl_DStringAppendElement(&ds, p);
    nfree(p);
    p = ssl_printnum(X509_get_serialNumber(cert));
    Tcl_DStringAppendElement(&ds, "serial");
    Tcl_DStringAppendElement(&ds, p);
    nfree(p);
  }
  /* We should always have a cipher, but who knows? */
  cipher = SSL_get_current_cipher(td->socklist[j].ssl);
  if (cipher) { /* don't bother if there's none */
    Tcl_DStringAppendElement(&ds, "protocol");
    Tcl_DStringAppendElement(&ds, SSL_CIPHER_get_version(cipher));
    Tcl_DStringAppendElement(&ds, "cipher");
    Tcl_DStringAppendElement(&ds, SSL_CIPHER_get_name(cipher));
  }

  /* Done, get a Tcl list from this and return it to the caller */
  Tcl_AppendResult(irp, Tcl_DStringValue(&ds), NULL);
  Tcl_DStringFree(&ds);
  return TCL_OK;
}

/* These will be added by tcl.c which is the established practice */
tcl_cmds tcltls_cmds[] = {
  {"istls",         tcl_istls},
  {"starttls",   tcl_starttls},
  {"tlsstatus", tcl_tlsstatus},
  {NULL,                 NULL}
};

#endif /* TLS */
