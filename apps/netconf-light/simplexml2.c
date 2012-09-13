/* $Id: simplexml.c,v 1.1.1.1 2002/08/23 10:38:57 essmann Exp $
 *
 * Copyright (c) 2001-2002 Bruno Essmann <essmann@users.sourceforge.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

//DY : FIX_ME kirpabilecegin kisimlari cikar yavru mesela nInputLineNumber bi isine yaramaz sanki eger validation yapmayacaksan, belki en azindan gereksiz fonksiyonlar olabilir kirpilacak.
//simpleXmlPopUserData ve simpleXmlPushUserData fonksiyonlari ile kullandiklari stack data structure lib icinde kullanilmiyo, sadece group.c orneginde var, amaci baska fonksiyonlarla baska tagleri parse edebilmek sanirim.

/* ---- includes */
#include <stdlib.h>
#include <string.h>
#include <stdio.h> //needed for xml generation sprintfs
#include "simplexml2.h"
#include "contiki.h"//added by VP
#include "cfs-coffee.h"//added by VP
#include <avr/pgmspace.h>
#include "raven-lcd.h"
#include <avr/io.h>

#if CONTIKI_TARGET_AVR_RAVEN
const char outputfile[] PROGMEM = "/output.xml";
const char *outfile[] PROGMEM = {
		outputfile
};
#endif
/* ---- definitions */

/* result code for various functions indicating a failure */
#define FAIL 0
/* result code for various functions indicating a success */
#define SUCCESS 1

/* error code value indicating that everything is ok */
#define NO_ERROR 0
/* error code value indicating that parsing hasn't started yet */
#define NOT_PARSED 1
/* error code value indicating that the parser couldn't
 * grow one of its internal structures using malloc */
#define OUT_OF_MEMORY 2
/* error code value indicating that the XML data was early
 * terminated, i.e. the parser was expecting more data */
#define EARLY_TERMINATION 3
/* error code value indicating that the ampersand character
 * was used in an illegal context */
#define ILLEGAL_AMPERSAND 4
/* error code indicating that a unicode character was read,
 * this parser does not support unicode characters (only
 * 8-bit values) */
#define NO_UNICODE_SUPPORT 5
/* indicates that the parser expected a greater than (>)
   character but read something else */
#define GREATER_THAN_EXPECTED 6
/* indicates that the parser expected a quote (' or ")
   character but read something else */
#define QUOTE_EXPECTED 7
/* indicates that an illegal handler was passed to the parser */
#define ILLEGAL_HANDLER 8
/* indicates that the parser was reused without proper initialization */
#define NOT_INITIALIZED 9
/* indicates that the document doesn't have a single tag */
#define NO_DOCUMENT_TAG 10
/* indicates that the parser encountered an illegal end tag */
#define MISMATCHED_END_TAG 11
/* indicates that the parser was expecting an attribute but
   instead read some other token type */
#define ATTRIBUTE_EXPECTED 12
/* indicates that the parser was expecting an equal sign (=)
   but some other character was read */
#define EQUAL_SIGN_EXPECTED 13

//DY
/* There can only be maximum of MAX_NUMBER_OF_XML_ATTRS */
#define MAX_NO_OF_ATTRS_EXCEEDED 14

/* next token type describing the opening of a begin tag */
#define TAG_BEGIN_OPENING 0
/* next token type describing the closing of a begin tag */
#define TAG_BEGIN_CLOSING 1
/* next token type describing an end tag */
#define TAG_END 2
/* next token type describing a processing instruction <?...?> */
#define PROCESSING_INSTRUCTION 3
/* next token type describing a document type <!DOCTYPE...> */
#define DOCTYPE 4
/* next token type describing a comment <!--...--> */
#define COMMENT 5
/* next token type describing an attribute */
#define ATTRIBUTE 6
/* next token type describing tag content */
#define CONTENT 7
/* next token type describing an unknown <!xxx> tag */
#define UNKNOWN 8

/* the space constants */
#define SPACE ' '
/* the linefeed constant */
#define LF '\xa'
/* the carriage return constant */
#define CR '\xd'

//DY
#define MAX_NUMBER_OF_XML_ATTRS 5

/* ---- types */

///**
// * Value buffer.
// * This structure resembles a string buffer that
// * grows automatically when inserting data.
// */
//typedef struct simplexml_value_buffer {
//  /* buffer data */
//  char* sBuffer;
//  /* size of the buffer */
//  long nSize;
//  /* insert position in buffer */
//  long nPosition;
//} TSimpleXmlValueBuffer, *SimpleXmlValueBuffer;


//DY : deleted for reducing code size
//**********************************DELETED*********************************
/**
 * User data stack.
 * This structure is a simple stack for user data.
 */
//typedef struct simplexml_user_data {
//  void* pData;
//  struct simplexml_user_data* next;
//} TSimpleXmlUserData, *SimpleXmlUserData;

/**
 * SimpleXmlParser internal state.
 *
 * This structure holds all data necessary for the
 * simple xml parser to operate.
 *
 * This struct describes the internal representation
 * of a SimpleXmlParserState.
 */
typedef struct simplexml_parser_state {
  /* error code if != NO_ERROR */
  int nError;
  /* value of the next token */
  SimpleXmlValueBuffer vbNextToken;
  /* next token type */
  int nNextToken;
  /* the name of an attribute read */
  char *szAttribute;
  /* allocated buffer size for attribute name */
  int nAttributeBufferSize;
  /* input data buffer */
  char *sInputData;
  /* length of the input data buffer */
  long nInputDataSize;
  /* read cursor on input data */
  long nInputDataPos;
  /* line number at cursor position */
  long nInputLineNumber;
  /* the user data pointer */
  //SimpleXmlUserData pUserData;
  void* pUserDefinedData;
} TSimpleXmlParserState, *SimpleXmlParserState;

/* ---- prototypes */

SimpleXmlParserState createSimpleXmlParser (const char *sData, long nDataSize);
void destroySimpleXmlParser (SimpleXmlParserState parser);
int initializeSimpleXmlParser (SimpleXmlParserState parser, const char *sData, long nDataSize);
char* getSimpleXmlParseErrorDescription (SimpleXmlParserState parser);
int parseSimpleXml (SimpleXmlParserState parser, SimpleXmlTagHandler handler);
int parseOneTag (SimpleXmlParserState parser, SimpleXmlTagHandler parentHandler);
int readNextTagToken (SimpleXmlParserState parser);
int readNextContentToken (SimpleXmlParserState parser);
int readChar (SimpleXmlParserState parser);
char peekInputCharAt (SimpleXmlParserState parser, int nOffset);
char peekInputChar (SimpleXmlParserState parser);
int skipWhitespace (SimpleXmlParserState parser);
void skipInputChars (SimpleXmlParserState parser, int nAmount);
void skipInputChar (SimpleXmlParserState parser);
char readInputChar (SimpleXmlParserState parser);
int addNextTokenCharValue (SimpleXmlParserState parser, char c);
int addNextTokenStringValue (SimpleXmlParserState parser, char *szInput);

SimpleXmlValueBuffer createSimpleXmlValueBuffer (long nInitialSize);
void destroySimpleXmlValueBuffer (SimpleXmlValueBuffer vb);
int growSimpleXmlValueBuffer (SimpleXmlValueBuffer vb);
int appendCharToSimpleXmlValueBuffer (SimpleXmlValueBuffer vb, char c);
int appendStringToSimpleXmlValueBuffer (SimpleXmlValueBuffer vb, const char *szInput);
int zeroTerminateSimpleXmlValueBuffer (SimpleXmlValueBuffer vb);
int clearSimpleXmlValueBuffer (SimpleXmlValueBuffer vb);
int getSimpleXmlValueBufferContentLength (SimpleXmlValueBuffer vb);
int getSimpleXmlValueBufferContents (SimpleXmlValueBuffer vb, char* szOutput, long nMaxLen);
char* getInternalSimpleXmlValueBufferContents (SimpleXmlValueBuffer vb);

/* ---- public api */
char *strdup(const char *str)
{
    int n = strlen(str) + 1;
    char *dup = malloc(n);
    if(dup)
    {
        strcpy(dup, str);
    }
    return dup;
}
SimpleXmlParser simpleXmlCreateParser (const char *sData, long nDataSize) {
  return (SimpleXmlParser) createSimpleXmlParser(sData, nDataSize);
}

void simpleXmlDestroyParser (SimpleXmlParser parser) {
  destroySimpleXmlParser((SimpleXmlParserState) parser);
}

int simpleXmlInitializeParser (SimpleXmlParser parser, const char *sData, long nDataSize) {
  return initializeSimpleXmlParser((SimpleXmlParserState) parser, sData, nDataSize);
}

int simpleXmlParse (SimpleXmlParser parser, SimpleXmlTagHandler handler) {
  if (parseSimpleXml((SimpleXmlParserState) parser, handler) == FAIL) {
    return ((SimpleXmlParserState) parser)->nError;
  }
  return 0;
}

char* simpleXmlGetErrorDescription (SimpleXmlParser parser) {
  return getSimpleXmlParseErrorDescription((SimpleXmlParserState) parser);
}

long simpleXmlGetLineNumber (SimpleXmlParser parser) {
  if (parser == NULL) {
    return -1;
  }
  return ((SimpleXmlParserState) parser)->nInputLineNumber + 1;
}

void simpleXmlParseAbort (SimpleXmlParser parser, int nErrorCode) {
  if (parser == NULL || nErrorCode < SIMPLE_XML_USER_ERROR) {
    return;
  }
  ((SimpleXmlParserState) parser)->nError= nErrorCode;
}

//DY : deleted for reducing code size
//**********************************DELETED*********************************
/*
int simpleXmlPushUserData (SimpleXmlParser parser, void* pData) {
  SimpleXmlUserData newUserData;
  if (parser == NULL || pData == NULL) {
    return 0;
  }
  newUserData= malloc(sizeof(TSimpleXmlUserData));
  if (newUserData == NULL) {
    return 0;
  }
  newUserData->pData= pData;
  if (((SimpleXmlParserState) parser)->pUserData == NULL) {
    newUserData->next= NULL;
  } else {
    newUserData->next= ((SimpleXmlParserState) parser)->pUserData;
  }
  ((SimpleXmlParserState) parser)->pUserData= newUserData;
  return 1;
}

void* simpleXmlPopUserData (SimpleXmlParser parser) {
  if (parser == NULL) {
    return NULL;
  }
  if (((SimpleXmlParserState) parser)->pUserData == NULL) {
    return NULL;
  } else {
    void* pData;
    SimpleXmlUserData ud= ((SimpleXmlParserState) parser)->pUserData;
    ((SimpleXmlParserState) parser)->pUserData= ud->next;
    pData= ud->pData;
    free(ud);
    return pData;
  }
}

void* simpleXmlGetUserDataAt (SimpleXmlParser parser, int nLevel) {
  if (parser == NULL) {
    return NULL;
  } else {
    SimpleXmlUserData ud= ((SimpleXmlParserState) parser)->pUserData;
    while (ud != NULL && nLevel > 0) {
      ud= ud->next;
      nLevel--;
    }
    if (ud != NULL && nLevel == 0) {
      return ud->pData;
    }
  }
  return NULL;
}

void* simpleXmlGetUserData (SimpleXmlParser parser) {
  return simpleXmlGetUserDataAt(parser, 0);
}
*/

/* ---- xml parser */

/**
 * No operation handler (used internally).
 */
void* simpleXmlNopHandler (SimpleXmlParser parser, SimpleXmlEvent event,
  const char* szName, const char* szAttribute, const char* szValue) {
  /* simply return the nop handler again */
  return simpleXmlNopHandler;
}

/**
 * Creates a new simple xml parser for the specified input data.
 *
 * @param sData the input data to parse (must no be NULL).
 * @param nDataSize the size of the input data buffer (sData)
 * to parse (must be greater than 0).
 * @return the new simple xml parser or NULL if there
 * is not enough memory or the input data specified cannot
 * be parsed.
 */

SimpleXmlParserState createSimpleXmlParser (const char *sData, long nDataSize) {
  if (sData != NULL && nDataSize > 0) {
    SimpleXmlParserState parser= malloc(sizeof(TSimpleXmlParserState));

    if (parser == NULL) {
      return NULL;
    }
    parser->nError= NOT_PARSED;
    parser->vbNextToken= createSimpleXmlValueBuffer(100);
    if (parser->vbNextToken == NULL) {
      free(parser);
      return NULL;
    }
    parser->szAttribute= NULL;
    parser->nAttributeBufferSize= 0;
    parser->sInputData= (char*) sData;
    parser->nInputDataSize= nDataSize;
    parser->nInputDataPos= 0;
    parser->nInputLineNumber= 0;

    //parser->pUserData= NULL;
    parser->pUserDefinedData = NULL;
    return parser;
  }
  return NULL;
}

 //DY
 #include "../../core/lib/list.h"
LIST(namespaces);
struct namespace
{
  struct namespace* next;
  char name[20];
  char value[70];
};
typedef struct namespace namespace;

/**
 * Destroys the specified simple xml parser.
 *
 * @param parser the parser to destroy (must have been
 * created using createSimpleXmlParser).
 */
void destroySimpleXmlParser (SimpleXmlParserState parser) {
  if (parser != NULL)
  {
    if (parser->vbNextToken != NULL)
    {
      //DY : there was memory leak here, just calling free does not deallocate the buffer inside.
      //free(parser->vbNextToken);
      destroySimpleXmlValueBuffer(parser->vbNextToken);
    }
    if (parser->szAttribute != NULL)
    {
      free(parser->szAttribute);
    }

    //DY : free namespaces
    namespace* ns = list_head(namespaces);
    namespace* next = NULL;
    while(ns)
    {
      next = ns->next;
      free(ns);
      ns = next;
    }
    list_init(namespaces);

    //DY : deleted for reducing code size
    //**********************************DELETED*********************************
    /*{
      SimpleXmlUserData ud= parser->pUserData;
      while (ud != NULL) {
        SimpleXmlUserData next= ud->next;
        free(ud);
        ud= next;
      }
    }*/

    free(parser);
  }
}

/**
 * Reinitializes the specified simple xml parser for
 * parsing the specified input data.
 *
 * @param parser the parser to initialize.
 * @param sData the input data to parse (must no be NULL).
 * @param nDataSize the size of the input data buffer (sData)
 * to parse (must be greater than 0).
 * @return 0 if the parser could not be initialized,
 * > 0 if the parser was initialized successfully.
 */
int initializeSimpleXmlParser (SimpleXmlParserState parser, const char *sData, long nDataSize) {
  if (parser != NULL && sData != NULL && nDataSize > 0) {
    if (parser->vbNextToken == NULL) {
      return FAIL;
    }
    parser->nError= NOT_PARSED;
    clearSimpleXmlValueBuffer(parser->vbNextToken);
    parser->sInputData= (char*) sData;
    parser->nInputDataSize= nDataSize;
    parser->nInputDataPos= 0;
    parser->nInputLineNumber= 0;

    //parser->pUserData= NULL;
    parser->pUserDefinedData = NULL;

    return SUCCESS;
  }
  return FAIL;
}

//DY : FIX_ME : can be put in ifdef to minimize code size
/**
 * Returns a description of the error that occured
 * during parsing.
 *
 * @param parser the parser for which to get the error
 * description.
 * @return an error description or NULL if there was
 * no error during parsing.
 */
char* getSimpleXmlParseErrorDescription (SimpleXmlParserState parser) {
  if (parser == NULL) {
    return NULL;
  }
  switch (parser->nError) {
    case NO_ERROR: return NULL;
    case NOT_PARSED: return "e1";//"parsing has not yet started";
    case OUT_OF_MEMORY: return "e2";//"out of memory";
    case EARLY_TERMINATION: return "e3";//"unexpected end of xml data";
    case ILLEGAL_AMPERSAND: return "e4";//"illegal use of ampersand (&)";
    case NO_UNICODE_SUPPORT: return "e5";//"unicode characters are not supported";
    case GREATER_THAN_EXPECTED: return "e6";//"greater than sign (>) expected";
    case QUOTE_EXPECTED: return "e7";//"quote (either \' or \") expected";
    case ILLEGAL_HANDLER: return "e8";//"illegal xml handler specified";
    case NOT_INITIALIZED: return "e9";//"xml parser not initialized";
    case NO_DOCUMENT_TAG: return "e10";//"no document tag found";
    case MISMATCHED_END_TAG: return "e11";//"mismatched end tag";
    case ATTRIBUTE_EXPECTED: return "e12";//"attribute expected";
    case EQUAL_SIGN_EXPECTED: return "e13";//"equal sign (=) expected";
  }
  if (parser->nError > SIMPLE_XML_USER_ERROR) {
    return "parsing aborted";
  }
  return "unknown error";
}

/**
 * Starts an initialized (or newly created) xml parser with the
 * specified document tag handler.
 *
 * @param parser the parser to start.
 * @param handler the handler to use for the document tag.
 * @return 0 if there was no error, and error code
 * > 0 if there was an error.
 */

int parseSimpleXml (SimpleXmlParserState parser, SimpleXmlTagHandler handler) {
  if (parser == NULL || handler == NULL) {
    parser->nError= ILLEGAL_HANDLER;
    return FAIL;
  }
  /* check if the parser was initialized properly */
  if (parser->nError != NOT_PARSED) {
    parser->nError= NOT_INITIALIZED;
    return FAIL;
  }

  /* reset error code */
  parser->nError= NO_ERROR;

  /* read xml prolog and dtd */
  do {
    skipWhitespace(parser);
    clearSimpleXmlValueBuffer(parser->vbNextToken);
    if (readNextContentToken(parser) == FAIL) {
      if (parser->nError == EARLY_TERMINATION) {
        /* read all data and didn't find any tag */
        parser->nError= NO_DOCUMENT_TAG;
      }
      return FAIL;
    }
  } while (
    parser->nNextToken == PROCESSING_INSTRUCTION ||
    parser->nNextToken == COMMENT ||
    parser->nNextToken == DOCTYPE
  );

  /* parse main xml tag */
  if (parser->nNextToken == TAG_BEGIN_OPENING) {

    //DY
    list_init(namespaces);
    return parseOneTag(parser, handler);
  }

  /* no document tag found */
  parser->nError= NO_DOCUMENT_TAG;
  return FAIL;
}

//DY : FIX_ME : hersey aq hersey, bool olcak bi kere
int addNamespace(const char* name, const char* value)
{
  int bReturnValue = 0;

  namespace* ns = (namespace*)malloc(sizeof(namespace));
  if ( ns )
  {
    strcpy(ns->name, name);
    strcpy(ns->value, value);
    list_add(namespaces, ns);
    bReturnValue = 1;
  }

  return bReturnValue;
}

const char* findNamespaceValue(const char* attribute, unsigned short sizeOfNS)
{
  namespace* ns = list_head(namespaces);

  for(; ns != NULL;
      ns = ns->next)
  {
    if ( !strncmp(ns->name, attribute, sizeOfNS) )
    {
      return ns->value;
    }
  }

  return NULL;
}

void freeAttributes(char** attr)
{
  unsigned short nIndex = 0;
  for ( nIndex=0; attr[nIndex] ;nIndex+=3 )
  {
    //dont free attr[index] since it points to namespace which will be deleted later
    free(attr[nIndex+1]);
    free(attr[nIndex+2]);
  }
}

//DY : FIX_ME : handler fonksiyonlar boolean dondurebilir, ve false donderirse parse etmeyi durdurursun mesela, user defined error diye
/**
 * Parses exactly one tag.
 */
int parseOneTag (SimpleXmlParserState parser, SimpleXmlTagHandler parentHandler) {
  //SimpleXmlTagHandler handler;
  char* szTagName;
  char* attr[MAX_NUMBER_OF_XML_ATTRS * 3];
  char* content = NULL;

  /*initialize the attr array*/
  attr[0] = NULL;

  if (getInternalSimpleXmlValueBufferContents(parser->vbNextToken) == NULL) {
    parser->nError= OUT_OF_MEMORY;
    return FAIL;
  }
  szTagName= strdup(getInternalSimpleXmlValueBufferContents(parser->vbNextToken));
  if (szTagName == NULL) {
    parser->nError= OUT_OF_MEMORY;
    return FAIL;
  }
  clearSimpleXmlValueBuffer(parser->vbNextToken);
//  handler= parentHandler((SimpleXmlParser) parser, ADD_SUBTAG, szTagName, NULL, NULL);

//  if (parser->nError != NO_ERROR) {
//    return FAIL;
//  }

//  if (handler == NULL) {
//    handler= simpleXmlNopHandler;
//  }

  if (readNextTagToken(parser) == FAIL) {
    free(szTagName);
    return FAIL;
  }
  unsigned char nAttrIndex = 0;

  while (
    parser->nNextToken != TAG_END &&
    parser->nNextToken != TAG_BEGIN_CLOSING
  ) {

    /* read attributes */
    if (parser->nNextToken == ATTRIBUTE)
    {
      //DY
      //printf("Attr Index %d %s\n", nAttrIndex, szTagName);
      if ( nAttrIndex == MAX_NUMBER_OF_XML_ATTRS - 1 )
      {
        parser->nError = MAX_NO_OF_ATTRS_EXCEEDED;
        free(szTagName);
        return FAIL;
      }
      if (getInternalSimpleXmlValueBufferContents(parser->vbNextToken) == NULL) {
        parser->nError= OUT_OF_MEMORY;
        free(szTagName);
        return FAIL;
      }

//      handler((SimpleXmlParser) parser, ADD_ATTRIBUTE, szTagName, parser->szAttribute,
//        getInternalSimpleXmlValueBufferContents(parser->vbNextToken));

      //DY : FIX_ME : free these
      char* delimiter = strchr(parser->szAttribute,':');
      char* ns = NULL;
      if (delimiter)
      {
        unsigned short sizeOfNS = delimiter-(parser->szAttribute);
        if (!strncmp("xmlns",parser->szAttribute,sizeOfNS))
        {
          ++delimiter;
          printf("NAMESPACE FOUND : %s\n",delimiter);
          addNamespace(delimiter,getInternalSimpleXmlValueBufferContents(parser->vbNextToken));
        }
        else
        {
          ns = (char *)findNamespaceValue(parser->szAttribute,sizeOfNS);
        }
      }

      if ( ns )
      {
        attr[nAttrIndex++] = ns;
        attr[nAttrIndex++] = strdup(delimiter);
      }
      else
      {
        attr[nAttrIndex++] = "";
        attr[nAttrIndex++] = strdup(parser->szAttribute);
      }

      attr[nAttrIndex++] = strdup(getInternalSimpleXmlValueBufferContents(parser->vbNextToken));

      if (parser->nError != NO_ERROR) {
        free(szTagName);
        return FAIL;
      }
      clearSimpleXmlValueBuffer(parser->vbNextToken);
    } else {
      /* attribute expected */
      parser->nError= ATTRIBUTE_EXPECTED;
      free(szTagName);
      return FAIL;
    }
    if (readNextTagToken(parser) == FAIL) {
      free(szTagName);
      return FAIL;
    }
  }
  //DY
  attr[nAttrIndex] = NULL;

  char* uri = NULL;

  char* tagDelimiter = strchr(szTagName, ':');

  if ( tagDelimiter ){
    uri=(char*)findNamespaceValue(szTagName,tagDelimiter-szTagName);
	printf("%s\n",uri);
    tagDelimiter++;
    parentHandler((SimpleXmlParser) parser, ADD_SUBTAG, uri, tagDelimiter, (const char**)attr);
  }
  else
  {
    parentHandler((SimpleXmlParser) parser, ADD_SUBTAG, "", szTagName, (const char**)attr);
  }

  //DY : FIX_ME : also call this in error situations
  freeAttributes(attr);

//  handler((SimpleXmlParser) parser, FINISH_ATTRIBUTES, szTagName, NULL, NULL);
//  parentHandler((SimpleXmlParser) parser, FINISH_ATTRIBUTES, szTagName, NULL, NULL);
//  parentHandler((SimpleXmlParser) parser, ADD_SUBTAG, uri, szTagName, (char**)attr);

//  if (handler == NULL)
//  {
//    handler= simpleXmlNopHandler;
//  }

  if (parser->nError != NO_ERROR) {
    free(szTagName);
    return FAIL;
  }

  if (parser->nNextToken == TAG_BEGIN_CLOSING) {
    if (readNextContentToken(parser) == FAIL) {
      free(szTagName);
      return FAIL;
    }
    while (parser->nNextToken != TAG_END) {
      /* read content */
      if (parser->nNextToken == TAG_BEGIN_OPENING) {
        /* read subtag -> recurse */

//        if (parseOneTag(parser, handler) == FAIL) {
        if (parseOneTag(parser, parentHandler) == FAIL) {
          free(szTagName);
          return FAIL;
        }
      } else if (parser->nNextToken == CONTENT) {
        /* read content value */
        if (getInternalSimpleXmlValueBufferContents(parser->vbNextToken) == NULL) {
          parser->nError= OUT_OF_MEMORY;
          free(szTagName);
          return FAIL;
        }

//        handler((SimpleXmlParser) parser, ADD_CONTENT, szTagName, NULL,
//          getInternalSimpleXmlValueBufferContents(parser->vbNextToken));
        content = getInternalSimpleXmlValueBufferContents(parser->vbNextToken);
        parentHandler((SimpleXmlParser) parser, ADD_CONTENT, "", content, NULL);

        if (parser->nError != NO_ERROR) {
          free(szTagName);
          return FAIL;
        }
        clearSimpleXmlValueBuffer(parser->vbNextToken);
      } else if (parser->nNextToken == COMMENT) {
        /* ignore comment for the moment (maybe we should call the handler) */
      }
      /* discard current token value */
      clearSimpleXmlValueBuffer(parser->vbNextToken);
      /* get the next token */
      if (readNextContentToken(parser) == FAIL) {
        free(szTagName);
        return FAIL;
      }
    }
    /* check the name of the closing tag */
    if (getInternalSimpleXmlValueBufferContents(parser->vbNextToken) == NULL) {
      parser->nError= OUT_OF_MEMORY;
      free(szTagName);
      return FAIL;
    }
    if (
      strcmp(
        szTagName, getInternalSimpleXmlValueBufferContents(parser->vbNextToken)
      ) != 0
    ) {
      parser->nError= MISMATCHED_END_TAG;
      free(szTagName);
      return FAIL;
    }
  }

  /* flush closing tag token data */
  clearSimpleXmlValueBuffer(parser->vbNextToken);

  //DY : FIX_ME : buna da namespace cakacan aslinda
  //handler((SimpleXmlParser) parser, FINISH_TAG, szTagName, NULL, NULL);
  //parentHandler((SimpleXmlParser) parser, FINISH_TAG, "", szTagName, NULL);

  uri = NULL;
  tagDelimiter = strchr(szTagName, ':');
  if ( tagDelimiter )
  {
    uri=(char*)findNamespaceValue(szTagName,tagDelimiter-szTagName);
    tagDelimiter++;
    parentHandler((SimpleXmlParser) parser, FINISH_TAG, uri, tagDelimiter, NULL);
  }
  else
  {
    parentHandler((SimpleXmlParser) parser, FINISH_TAG, "", szTagName, NULL);
  }

  if (parser->nError != NO_ERROR) {
    free(szTagName);
    return FAIL;
  }

  free(szTagName);
  return SUCCESS;
}

/**
 * Scanner that reads the contents of a tag and
 * sets the nNextToken type and the value buffer of
 * the parser. Must not be invoked unless the last
 * token read was a TAG_BEGIN (see readNextContentToken
 * in such a case).
 *
 * The following token types are supported:
 *
 * Type                   | ValueBuffer | Example
 * -----------------------+-------------+-------------
 * TAG_END                | <unchanged> | />
 * TAG_BEGIN_CLOSING      | <unchanged> | >
 * ATTRIBUTE              | bar         | foo="bar"
 *
 * Note: The name of an attribute (e.g. foo in the above
 * example) is stored in the attribute name field of the
 * parser (szAttributeName).
 *
 * @param parser the parser for which to read the next token.
 * @return SUCCESS or FAIL.
 */
int readNextTagToken (SimpleXmlParserState parser) {
  /* read tag closing or attribute */
  if (peekInputChar(parser) == '/') {
    /* read tag closing (combined end tag) */
    skipInputChar(parser);
    if (peekInputChar(parser) == '>') {
      parser->nNextToken= TAG_END;
      skipInputChar(parser);
    } else {
      parser->nError= GREATER_THAN_EXPECTED;
      return FAIL;
    }
  } else if (peekInputChar(parser) == '>') {
    /* read tag closing */
    parser->nNextToken= TAG_BEGIN_CLOSING;
    skipInputChar(parser);
  } else {
    /* read attribute */
    char cQuote; /* the quote type used (either ' or " -> a='b' or a="b") */
    parser->nNextToken= ATTRIBUTE;
    while (peekInputChar(parser) != '=' && peekInputChar(parser) > SPACE) {
      /* read one character into next token value buffer */
      if (readChar(parser) == FAIL) {
        return FAIL;
      }
    }
    /* skip whitespace */
    if (skipWhitespace(parser) == FAIL) {
      return FAIL;
    }
    if (peekInputChar(parser) != '=') {
      parser->nError= EQUAL_SIGN_EXPECTED;
      return FAIL;
    }
    /* skip '=' */
    skipInputChar(parser);
    /* copy contents of value buffer to attribute name */
    if (
      parser->szAttribute == NULL ||
      parser->nAttributeBufferSize < getSimpleXmlValueBufferContentLength(parser->vbNextToken)
    ) {
      if (parser->szAttribute != NULL) {
        free(parser->szAttribute);
      }
      parser->nAttributeBufferSize= getSimpleXmlValueBufferContentLength(parser->vbNextToken);
      parser->szAttribute= malloc(parser->nAttributeBufferSize);
    }
    if (parser->szAttribute == NULL) {
      parser->nError= OUT_OF_MEMORY;
      return FAIL;
    }
    if (
      getSimpleXmlValueBufferContents(
        parser->vbNextToken, parser->szAttribute, parser->nAttributeBufferSize
      ) == FAIL
    ) {
      parser->nError= OUT_OF_MEMORY;
      return FAIL;
    }
    clearSimpleXmlValueBuffer(parser->vbNextToken);
    /* skip whitespace */
    if (skipWhitespace(parser) == FAIL) {
      return FAIL;
    }
    cQuote= readInputChar(parser);
    if (parser->nError != NO_ERROR) {
      return FAIL;
    }
    if (cQuote != '\'' && cQuote != '"') {
      parser->nError= QUOTE_EXPECTED;
      return FAIL;
    }
    while (peekInputChar(parser) != cQuote) {
      /* read one character into next token value buffer */
      if (readChar(parser) == FAIL) {
        return FAIL;
      }
    }
    /* skip quote character */
    skipInputChar(parser);
    /* skip whitespace */
    if (skipWhitespace(parser) == FAIL) {
      return FAIL;
    }
  }
  return SUCCESS;
}

/**
 * Scanner that reads the next token type and sets
 * the nNextToken type and the value buffer of the
 * parser. Must not be invoked when the last token
 * read was a TAG_BEGIN (use readNextTagToken in
 * such a case).
 *
 * The following token types are supported:
 *
 * Type                   | ValueBuffer | Example
 * -----------------------+-------------+-------------
 * TAG_BEGIN_OPENING      | foo         | <foo
 * TAG_END                | foo         | </foo>
 * CONTENT                | foo         | foo
 * PROCESSING_INSTRUCTION | XML         | <?XML?>
 * UNKNOWN                | WHATEVER    | <!WHATEVER>
 * COMMENT                | foo         | <!--foo-->
 * DOCTYPE                | foo         | <!DOCTYPEfoo>
 *
 * @param parser the parser for which to read the next token.
 * @return SUCCESS or FAIL.
 */
int readNextContentToken (SimpleXmlParserState parser) {
  /* read markup or content */
  if (peekInputChar(parser) == '<') {
    /* read markup */
    skipInputChar(parser);
    if (peekInputChar(parser) == '/') {
      /* read tag closing */
      parser->nNextToken= TAG_END;
      skipInputChar(parser);
      while (
        peekInputChar(parser) > SPACE &&
        peekInputChar(parser) != '>'
      ) {
        /* read one character into next token value buffer */
        if (readChar(parser) == FAIL) {
          return FAIL;
        }
      }
      while (peekInputChar(parser) != '>') {
        skipInputChar(parser);
      }
      if (peekInputChar(parser) != '>') {
        parser->nError= EARLY_TERMINATION;
        return FAIL;
      }
      /* skip closing '>' */
      skipInputChar(parser);
    } else if (peekInputChar(parser) == '?') {
      /* read processing instruction (e.g. <?XML ... ?> prolog) */
      parser->nNextToken= PROCESSING_INSTRUCTION;
      skipInputChar(parser);
      while (
        peekInputCharAt(parser, 0) != '?' ||
        peekInputCharAt(parser, 1) != '>'
      ) {
        /* read one character into next token value buffer */
        if (readChar(parser) == FAIL) {
          return FAIL;
        }
      }
      /* skip closing '?>' */
      skipInputChars(parser, 2);
    } else if (peekInputChar(parser) == '!') {
      /* read comment, doctype or cdata */
      skipInputChar(parser);
      if (
        peekInputCharAt(parser, 0) == '-' &&
        peekInputCharAt(parser, 1) == '-'
      ) {
        /* read comment */
        parser->nNextToken= COMMENT;
        skipInputChars(parser, 2);
        while (
          peekInputCharAt(parser, 0) != '-' ||
          peekInputCharAt(parser, 1) != '-' ||
          peekInputCharAt(parser, 2) != '>'
        ) {
          /* read one character into next token value buffer */
          if (readChar(parser) == FAIL) {
            return FAIL;
          }
        }
        /* skip closing '-->' */
        skipInputChars(parser, 3);
      } else if (
        peekInputCharAt(parser, 0) == 'D' &&
        peekInputCharAt(parser, 1) == 'O' &&
        peekInputCharAt(parser, 2) == 'C' &&
        peekInputCharAt(parser, 3) == 'T' &&
        peekInputCharAt(parser, 4) == 'Y' &&
        peekInputCharAt(parser, 5) == 'P' &&
        peekInputCharAt(parser, 6) == 'E'
      ) {
        /* read doctype declaration (external only) */
        int nCount= 1;
        parser->nNextToken= DOCTYPE;
        skipInputChars(parser, 7);
        while (nCount > 0) {
          if (peekInputChar(parser) == '>') {
            nCount--;
          } else if (peekInputChar(parser) == '<') {
            nCount++;
          }
          /* read one character into next token value buffer */
          if (nCount > 0 && readChar(parser) == FAIL) {
            return FAIL;
          }
        }
        /* skip closing '>' */
        skipInputChar(parser);
      } else {
        /* read cdata, not supported yet \,
           simply skip to the next closing '>' */
        parser->nNextToken= UNKNOWN;
        while (peekInputChar(parser) != '>') {
          /* read one character into next token value buffer */
          if (readChar(parser) == FAIL) {
            return FAIL;
          }
        }
        /* skip closing '>' */
        skipInputChar(parser);
      }
    } else {
      /* read tag opening (without '>' or '/>') */
      parser->nNextToken= TAG_BEGIN_OPENING;
      while (
        peekInputChar(parser) > SPACE &&
        peekInputChar(parser) != '/' &&
        peekInputChar(parser) != '>'
      ) {
        /* read one character into next token value buffer */
        if (readChar(parser) == FAIL) {
          return FAIL;
        }
      }
      /* skip whitespace */
      if (skipWhitespace(parser) == FAIL) {
        return FAIL;
      }
    }
  } else {
    /* read content */
    parser->nNextToken= CONTENT;
    while (peekInputChar(parser) != '<') {
      /* read one character into next token value buffer */
      if (readChar(parser) == FAIL) {
        return FAIL;
      }
    }
  }
  return SUCCESS;
}

/**
 * Reads the next character from the input data and
 * appends it to the next token value buffer of the parser.
 *
 * Note: This method does not support unicode and 8-bit
 * characters are read using the default platform encoding.
 *
 * @param parser the parser for which to read the next input character.
 * @return SUCCESS or FAIL.
 */
int readChar (SimpleXmlParserState parser) {
  char c= readInputChar(parser);
  if (c == '\0' && parser->nError != NO_ERROR) {
    return FAIL;
  } else if (c == '&') {
    /* read entity */
    if (peekInputCharAt(parser, 0) == '#') {
      int nCode= 0;
      skipInputChar(parser);
      c= readInputChar(parser);
      if (c == 'x') {
        /* &#x<hex>; */
        c= readInputChar(parser);
        while (c != ';') {
          if (c >= '0' && c <= '9') {
            nCode= (nCode * 16) + (c - '0');
          } else if (c >= 'A' && c <= 'F') {
            nCode= (nCode * 16) + (c - 'A' + 10);
          } else if (c >= 'a' && c <= 'f') {
            nCode= (nCode * 16) + (c - 'a' + 10);
          } else {
            parser->nError= ILLEGAL_AMPERSAND;
            return FAIL;
          }
          c= readInputChar(parser);
        }
      } else if (c >= '0' && c <= '9') {
        /* &#<dec>; */
        c= readInputChar(parser);
        while (c != ';') {
          if (c >= '0' && c <= '9') {
            nCode= (nCode * 16) + (c - '0');
          } else {
            parser->nError= ILLEGAL_AMPERSAND;
            return FAIL;
          }
          c= readInputChar(parser);
        }
      } else {
        /* illegal use of ampersand */
        parser->nError= ILLEGAL_AMPERSAND;
        return FAIL;
      }
      if (nCode > 255) {
        parser->nError= NO_UNICODE_SUPPORT;
        return FAIL;
      }
      return addNextTokenCharValue(parser, (char) nCode);
    } else if (
      peekInputCharAt(parser, 0) == 'a' &&
      peekInputCharAt(parser, 1) == 'm' &&
      peekInputCharAt(parser, 2) == 'p' &&
      peekInputCharAt(parser, 3) == ';'
    ) {
      /* &amp; -> & */
      skipInputChars(parser, 4);
      return addNextTokenCharValue(parser, '&');
    } else if (
      peekInputCharAt(parser, 0) == 'a' &&
      peekInputCharAt(parser, 1) == 'p' &&
      peekInputCharAt(parser, 2) == 'o' &&
      peekInputCharAt(parser, 3) == 's' &&
      peekInputCharAt(parser, 4) == ';'
    ) {
      /* &apos; -> ' */
      skipInputChars(parser, 5);
      return addNextTokenCharValue(parser, '\'');
    } else if (
      peekInputCharAt(parser, 0) == 'q' &&
      peekInputCharAt(parser, 1) == 'u' &&
      peekInputCharAt(parser, 2) == 'o' &&
      peekInputCharAt(parser, 3) == 't' &&
      peekInputCharAt(parser, 4) == ';'
    ) {
      /* &quot; -> " */
      skipInputChars(parser, 5);
      return addNextTokenCharValue(parser, '"');
    } else if (
      peekInputCharAt(parser, 0) == 'l' &&
      peekInputCharAt(parser, 1) == 't' &&
      peekInputCharAt(parser, 2) == ';'
    ) {
      /* &lt; -> < */
      skipInputChars(parser, 3);
      return addNextTokenCharValue(parser, '<');
    } else if (
      peekInputCharAt(parser, 0) == 'g' &&
      peekInputCharAt(parser, 1) == 't' &&
      peekInputCharAt(parser, 2) == ';'
    ) {
      /* &gt; -> > */
      skipInputChars(parser, 3);
      return addNextTokenCharValue(parser, '>');
    } else {
      /* illegal use of ampersand */
      parser->nError= ILLEGAL_AMPERSAND;
      return FAIL;
    }
  } else {
    /* read simple character */
    return addNextTokenCharValue(parser, c);
  }
}

/**
 * Peeks at the character with the specified offset from
 * the cursor (i.e. the last character read).
 *
 * Note: To peek at the next character that will be read
 * use and offset of 0.
 *
 * @param parser the parser for which to peek.
 * @param nOffset the peek offset relative to the
 * position of the last char read.
 * @return the peeked character or '\0' if there are
 * no more data.
 */
char peekInputCharAt (SimpleXmlParserState parser, int nOffset) {
  int nPos= parser->nInputDataPos + nOffset;
  if (nPos < 0 || nPos >= parser->nInputDataSize) {
    return '\0';
  }
  char tmp[1];
  int fd = cfs_open("/input.xml",CFS_READ);
  cfs_seek(fd,nPos,CFS_SEEK_SET);
  cfs_read(fd,tmp,1);
  cfs_close(fd);
  return tmp[0];
  //return parser->sInputData[nPos];
}

/**
 * Peeks at the current input character at the read cursor
 * position of the specified parser.
 *
 * @param parser the parser for which to peek.
 * @return the peeked character or '\0' if there are
 * no more data.
 */
char peekInputChar (SimpleXmlParserState parser) {
  return peekInputCharAt(parser, 0);
}

/**
 * Skips any whitespace at the cursor position of the
 * parser specified.
 *
 * Note: All characters smaller than the space character
 * are considered to be whitespace.
 *
 * @param parser the parser for which to skip whitespace.
 * @return SUCCESS or FAIL.
 */
int skipWhitespace (SimpleXmlParserState parser) {
  while (peekInputChar(parser) <= SPACE) {
    /* skip whitespace but make sure we don't
       read more than is available */
    readInputChar(parser);
    if (parser->nError != NO_ERROR) {
      return FAIL;
    }
  }
  return SUCCESS;
}

/**
 * Moves the read cursor of the specified parser by the
 * specified amount.
 *
 * @param parser the parser whose read cursor is to be moved.
 * @param nAmount the amount by which to move the cursor (> 0).
 */
void skipInputChars (SimpleXmlParserState parser, int nAmount) {
  int i;
  for (i= 0; i < nAmount; i++) {
    skipInputChar(parser);
  }
}

/**
 * Skips the current input read character.
 *
 * @param parser the parser whose read cursor should be incremented.
 * @see #skipInputChars
 * @see #peekInputChar
 */
void skipInputChar (SimpleXmlParserState parser) {
  char tmp[2];
  int fd = cfs_open("/input.xml",CFS_READ);
  cfs_seek(fd,parser->nInputDataPos,CFS_SEEK_SET);
  cfs_read(fd,tmp,2);
  cfs_close(fd);
  if (parser->nInputDataPos >= 0 && parser->nInputDataPos < parser->nInputDataSize) {
   // if (parser->sInputData[parser->nInputDataPos] == LF) {
    if (tmp[0] == LF) {
      /* always increment line counter on a linefeed */
      parser->nInputLineNumber++;
    //} else if (parser->sInputData[parser->nInputDataPos] == CR) {
    } else if (tmp[0] == CR) {
      /* only increment on an carriage return if no linefeed follows
         (this is for the mac filetypes) */
      if (parser->nInputDataPos + 1 < parser->nInputDataSize) {
       // if (parser->sInputData[parser->nInputDataPos + 1] != LF) {
        if (tmp[1] != LF) {
          parser->nInputLineNumber++;
        }
      }
    }
  }
  parser->nInputDataPos++;
}

/**
 * Reads the next input character from the specified parser
 * and returns it.
 *
 * Note: If an error is encountered '\0' is returned and the
 * nError flag of the parser is set to EARLY_TERMINATION.
 *
 * @param parser the parser from which to read the next
 * input character.
 * @return the next input character or '\0' if there is none.
 */
char readInputChar (SimpleXmlParserState parser) {
  char cRead;
  if (parser->nInputDataPos < 0 || parser->nInputDataPos >= parser->nInputDataSize) {
    parser->nError= EARLY_TERMINATION;
    return '\0';
  }
  char tmp[1];
  int fd = cfs_open("/input.xml",CFS_READ);
  cfs_seek(fd,parser->nInputDataPos,CFS_SEEK_SET);
  cfs_read(fd,tmp,1);
  cfs_close(fd);
 // cRead= parser->sInputData[parser->nInputDataPos];
  cRead = tmp[0];
  skipInputChar(parser);
  return cRead;
}

/**
 * Appends a character to the next token value string.
 *
 * @param parser the parser whose next token value string
 * is to be modified.
 * @param c the character to append.
 * @return SUCCESS or FAIL (if there is not enough memory).
 */
int addNextTokenCharValue (SimpleXmlParserState parser, char c) {
  if (appendCharToSimpleXmlValueBuffer(parser->vbNextToken, c) == FAIL) {
    parser->nError= OUT_OF_MEMORY;
    return FAIL;
  }
  return SUCCESS;
}

/**
 * Appends a zero terminated string to the next token value string.
 *
 * @param parser the parser whose next token value string
 * is to be modified.
 * @param szInput the zero terminated string to append.
 * @return SUCCESS or FAIL (if there is not enough memory).
 */
int addNextTokenStringValue (SimpleXmlParserState parser, char *szInput) {
  while (*szInput != '\0') {
    if (addNextTokenCharValue(parser, *szInput) == FAIL) {
      return FAIL;
    }
    szInput++;
  }
  return SUCCESS;

}

/* ---- SimpleXmlValueBuffer */

/**
 * Creates a new value buffer of the specified size.
 *
 * The value buffer automatically grows when appending characters
 * if it is not large enough.
 *
 * The value buffer uses 'malloc' to allocate buffer space.
 * The user is responsible for freeing the value buffer created
 * using destroySimpleXmlValueBuffer.
 *
 * @param nInitialSize the initial size of the value buffer in chars.
 * @return NULL if the value buffer could not be allocated,
 * the newly allocated value buffer otherwise (to be freed by the
 * caller).
 * @see #destroySimpleXmlValueBuffer
 */
SimpleXmlValueBuffer createSimpleXmlValueBuffer (long nInitialSize) {
  SimpleXmlValueBuffer vb= malloc(sizeof(TSimpleXmlValueBuffer));
  if (vb == NULL) {
    return NULL;
  }
  vb->sBuffer= malloc(nInitialSize);
  if (vb->sBuffer == NULL) {
    free(vb);
    return NULL;
  }
  vb->nSize= nInitialSize;
  vb->nPosition= 0;
  return vb;
}

/**
 * Destroys a value buffer created using createSimpleXmlValueBuffer.
 *
 * @param vb the value buffer to destroy.
 * @see #destroySimpleXmlValueBuffer
 */
void destroySimpleXmlValueBuffer (SimpleXmlValueBuffer vb) {
  if (vb != NULL) {
    if (vb->sBuffer != NULL) {
      free(vb->sBuffer);
    }
    free(vb);
  }
}

/**
 * Grows the internal data buffer of the value buffer.
 *
 * @param vb the value buffer to grow.
 * @return SUCCESS or FAIL (if there is not enough memory).
 */
int growSimpleXmlValueBuffer (SimpleXmlValueBuffer vb) {
  char* sOldBuffer= vb->sBuffer;
  char* sNewBuffer= malloc(vb->nSize * 2);
  if (sNewBuffer == NULL) {
    return FAIL;
  }
  memcpy(sNewBuffer, vb->sBuffer, vb->nSize);
  vb->sBuffer= sNewBuffer;
  vb->nSize= vb->nSize * 2;
  free(sOldBuffer);
  return SUCCESS;
}

/**
 * Appends a character to the value buffer.
 *
 * @param vb the value buffer to append to.
 * @param c the character to append.
 * @return SUCCESS or FAIL (if there is not enough memory).
 */
int appendCharToSimpleXmlValueBuffer (SimpleXmlValueBuffer vb, char c) {
  if (vb == NULL) {
    return FAIL;
  }
  if (vb->nPosition >= vb->nSize) {
    if (growSimpleXmlValueBuffer(vb) == FAIL) {
      return FAIL;
    }
  }
  vb->sBuffer[vb->nPosition++]= c;
  return SUCCESS;
}

/**
 * Appends a zero terminated string to the value buffer.
 *
 * @param vb the value buffer to append to.
 * @param szInput the input string to append.
 * @return SUCCESS or FAIL (if there is not enough memory).
 */
int appendStringToSimpleXmlValueBuffer (SimpleXmlValueBuffer vb, const char *szInput) {
  while (*szInput != '\0') {
    if (appendCharToSimpleXmlValueBuffer(vb, *szInput) == FAIL) {
      return FAIL;
    }
    szInput++;
  }
  return SUCCESS;
}

/**
 * Zero terminates the internal buffer without appending
 * any characters (i.e. the append location is not modified).
 *
 * @param vb the value buffer to zero terminate.
 * @return SUCCESS or FAIL (if there is not enough memory).
 */
int zeroTerminateSimpleXmlValueBuffer (SimpleXmlValueBuffer vb) {
  if (vb == NULL) {
    return FAIL;
  }
  if (vb->nPosition >= vb->nSize) {
    if (growSimpleXmlValueBuffer(vb) == FAIL) {
      return FAIL;
    }
  }
  vb->sBuffer[vb->nPosition]= '\0';
  return SUCCESS;
}

/**
 * Resets the append location of the value buffer.
 *
 * @param vb the value buffer to clear.
 * @return SUCCESS or FAIL.
 */
int clearSimpleXmlValueBuffer (SimpleXmlValueBuffer vb) {
  if (vb == NULL) {
    return FAIL;
  }
  vb->nPosition= 0;
  return SUCCESS;
}

/**
 * Returns the content length of the value buffer
 * (including a trailing zero termination character).
 *
 * @param vb the value buffer whose content length should
 * be determined.
 * @return the content length (i.e. 1 if the content is
 * empty, 0 in case of a failure).
 */
int getSimpleXmlValueBufferContentLength (SimpleXmlValueBuffer vb) {
	if (vb == NULL) {
		return 0;
	}
	return vb->nPosition + 1;
}

/**
 * Retrieves the buffer content and stores it to the
 * specified output array.
 *
 * @param vb the value buffer whose content should be retrieved.
 * @param szOutput the output character array to store it to,
 * the output array is in any case zero terminated!
 * @param nMaxLen the maximum number of characters to write to
 * the output array.
 * @return SUCCESS or FAIL.
 */
int getSimpleXmlValueBufferContents (SimpleXmlValueBuffer vb, char* szOutput, long nMaxLen) {
	int nMax; /* max. number of chars to copy */
	if (vb == NULL) {
		return FAIL;
	}
	nMaxLen-= 1; /* reserve space for terminating zero */
	nMax= nMaxLen < vb->nPosition ? nMaxLen : vb->nPosition;
	memcpy(szOutput, vb->sBuffer, nMax);
	szOutput[nMax]= '\0';
	return SUCCESS;
}

/**
 * Returns the zero terminated internal string buffer of
 * the value buffer specified.
 *
 * Warning: Modifying the array returned modifies the
 * internal data of the value buffer!
 *
 * @param vb the value buffer whose string buffer should
 * be returned.
 * @return the string buffer or NULL (if there is not enough memory).
 */
char* getInternalSimpleXmlValueBufferContents (SimpleXmlValueBuffer vb) {
	if (zeroTerminateSimpleXmlValueBuffer(vb) == FAIL) {
		return NULL;
	}
	return vb->sBuffer;
}

//DY : made user data simpler, it was a stack before
void* simpleXmlGetUserData(SimpleXmlParser parser)
{
  void* pData = NULL;
  if (parser)
  {
    pData = ((SimpleXmlParserState) parser)->pUserDefinedData;
  }

  return pData;
}

int simpleXmlSetUserData(SimpleXmlParser parser, void* pData)
{
  int returnValue = SUCCESS;
  if ( parser && pData )
  {
    ((SimpleXmlParserState) parser)->pUserDefinedData = pData;
    returnValue = FAIL;
  }

  return returnValue;
}

//DY : attribute handlers
const char* simpleXmlGetAttrUri(size_t nNumber, const char** attr)
{
  return attr[nNumber*3];
}

const char* simpleXmlGetAttrName(size_t nNumber, const char** attr)
{
  return attr[nNumber*3 + 1];
}

const char* simpleXmlGetAttrValue(size_t nNumber, const char** attr)
{
  return attr[nNumber*3 + 2];
}

size_t simpleXmlGetNumOfAttrs(const char** attr)
{
  size_t nNumber = 0;
  size_t nAttrIndex = 0;
  while( attr[nAttrIndex] )
  {
    nAttrIndex += 3;
    nNumber++;
  }

  return nNumber;
}

//added for xml generation
//DY : FIX_ME : encoding not handled yet
//DY : FIX_ME : can be made more intelligent by adding more states and level of tags. For example, if no text element no need for closing tag.
int getOutputSize(XmlWriter* xmlWriter){
	return xmlWriter->xmlWriteBuffer.nPosition;
}
#define POSITION(xmlWriter) (xmlWriter->xmlWriteBuffer.nPosition)
#define STATE(xmlWriter) (xmlWriter->state)
#define WRITE_BUFFER(xmlWriter) (&(xmlWriter->xmlWriteBuffer.sBuffer[POSITION(xmlWriter)]))
void writeToFile(XmlWriter* xmlWriter, int fd, const char* value){
	cfs_seek(fd, POSITION(xmlWriter), CFS_SEEK_SET);
	if (strlen(value)==1){
		char tmp[2];
		tmp[0]=value[0];
		tmp[1]=' ';
		cfs_write(fd,tmp,2);

	} else {
		cfs_write(fd,value,strlen(value));
	}
	POSITION(xmlWriter)+=strlen(value);
}
void simpleXmlStartDocument(XmlWriter* xmlWriter, char* buffer, unsigned short size)
{
  xmlWriter->state = CLOSED_TAG;
  xmlWriter->xmlWriteBuffer.sBuffer = buffer;
  xmlWriter->xmlWriteBuffer.sBuffer[0] = 0;
  xmlWriter->xmlWriteBuffer.nSize = size;
  xmlWriter->xmlWriteBuffer.nPosition = 0;

  //POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter),"<?xml version=\'1.0\' encoding=\'UTF-8\'?>");
#if CONTIKI_TARGET_AVR_RAVEN
	char buf[15];
	strcpy_P(buf, (char*)pgm_read_word(&(outfile[0])));
	int fdout = cfs_open(buf, CFS_WRITE);
#else
	int fdout = cfs_open("/output.xml", CFS_WRITE);
#endif
	writeToFile(xmlWriter, fdout, "<?xml version=\'1.0\' encoding=\'UTF-8\'?>");
	cfs_close(fdout);
}

void simpleXmlStartElement(XmlWriter* xmlWriter, const char* ns, const char* name)
{
#if CONTIKI_TARGET_AVR_RAVEN
	char buffer[15];
	strcpy_P(buffer, (char*)pgm_read_word(&(outfile[0])));
	int fdout = cfs_open(buffer, CFS_WRITE);
#else
	int fdout = cfs_open("/output.xml", CFS_WRITE);
#endif
	if ( STATE(xmlWriter) == OPENED_TAG ) //if parent element need closing angle
  {
    //POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter),">");

	writeToFile(xmlWriter, fdout, ">");
  }

  if (ns)
  {//raven_lcd_show_text("5");
    //POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter),"<%s:%s",ns,name);
	writeToFile(xmlWriter, fdout, "<");
	writeToFile(xmlWriter, fdout, ns);
	writeToFile(xmlWriter, fdout, ":");
	writeToFile(xmlWriter, fdout, name);
  }
  else
  {
    //POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter),"<%s",name);
	writeToFile(xmlWriter, fdout, "<");
	writeToFile(xmlWriter, fdout, name);
  }
  STATE(xmlWriter) = OPENED_TAG;
	cfs_close(fdout);
}

void simpleXmlAddAttribute(XmlWriter* xmlWriter, const char* ns, const char* name, const char* value)
{
#if CONTIKI_TARGET_AVR_RAVEN
	char buffer[15];
	strcpy_P(buffer, (char*)pgm_read_word(&(outfile[0])));
	int fdout = cfs_open(buffer, CFS_WRITE);
#else
	int fdout = cfs_open("/output.xml", CFS_WRITE);
#endif
  if (ns)
  {
    //POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter)," %s:%s=\"%s\"",ns, name,value);
	writeToFile(xmlWriter, fdout, " ");
	writeToFile(xmlWriter, fdout, ns);
	writeToFile(xmlWriter, fdout, ":");
	writeToFile(xmlWriter, fdout, name);
	writeToFile(xmlWriter, fdout, "=\"");
	writeToFile(xmlWriter, fdout, value);
	writeToFile(xmlWriter, fdout, "\"");
  }
  else
  {
    //POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter)," %s=\"%s\"",name,value);
	writeToFile(xmlWriter, fdout, " ");
	writeToFile(xmlWriter, fdout, name);
	writeToFile(xmlWriter, fdout, "=\"");
	writeToFile(xmlWriter, fdout, value);
	writeToFile(xmlWriter, fdout, "\"");
  }
	cfs_close(fdout);
}

void simpleXmlCharacters(XmlWriter* xmlWriter, const char* value)
{
#if CONTIKI_TARGET_AVR_RAVEN
	char buffer[15];
	strcpy_P(buffer, (char*)pgm_read_word(&(outfile[0])));
	int fdout = cfs_open(buffer, CFS_WRITE);
#else
	int fdout = cfs_open("/output.xml", CFS_WRITE);
#endif
  if ( STATE(xmlWriter) == OPENED_TAG )
  {
    //POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter),">");
	writeToFile(xmlWriter, fdout, ">");
    STATE(xmlWriter) = CLOSED_TAG;
  }

  if ( value )
  {
    //POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter),"%s",value);
	writeToFile(xmlWriter, fdout, value);
  }
	cfs_close(fdout);
}

void simpleXmlEndElement(XmlWriter* xmlWriter, const char* ns, const char* name)
{
#if CONTIKI_TARGET_AVR_RAVEN
	char buffer[15];
	strcpy_P(buffer, (char*)pgm_read_word(&(outfile[0])));
	int fdout = cfs_open(buffer, CFS_WRITE);
#else
	int fdout = cfs_open("/output.xml", CFS_WRITE);
#endif
  if ( STATE(xmlWriter) == OPENED_TAG )
  {
//    POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter),">");
	writeToFile(xmlWriter, fdout, ">");
  }

  if (ns)
  {
//    POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter),"</%s:%s>",ns,name);
	writeToFile(xmlWriter, fdout, "</");
	writeToFile(xmlWriter, fdout, ns);
	writeToFile(xmlWriter, fdout, ":");
	writeToFile(xmlWriter, fdout, name);
	writeToFile(xmlWriter, fdout, ">");
  }
  else
  {
   // POSITION(xmlWriter) += sprintf(WRITE_BUFFER(xmlWriter),"</%s>",name);
	writeToFile(xmlWriter, fdout, "</");
	writeToFile(xmlWriter, fdout, name);
	writeToFile(xmlWriter, fdout, ">");
  }
  STATE(xmlWriter) = CLOSED_TAG;
	cfs_close(fdout);
}

void simpleXmlEndDocument(XmlWriter* xmlWriter)
{
	//int fdout = cfs_open("/output.xml", CFS_WRITE);
	//writeToFile(xmlWriter, fdout, "\0");
	//cfs_close(fdout);
}