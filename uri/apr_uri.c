/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * apr_uri.c: URI related utility things
 * 
 */

#include <stdlib.h>

#include "apu.h"
#include "apr.h"
#include "apr_general.h"
#include "apr_strings.h"

#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "apr_uri.h"

typedef struct schemes_t schemes_t;

/** Structure to store various schemes and their default ports */
struct schemes_t {
    /** The name of the scheme */
    const char *name;
    /** The default port for the scheme */
    apr_port_t default_port;
};

/* Some WWW schemes and their default ports; this is basically /etc/services */
/* This will become global when the protocol abstraction comes */
/* As the schemes are searched by a linear search, */
/* they are sorted by their expected frequency */
static schemes_t schemes[] =
{
    {"http",     APU_URI_HTTP_DEFAULT_PORT},
    {"ftp",      APU_URI_FTP_DEFAULT_PORT},
    {"https",    APU_URI_HTTPS_DEFAULT_PORT},
    {"gopher",   APU_URI_GOPHER_DEFAULT_PORT},
    {"ldap",     APU_URI_LDAP_DEFAULT_PORT},
    {"nntp",     APU_URI_NNTP_DEFAULT_PORT},
    {"snews",    APU_URI_SNEWS_DEFAULT_PORT},
    {"imap",     APU_URI_IMAP_DEFAULT_PORT},
    {"pop",      APU_URI_POP_DEFAULT_PORT},
    {"sip",      APU_URI_SIP_DEFAULT_PORT},
    {"rtsp",     APU_URI_RTSP_DEFAULT_PORT},
    {"wais",     APU_URI_WAIS_DEFAULT_PORT},
    {"z39.50r",  APU_URI_WAIS_DEFAULT_PORT},
    {"z39.50s",  APU_URI_WAIS_DEFAULT_PORT},
    {"prospero", APU_URI_PROSPERO_DEFAULT_PORT},
    {"nfs",      APU_URI_NFS_DEFAULT_PORT},
    {"tip",      APU_URI_TIP_DEFAULT_PORT},
    {"acap",     APU_URI_ACAP_DEFAULT_PORT},
    {"telnet",   APU_URI_TELNET_DEFAULT_PORT},
    {"ssh",      APU_URI_SSH_DEFAULT_PORT},
    { NULL, 0xFFFF }     /* unknown port */
};

APU_DECLARE(apr_port_t) apr_uri_default_port_for_scheme(const char *scheme_str)
{
    schemes_t *scheme;

    for (scheme = schemes; scheme->name != NULL; ++scheme)
	if (strcasecmp(scheme_str, scheme->name) == 0)
	    return scheme->default_port;

    return 0;
}

/* Unparse a apr_uri_components structure to an URI string.
 * Optionally suppress the password for security reasons.
 */
APU_DECLARE(char *) apr_uri_unparse_components(apr_pool_t *p, 
                                               const apr_uri_components *uptr, 
                                               unsigned flags)
{
    char *ret = "";

    /* If suppressing the site part, omit both user name & scheme://hostname */
    if (!(flags & APR_URI_UNP_OMITSITEPART)) {

	/* Construct a "user:password@" string, honoring the passed APR_URI_UNP_ flags: */
	if (uptr->user||uptr->password)
	    ret = apr_pstrcat (p,
			(uptr->user     && !(flags & APR_URI_UNP_OMITUSER)) ? uptr->user : "",
			(uptr->password && !(flags & APR_URI_UNP_OMITPASSWORD)) ? ":" : "",
			(uptr->password && !(flags & APR_URI_UNP_OMITPASSWORD))
			   ? ((flags & APR_URI_UNP_REVEALPASSWORD) ? uptr->password : "XXXXXXXX")
			   : "",
            ((uptr->user     && !(flags & APR_URI_UNP_OMITUSER)) ||
             (uptr->password && !(flags & APR_URI_UNP_OMITPASSWORD))) ? "@" : "", 
            NULL);

	/* Construct scheme://site string */
	if (uptr->hostname) {
	    int is_default_port;

	    is_default_port =
		(uptr->port_str == NULL ||
		 uptr->port == 0 ||
		 uptr->port == apr_uri_default_port_for_scheme(uptr->scheme));

	    ret = apr_pstrcat (p,
			uptr->scheme, "://", ret, 
			uptr->hostname ? uptr->hostname : "",
			is_default_port ? "" : ":",
			is_default_port ? "" : uptr->port_str,
			NULL);
	}
    }

    /* Should we suppress all path info? */
    if (!(flags & APR_URI_UNP_OMITPATHINFO)) {
	/* Append path, query and fragment strings: */
	ret = apr_pstrcat (p,
		ret,
		uptr->path ? uptr->path : "",
		(uptr->query    && !(flags & APR_URI_UNP_OMITQUERY)) ? "?" : "",
		(uptr->query    && !(flags & APR_URI_UNP_OMITQUERY)) ? uptr->query : "",
		(uptr->fragment && !(flags & APR_URI_UNP_OMITQUERY)) ? "#" : NULL,
		(uptr->fragment && !(flags & APR_URI_UNP_OMITQUERY)) ? uptr->fragment : NULL,
		NULL);
    }
    return ret;
}

/* Here is the hand-optimized parse_uri_components().  There are some wild
 * tricks we could pull in assembly language that we don't pull here... like we
 * can do word-at-time scans for delimiter characters using the same technique
 * that fast memchr()s use.  But that would be way non-portable. -djg
 */

/* We have a apr_table_t that we can index by character and it tells us if the
 * character is one of the interesting delimiters.  Note that we even get
 * compares for NUL for free -- it's just another delimiter.
 */

#define T_COLON		0x01	/* ':' */
#define T_SLASH		0x02	/* '/' */
#define T_QUESTION	0x04	/* '?' */
#define T_HASH		0x08	/* '#' */
#define T_NUL		0x80	/* '\0' */

/* the uri_delims.h file is autogenerated by gen_uri_delims.c */
#include "uri_delims.h"

/* it works like this:
    if (uri_delims[ch] & NOTEND_foobar) {
	then we're not at a delimiter for foobar
    }
*/

/* Note that we optimize the scheme scanning here, we cheat and let the
 * compiler know that it doesn't have to do the & masking.
 */
#define NOTEND_SCHEME	(0xff)
#define NOTEND_HOSTINFO	(T_SLASH | T_QUESTION | T_HASH | T_NUL)
#define NOTEND_PATH	(T_QUESTION | T_HASH | T_NUL)

/* parse_uri_components():
 * Parse a given URI, fill in all supplied fields of a uri_components
 * structure. This eliminates the necessity of extracting host, port,
 * path, query info repeatedly in the modules.
 * Side effects:
 *  - fills in fields of uri_components *uptr
 *  - none on any of the r->* fields
 */
APU_DECLARE(int) apr_uri_parse_components(apr_pool_t *p, const char *uri, 
                                          apr_uri_components *uptr)
{
    const char *s;
    const char *s1;
    const char *hostinfo;
    char *endstr;
    int port;

    /* Initialize the structure. parse_uri() and parse_uri_components()
     * can be called more than once per request.
     */
    memset (uptr, '\0', sizeof(*uptr));
    uptr->is_initialized = 1;

    /* We assume the processor has a branch predictor like most --
     * it assumes forward branches are untaken and backwards are taken.  That's
     * the reason for the gotos.  -djg
     */
    if (uri[0] == '/') {
deal_with_path:
	/* we expect uri to point to first character of path ... remember
	 * that the path could be empty -- http://foobar?query for example
	 */
	s = uri;
	while ((uri_delims[*(unsigned char *)s] & NOTEND_PATH) == 0) {
	    ++s;
	}
	if (s != uri) {
	    uptr->path = apr_pstrndup(p, uri, s - uri);
	}
	if (*s == 0) {
	    return APR_SUCCESS;
	}
	if (*s == '?') {
	    ++s;
	    s1 = strchr(s, '#');
	    if (s1) {
		uptr->fragment = apr_pstrdup(p, s1 + 1);
		uptr->query = apr_pstrndup(p, s, s1 - s);
	    }
	    else {
		uptr->query = apr_pstrdup(p, s);
	    }
	    return APR_SUCCESS;
	}
	/* otherwise it's a fragment */
	uptr->fragment = apr_pstrdup(p, s + 1);
	return APR_SUCCESS;
    }

    /* find the scheme: */
    s = uri;
    while ((uri_delims[*(unsigned char *)s] & NOTEND_SCHEME) == 0) {
	++s;
    }
    /* scheme must be non-empty and followed by :// */
    if (s == uri || s[0] != ':' || s[1] != '/' || s[2] != '/') {
	goto deal_with_path;	/* backwards predicted taken! */
    }

    uptr->scheme = apr_pstrndup(p, uri, s - uri);
    s += 3;
    hostinfo = s;
    while ((uri_delims[*(unsigned char *)s] & NOTEND_HOSTINFO) == 0) {
	++s;
    }
    uri = s;	/* whatever follows hostinfo is start of uri */
    uptr->hostinfo = apr_pstrndup(p, hostinfo, uri - hostinfo);

    /* If there's a username:password@host:port, the @ we want is the last @...
     * too bad there's no memrchr()... For the C purists, note that hostinfo
     * is definately not the first character of the original uri so therefore
     * &hostinfo[-1] < &hostinfo[0] ... and this loop is valid C.
     */
    do {
	--s;
    } while (s >= hostinfo && *s != '@');
    if (s < hostinfo) {
	/* again we want the common case to be fall through */
deal_with_host:
	/* We expect hostinfo to point to the first character of
	 * the hostname.  If there's a port it is the first colon.
	 */
	s = memchr(hostinfo, ':', uri - hostinfo);
	if (s == NULL) {
	    /* we expect the common case to have no port */
	    uptr->hostname = apr_pstrndup(p, hostinfo, uri - hostinfo);
	    goto deal_with_path;
	}
	uptr->hostname = apr_pstrndup(p, hostinfo, s - hostinfo);
	++s;
	uptr->port_str = apr_pstrndup(p, s, uri - s);
	if (uri != s) {
	    port = strtol(uptr->port_str, &endstr, 10);
	    uptr->port = port;
	    if (*endstr == '\0') {
		goto deal_with_path;
	    }
	    /* Invalid characters after ':' found */
	    return APR_EGENERAL;
	}
	uptr->port = apr_uri_default_port_for_scheme(uptr->scheme);
	goto deal_with_path;
    }

    /* first colon delimits username:password */
    s1 = memchr(hostinfo, ':', s - hostinfo);
    if (s1) {
	uptr->user = apr_pstrndup(p, hostinfo, s1 - hostinfo);
	++s1;
	uptr->password = apr_pstrndup(p, s1, s - s1);
    }
    else {
	uptr->user = apr_pstrndup(p, hostinfo, s - hostinfo);
    }
    hostinfo = s + 1;
    goto deal_with_host;
}

/* Special case for CONNECT parsing: it comes with the hostinfo part only */
/* See the INTERNET-DRAFT document "Tunneling SSL Through a WWW Proxy"
 * currently at http://www.mcom.com/newsref/std/tunneling_ssl.html
 * for the format of the "CONNECT host:port HTTP/1.0" request
 */
APU_DECLARE(int) apr_uri_parse_hostinfo_components(apr_pool_t *p, 
                                                   const char *hostinfo, 
                                                   apr_uri_components *uptr)
{
    const char *s;
    char *endstr;

    /* Initialize the structure. parse_uri() and parse_uri_components()
     * can be called more than once per request.
     */
    memset (uptr, '\0', sizeof(*uptr));
    uptr->is_initialized = 1;
    uptr->hostinfo = apr_pstrdup(p, hostinfo);

    /* We expect hostinfo to point to the first character of
     * the hostname.  There must be a port, separated by a colon
     */
    s = strchr(hostinfo, ':');
    if (s == NULL) {
	return APR_EGENERAL;
    }
    uptr->hostname = apr_pstrndup(p, hostinfo, s - hostinfo);
    ++s;
    uptr->port_str = apr_pstrdup(p, s);
    if (*s != '\0') {
	uptr->port = (unsigned short) strtol(uptr->port_str, &endstr, 10);
	if (*endstr == '\0') {
	    return APR_SUCCESS;
	}
	/* Invalid characters after ':' found */
    }
    return APR_EGENERAL;
}
