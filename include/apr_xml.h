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
/**
 * @file apr_xml.h
 * @brief APR-UTIL XML Library
 */
#ifndef APR_XML_H
#define APR_XML_H

/**
 * @defgroup APR_Util_XML XML 
 * @ingroup APR_Util
 * @{
 */
#include "apr_pools.h"
#include "apr_tables.h"
#include "apr_file_io.h"

#include "apu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @package Apache XML library
 */

/* -------------------------------------------------------------------- */

/* ### these will need to move at some point to a more logical spot */

/* simple strutures to keep a linked list of pieces of text */
typedef struct apr_text apr_text;

/** Structure to keep a linked list of pieces of text */
struct apr_text {
    /** The current piece of text */
    const char *text;
    /** a pointer to the next piece of text 
     *  @defvar apr_text *next */
    struct apr_text *next;
};

typedef struct apr_text_header apr_text_header;

/** A list of pieces of text */
struct apr_text_header {
    /** The first piece of text in the list */
    apr_text *first;
    /** The last piece of text in the list */
    apr_text *last;
};

/**
 * Append a piece of text to the end of a list
 * @param p The pool to allocate out of
 * @param hdr The text header to append to
 * @param text The new text to append
 */
APU_DECLARE(void) apr_text_append(apr_pool_t *p, apr_text_header *hdr,
                                  const char *text);


/* --------------------------------------------------------------------
**
** XML PARSING
*/

/*
** Qualified namespace values
**
** APR_XML_NS_DAV_ID
**    We always insert the "DAV:" namespace URI at the head of the
**    namespace array. This means that it will always be at ID==0,
**    making it much easier to test for.
**
** APR_XML_NS_NONE
**    This special ID is used for two situations:
**
**    1) The namespace prefix begins with "xml" (and we do not know
**       what it means). Namespace prefixes with "xml" (any case) as
**       their first three characters are reserved by the XML Namespaces
**       specification for future use. mod_dav will pass these through
**       unchanged. When this identifier is used, the prefix is LEFT in
**       the element/attribute name. Downstream processing should not
**       prepend another prefix.
**
**    2) The element/attribute does not have a namespace.
**
**       a) No prefix was used, and a default namespace has not been
**          defined.
**       b) No prefix was used, and the default namespace was specified
**          to mean "no namespace". This is done with a namespace
**          declaration of:  xmlns=""
**          (this declaration is typically used to override a previous
**          specification for the default namespace)
**
**       In these cases, we need to record that the elem/attr has no
**       namespace so that we will not attempt to prepend a prefix.
**       All namespaces that are used will have a prefix assigned to
**       them -- mod_dav will never set or use the default namespace
**       when generating XML. This means that "no prefix" will always
**       mean "no namespace".
**
**    In both cases, the XML generation will avoid prepending a prefix.
**    For the first case, this means the original prefix/name will be
**    inserted into the output stream. For the latter case, it means
**    the name will have no prefix, and since we never define a default
**    namespace, this means it will have no namespace.
**
** Note: currently, mod_dav understands the "xmlns" prefix and the
**     "xml:lang" attribute. These are handled specially (they aren't
**     left within the XML tree), so the APR_XML_NS_NONE value won't ever
**     really apply to these values.
*/
#define APR_XML_NS_DAV_ID	0	/* namespace ID for "DAV:" */
#define APR_XML_NS_NONE		-10	/* no namespace for this elem/attr */

#define APR_XML_NS_ERROR_BASE	-100	/* used only during processing */
#define APR_XML_NS_IS_ERROR(e)	((e) <= APR_XML_NS_ERROR_BASE)


typedef struct apr_xml_attr apr_xml_attr;
typedef struct apr_xml_elem apr_xml_elem;
typedef struct apr_xml_doc apr_xml_doc;

/** apr_xml_attr: holds a parsed XML attribute */
struct apr_xml_attr {
    /** attribute name */
    const char *name;
    /** index into namespace array */
    int ns;

    /** attribute value */
    const char *value;

    /** next attribute
     *  @defvar apr_xml_attr *next */
    struct apr_xml_attr *next;
};

/** apr_xml_elem: holds a parsed XML element */
struct apr_xml_elem {
    /** element name */
    const char *name;
    /** index into namespace array */
    int ns;
    /** xml:lang for attrs/contents */
    const char *lang;

    /** cdata right after start tag */
    apr_text_header first_cdata;
    /** cdata after MY end tag */
    apr_text_header following_cdata;

    /** parent element 
     *  @defvar apr_xml_elem *parent */
    struct apr_xml_elem *parent;	
    /** next (sibling) element 
     *  @defvar apr_xml_elem *next */
    struct apr_xml_elem *next;	
    /** first child element 
     *  @defvar apr_xml_elem *first_child */
    struct apr_xml_elem *first_child;
    /** first attribute 
     *  @defvar apr_xml_attr *attr */
    struct apr_xml_attr *attr;		

    /* used only during parsing */
    /** last child element 
     *  @defvar apr_xml_elem *last_child */
    struct apr_xml_elem *last_child;
    /** namespaces scoped by this elem 
     *  @defvar apr_xml_ns_scope *ns_scope */
    struct apr_xml_ns_scope *ns_scope;

    /* used by modules during request processing */
    /** Place for modules to store private data */
    void *priv;
};

#define APR_XML_ELEM_IS_EMPTY(e) ((e)->first_child == NULL && \
                                  (e)->first_cdata.first == NULL)

/** apr_xml_doc: holds a parsed XML document */
struct apr_xml_doc {
    /** root element */
    apr_xml_elem *root;	
    /** array of namespaces used */
    apr_array_header_t *namespaces;
};


typedef struct apr_xml_parser apr_xml_parser;

/**
 * Create an XML parser
 * @param pool The pool for allocating the parser and the parse results.
 * @return The new parser.
 */
APU_DECLARE(apr_xml_parser *) apr_xml_parser_create(apr_pool_t *pool);

/**
 * Parse a File, producing a xml_doc
 * @param p      The pool for allocating the parse results.
 * @param parser A pointer to *parser (needed so calling function can get
 *               errors), will be set to NULL on successfull completion.
 * @param ppdoc  A pointer to *apr_xml_doc (which has the parsed results in it)
 * @param xmlfd  A file to read from.
 * @param buffer_length Buffer length which would be suitable 
 * @return Any errors found during parsing.
 */
APU_DECLARE(apr_status_t) apr_xml_parse_file(apr_pool_t *p,
                                             apr_xml_parser **parser,
                                             apr_xml_doc **ppdoc,
                                             apr_file_t *xmlfd,
                                             apr_size_t buffer_length);


/**
 * Feed input into the parser
 * @param parser The XML parser for parsing this data.
 * @param data The data to parse.
 * @param len The length of the data.
 * @return Any errors found during parsing.
 * @remark Use apr_xml_parser_geterror() to get more error information.
 */
APU_DECLARE(apr_status_t) apr_xml_parser_feed(apr_xml_parser *parser,
                                              const char *data,
                                              apr_size_t len);

/**
 * Terminate the parsing and return the result
 * @param parser The XML parser for parsing this data.
 * @param pdoc The resulting parse information. May be NULL to simply
 *             terminate the parsing without fetching the info.
 * @return Any errors found during the final stage of parsing.
 * @remark Use apr_xml_parser_geterror() to get more error information.
 */
APU_DECLARE(apr_status_t) apr_xml_parser_done(apr_xml_parser *parser,
                                              apr_xml_doc **pdoc);

/**
 * Fetch additional error information from the parser.
 * @param parser The XML parser to query for errors.
 * @param errbuf A buffer for storing error text.
 * @param errbufsize The length of the error text buffer.
 * @return The error buffer
 */
APU_DECLARE(char *) apr_xml_parser_geterror(apr_xml_parser *parser,
                                            char *errbuf,
                                            apr_size_t errbufsize);


/**
 * Converts an XML element tree to flat text 
 * @param p The pool to allocate out of
 * @param elem The XML element to convert
 * @param style How to covert the XML.  One of:
 * <PRE>
 *     APR_XML_X2T_FULL                start tag, contents, end tag 
 *     APR_XML_X2T_INNER               contents only 
 *     APR_XML_X2T_LANG_INNER          xml:lang + inner contents 
 *     APR_XML_X2T_FULL_NS_LANG        FULL + ns defns + xml:lang 
 * </PRE>
 * @param namespaces The namespace of the current XML element
 * @param ns_map Namespace mapping
 * @param pbuf Buffer to put the converted text into
 * @param psize Size of the converted text
 */
APU_DECLARE(void) apr_xml_to_text(apr_pool_t *p, const apr_xml_elem *elem,
                                  int style, apr_array_header_t *namespaces,
                                  int *ns_map, const char **pbuf,
                                  apr_size_t *psize);

/* style argument values: */
#define APR_XML_X2T_FULL         0	/**< start tag, contents, end tag */
#define APR_XML_X2T_INNER        1	/**< contents only */
#define APR_XML_X2T_LANG_INNER   2	/**< xml:lang + inner contents */
#define APR_XML_X2T_FULL_NS_LANG 3	/**< FULL + ns defns + xml:lang */

/**
 * empty XML element
 * @param p The pool to allocate out of
 * @param elem The XML element to empty
 * @return the string that was stored in the XML element
 */
APU_DECLARE(const char *) apr_xml_empty_elem(apr_pool_t *p,
                                             const apr_xml_elem *elem);

/**
 * quote an XML string
 * Replace '<', '>', and '&' with '&lt;', '&gt;', and '&amp;'.
 * @param p The pool to allocate out of
 * @param s The string to quote
 * @param quotes If quotes is true, then replace '"' with '&quot;'.
 * @return The quoted string
 */
APU_DECLARE(const char *) apr_xml_quote_string(apr_pool_t *p, const char *s,
                                               int quotes);

/**
 * Quote an XML element
 * @param p The pool to allocate out of
 * @param elem The element to quote
 */
APU_DECLARE(void) apr_xml_quote_elem(apr_pool_t *p, apr_xml_elem *elem);

/* manage an array of unique URIs: apr_xml_insert_uri() and APR_XML_URI_ITEM() */

/**
 * return the URI's (existing) index, or insert it and return a new index 
 * @param uri_array array to insert into
 * @param uri The uri to insert
 * @return int The uri's index
 */
APU_DECLARE(int) apr_xml_insert_uri(apr_array_header_t *uri_array,
                                    const char *uri);
#define APR_XML_GET_URI_ITEM(ary, i) (((const char * const *)(ary)->elts)[i])

#ifdef __cplusplus
}
#endif
/** @} */
#endif /* APR_XML_H */
