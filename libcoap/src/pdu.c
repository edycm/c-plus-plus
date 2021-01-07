/* pdu.c -- CoAP message structure
 *
 * Copyright (C) 2010--2016 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use.
 */

#include "coap_internal.h"

#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

void
coap_pdu_clear(coap_pdu_t *pdu, size_t size) {
  assert(pdu);
  assert(pdu->token);
  assert(pdu->max_hdr_size >= COAP_PDU_MAX_UDP_HEADER_SIZE);
  if (pdu->alloc_size > size)
    pdu->alloc_size = size;
  pdu->type = 0;
  pdu->code = 0;
  pdu->hdr_size = 0;
  pdu->token_length = 0;
  pdu->tid = 0;
  pdu->max_opt = 0;
  pdu->max_size = size;
  pdu->used_size = 0;
  pdu->data = NULL;
}

#ifdef WITH_LWIP
coap_pdu_t *
coap_pdu_from_pbuf( struct pbuf *pbuf )
{
  coap_pdu_t *pdu;

  if (pbuf == NULL) return NULL;

  LWIP_ASSERT("Can only deal with contiguous PBUFs", pbuf->tot_len == pbuf->len);
  LWIP_ASSERT("coap_io_do_io needs to receive an exclusive copy of the incoming pbuf", pbuf->ref == 1);

  pdu = coap_malloc_type(COAP_PDU, sizeof(coap_pdu_t) );
  if (!pdu) {
    pbuf_free(pbuf);
    return NULL;
  }

  pdu->max_hdr_size = COAP_PDU_MAX_UDP_HEADER_SIZE;
  pdu->pbuf = pbuf;
  pdu->token = (uint8_t *)pbuf->payload + pdu->max_hdr_size;
  pdu->alloc_size = pbuf->tot_len - pdu->max_hdr_size;
  coap_pdu_clear(pdu, pdu->alloc_size);

  return pdu;
}
#endif

coap_pdu_t *
coap_pdu_init(uint8_t type, uint8_t code, uint16_t tid, size_t size) {
  coap_pdu_t *pdu;

  pdu = coap_malloc_type(COAP_PDU, sizeof(coap_pdu_t));
  if (!pdu) return NULL;

#if defined(WITH_CONTIKI) || defined(WITH_LWIP)
  assert(size <= COAP_MAX_MESSAGE_SIZE_TCP16 + 4);
  if (size > COAP_MAX_MESSAGE_SIZE_TCP16 + 4)
    return NULL;
  pdu->max_hdr_size = COAP_PDU_MAX_UDP_HEADER_SIZE;
#else
  pdu->max_hdr_size = COAP_PDU_MAX_TCP_HEADER_SIZE;
#endif

#ifdef WITH_LWIP
  pdu->pbuf = pbuf_alloc(PBUF_TRANSPORT, size + pdu->max_hdr_size, PBUF_RAM);
  if (pdu->pbuf == NULL) {
    coap_free_type(COAP_PDU, pdu);
    return NULL;
  }
  pdu->token = (uint8_t *)pdu->pbuf->payload + pdu->max_hdr_size;
#else /* WITH_LWIP */
  uint8_t *buf;
  pdu->alloc_size = min(size, 256);
  buf = coap_malloc_type(COAP_PDU_BUF, pdu->alloc_size + pdu->max_hdr_size);
  if (buf == NULL) {
    coap_free_type(COAP_PDU, pdu);
    return NULL;
  }
  pdu->token = buf + pdu->max_hdr_size;
#endif /* WITH_LWIP */
  coap_pdu_clear(pdu, size);
  pdu->tid = tid;
  pdu->type = type;
  pdu->code = code;
  return pdu;
}

coap_pdu_t *
coap_new_pdu(const struct coap_session_t *session) {
  coap_pdu_t *pdu = coap_pdu_init(0, 0, 0, coap_session_max_pdu_size(session));
  if (!pdu)
    coap_log(LOG_CRIT, "coap_new_pdu: cannot allocate memory for new PDU\n");
  return pdu;
}

void
coap_delete_pdu(coap_pdu_t *pdu) {
  if (pdu != NULL) {
#ifdef WITH_LWIP
    pbuf_free(pdu->pbuf);
#else
    if (pdu->token != NULL)
      coap_free_type(COAP_PDU_BUF, pdu->token - pdu->max_hdr_size);
#endif
    coap_free_type(COAP_PDU, pdu);
  }
}

int
coap_pdu_resize(coap_pdu_t *pdu, size_t new_size) {
  if (new_size > pdu->alloc_size) {
#if !defined(WITH_LWIP) && !defined(WITH_CONTIKI)
    uint8_t *new_hdr;
    size_t offset;
#endif
    if (pdu->max_size && new_size > pdu->max_size) {
      coap_log(LOG_WARNING, "coap_pdu_resize: pdu too big\n");
      return 0;
    }
#if !defined(WITH_LWIP) && !defined(WITH_CONTIKI)
    if (pdu->data != NULL) {
      assert(pdu->data > pdu->token);
      offset = pdu->data - pdu->token;
    } else {
      offset = 0;
    }
    new_hdr = (uint8_t*)realloc(pdu->token - pdu->max_hdr_size, new_size + pdu->max_hdr_size);
    if (new_hdr == NULL) {
      coap_log(LOG_WARNING, "coap_pdu_resize: realloc failed\n");
      return 0;
    }
    pdu->token = new_hdr + pdu->max_hdr_size;
    if (offset > 0)
      pdu->data = pdu->token + offset;
    else
      pdu->data = NULL;
#endif
  }
  pdu->alloc_size = new_size;
  return 1;
}

int
coap_pdu_check_resize(coap_pdu_t *pdu, size_t size) {
  if (size > pdu->alloc_size) {
    size_t new_size = max(256, pdu->alloc_size * 2);
    while (size > new_size)
      new_size *= 2;
    if (pdu->max_size && new_size > pdu->max_size) {
      new_size = pdu->max_size;
      if (new_size < size)
        return 0;
    }
    if (!coap_pdu_resize(pdu, new_size))
      return 0;
  }
  return 1;
}

int
coap_add_token(coap_pdu_t *pdu, size_t len, const uint8_t *data) {
  /* must allow for pdu == NULL as callers may rely on this */
  if (!pdu || len > 8)
    return 0;

  if (pdu->used_size) {
    coap_log(LOG_WARNING,
             "coap_add_token: The token must defined first. Token ignored\n");
    return 0;
  }
  if (!coap_pdu_check_resize(pdu, len))
    return 0;
  pdu->token_length = (uint8_t)len;
  if (len)
    memcpy(pdu->token, data, len);
  pdu->max_opt = 0;
  pdu->used_size = len;
  pdu->data = NULL;

  return 1;
}

size_t
coap_insert_option(coap_pdu_t *pdu, uint16_t type, size_t len,
                   const uint8_t *data) {
  coap_opt_iterator_t opt_iter;
  coap_opt_t *option;
  uint16_t prev_type = 0;
  size_t shift;
  size_t opt_delta;
  coap_option_t decode;
  size_t shrink = 0;

  if (type >= pdu->max_opt)
    return coap_add_option(pdu, type, len, data);

  /* Need to locate where in current options to insert this one */
  coap_option_iterator_init(pdu, &opt_iter, COAP_OPT_ALL);
  while ((option = coap_option_next(&opt_iter))) {
    if (opt_iter.type > type) {
      /* Found where to insert */
      break;
    }
    prev_type = opt_iter.type;
  }
  assert(option != NULL);
  /* size of option inc header to insert */
  shift = coap_opt_encode_size(type - prev_type, len);

  /* size of next option (header may shrink in size as delta changes */
  if (!coap_opt_parse(option, opt_iter.length + 5, &decode))
    return 0;
  opt_delta = opt_iter.type - type;

  if (!coap_pdu_check_resize(pdu,
                         pdu->used_size + shift - shrink))
    return 0;

  /* Possible a re-size took place with a realloc() */
  /* Need to locate where in current options to insert this one */
  coap_option_iterator_init(pdu, &opt_iter, COAP_OPT_ALL);
  while ((option = coap_option_next(&opt_iter))) {
    if (opt_iter.type > type) {
      /* Found where to insert */
      break;
    }
  }
  assert(option != NULL);

  if (decode.delta <= 12) {
    /* can simply patch in the new delta of next option */
    option[0] = (option[0] & 0x0f) + (opt_delta << 4);
  }
  else if (decode.delta <= 269 && opt_delta <= 12) {
    /* option header is going to shrink by one byte */
    option[1] = (option[0] & 0x0f) + (opt_delta << 4);
    shrink = 1;
  }
  else if (decode.delta <= 269 && opt_delta <= 269) {
    /* can simply patch in the new delta of next option */
    option[1] = opt_delta - 13;
  }
  else if (opt_delta <= 12) {
    /* option header is going to shrink by two bytes */
    option[2] = (option[0] & 0x0f) + (opt_delta << 4);
    shrink = 2;
  }
  else if (opt_delta <= 269) {
    /* option header is going to shrink by one bytes */
    option[1] = (option[0] & 0x0f) + 0xd0;
    option[2] = opt_delta - 13;
    shrink = 1;
  }
  else {
    /* can simply patch in the new delta of next option */
    option[1] = (opt_delta - 269) >> 8;
    option[2] = (opt_delta - 269) & 0xff;
  }

  memmove(&option[shift], &option[shrink],
          pdu->used_size - (option - pdu->token) - shrink);
  if (!coap_opt_encode(option, pdu->alloc_size - pdu->used_size,
                            type - prev_type, data, len))
    return 0;

  pdu->used_size += shift - shrink;
  if (pdu->data)
    pdu->data += shift - shrink;
  return shift;
}

int
coap_update_option(coap_pdu_t *pdu, uint16_t type, size_t len,
                   const uint8_t *data) {
  coap_opt_iterator_t opt_iter;
  coap_opt_t *option;
  coap_option_t decode;
  size_t new_length = 0;
  size_t old_length = 0;

  option = coap_check_option(pdu, type, &opt_iter);
  if (!option)
    return coap_insert_option(pdu, type, len, data);

  old_length = coap_opt_parse(option, (size_t)-1, &decode);
  if (old_length == 0)
    return 0;
  new_length = coap_opt_encode_size(decode.delta, len);

  if (new_length > old_length) {
    if (!coap_pdu_check_resize(pdu,
                         pdu->used_size + new_length - old_length))
      return 0;
    /* Possible a re-size took place with a realloc() */
    option = coap_check_option(pdu, type, &opt_iter);
  }

  if (new_length != old_length)
    memmove(&option[new_length], &option[old_length],
            pdu->used_size - (option - pdu->token) - old_length);

  if (!coap_opt_encode(option, new_length,
                            decode.delta, data, len))
    return 0;

  pdu->used_size += new_length - old_length;
  if (pdu->data)
    pdu->data += new_length - old_length;
  return 1;
}

/* FIXME: de-duplicate code with coap_add_option_later */
size_t
coap_add_option(coap_pdu_t *pdu, uint16_t type, size_t len,
                const uint8_t *data) {
  size_t optsize;
  coap_opt_t *opt;

  assert(pdu);
  pdu->data = NULL;

  if (type == pdu->max_opt) {
    /* Validate that the option is repeatable */
    switch (type) {
    /* Ignore list of genuine repeatable */
    case COAP_OPTION_IF_MATCH:
    case COAP_OPTION_ETAG:
    case COAP_OPTION_LOCATION_PATH:
    case COAP_OPTION_URI_PATH:
    case COAP_OPTION_URI_QUERY:
    case COAP_OPTION_LOCATION_QUERY:
      break;
    default:
      coap_log(LOG_INFO, "Option %d is not defined as repeatable\n", type);
      /* Accepting it after warning as there may be user defineable options */
      break;
    }
  }

  if (COAP_PDU_IS_REQUEST(pdu) &&
      (type == COAP_OPTION_PROXY_URI || type == COAP_OPTION_PROXY_SCHEME)) {
    /*
     * Need to check whether there is a hop-limit option.  If not, it needs
     * to be inserted by default (RFC 8768).
     */
    coap_opt_iterator_t opt_iter;

    if (coap_check_option(pdu, COAP_OPTION_HOP_LIMIT, &opt_iter) == NULL) {
      size_t hop_limit = COAP_OPTION_HOP_LIMIT;

      coap_insert_option(pdu, COAP_OPTION_HOP_LIMIT, 1, (uint8_t *)&hop_limit);
    }
  }

  if (type < pdu->max_opt) {
    coap_log(LOG_DEBUG,
             "coap_add_option: options are not in correct order\n");
    return coap_insert_option(pdu, type, len, data);
  }

  if (!coap_pdu_check_resize(pdu,
      pdu->used_size + coap_opt_encode_size(type - pdu->max_opt, len)))
    return 0;

  opt = pdu->token + pdu->used_size;

  /* encode option and check length */
  optsize = coap_opt_encode(opt, pdu->alloc_size - pdu->used_size,
                            type - pdu->max_opt, data, len);

  if (!optsize) {
    coap_log(LOG_WARNING, "coap_add_option: cannot add option\n");
    /* error */
    return 0;
  } else {
    pdu->max_opt = type;
    pdu->used_size += optsize;
  }

  return optsize;
}

int
coap_add_data(coap_pdu_t *pdu, size_t len, const uint8_t *data) {
  if (len == 0) {
    return 1;
  } else {
    uint8_t *payload = coap_add_data_after(pdu, len);
    if (payload != NULL)
      memcpy(payload, data, len);
    return payload != NULL;
  }
}

uint8_t *
coap_add_data_after(coap_pdu_t *pdu, size_t len) {
  assert(pdu);
  assert(pdu->data == NULL);

  pdu->data = NULL;

  if (len == 0)
    return NULL;

  if (!coap_pdu_resize(pdu, pdu->used_size + len + 1))
    return 0;
  pdu->token[pdu->used_size++] = COAP_PAYLOAD_START;
  pdu->data = pdu->token + pdu->used_size;
  pdu->used_size += len;
  return pdu->data;
}

int
coap_get_data(const coap_pdu_t *pdu, size_t *len, uint8_t **data) {
  assert(pdu);
  assert(len);
  assert(data);

  *data = pdu->data;
  if(pdu->data == NULL) {
     *len = 0;
     return 0;
  }

  *len = pdu->used_size - (pdu->data - pdu->token);

  return 1;
}

#ifndef SHORT_ERROR_RESPONSE
typedef struct {
  unsigned char code;
  const char *phrase;
} error_desc_t;

/* if you change anything here, make sure, that the longest string does not
 * exceed COAP_ERROR_PHRASE_LENGTH. */
error_desc_t coap_error[] = {
  { COAP_RESPONSE_CODE(201), "Created" },
  { COAP_RESPONSE_CODE(202), "Deleted" },
  { COAP_RESPONSE_CODE(203), "Valid" },
  { COAP_RESPONSE_CODE(204), "Changed" },
  { COAP_RESPONSE_CODE(205), "Content" },
  { COAP_RESPONSE_CODE(231), "Continue" },
  { COAP_RESPONSE_CODE(400), "Bad Request" },
  { COAP_RESPONSE_CODE(401), "Unauthorized" },
  { COAP_RESPONSE_CODE(402), "Bad Option" },
  { COAP_RESPONSE_CODE(403), "Forbidden" },
  { COAP_RESPONSE_CODE(404), "Not Found" },
  { COAP_RESPONSE_CODE(405), "Method Not Allowed" },
  { COAP_RESPONSE_CODE(406), "Not Acceptable" },
  { COAP_RESPONSE_CODE(408), "Request Entity Incomplete" },
  { COAP_RESPONSE_CODE(412), "Precondition Failed" },
  { COAP_RESPONSE_CODE(413), "Request Entity Too Large" },
  { COAP_RESPONSE_CODE(415), "Unsupported Content-Format" },
  { COAP_RESPONSE_CODE(500), "Internal Server Error" },
  { COAP_RESPONSE_CODE(501), "Not Implemented" },
  { COAP_RESPONSE_CODE(502), "Bad Gateway" },
  { COAP_RESPONSE_CODE(503), "Service Unavailable" },
  { COAP_RESPONSE_CODE(504), "Gateway Timeout" },
  { COAP_RESPONSE_CODE(505), "Proxying Not Supported" },
  { COAP_RESPONSE_CODE(508), "Hop Limit Reached" },
  { 0, NULL }                        /* end marker */
};

const char *
coap_response_phrase(unsigned char code) {
  int i;
  for (i = 0; coap_error[i].code; ++i) {
    if (coap_error[i].code == code)
      return coap_error[i].phrase;
  }
  return NULL;
}
#endif

/**
 * Advances *optp to next option if still in PDU. This function
 * returns the number of bytes opt has been advanced or @c 0
 * on error.
 */
static size_t
next_option_safe(coap_opt_t **optp, size_t *length, uint16_t *max_opt) {
  coap_option_t option;
  size_t optsize;

  assert(optp); assert(*optp);
  assert(length);

  optsize = coap_opt_parse(*optp, *length, &option);
  if (optsize) {
    assert(optsize <= *length);

    /* signal an error if this option would exceed the
     * allowed number space */
    if (*max_opt + option.delta > COAP_MAX_OPT) {
      return 0;
    }
    *max_opt += option.delta;
    *optp += optsize;
    *length -= optsize;
  }

  return optsize;
}

size_t
coap_pdu_parse_header_size(coap_proto_t proto,
                           const uint8_t *data) {
  assert(data);
  size_t header_size = 0;

  if (proto == COAP_PROTO_TCP || proto==COAP_PROTO_TLS) {
    uint8_t len = *data >> 4;
    if (len < 13)
      header_size = 2;
    else if (len==13)
      header_size = 3;
    else if (len==14)
      header_size = 4;
    else
      header_size = 6;
  } else if (proto == COAP_PROTO_UDP || proto==COAP_PROTO_DTLS) {
    header_size = 4;
  }

  return header_size;
}

size_t
coap_pdu_parse_size(coap_proto_t proto,
                    const uint8_t *data,
                    size_t length) {
  assert(data);
  assert(proto == COAP_PROTO_TCP || proto == COAP_PROTO_TLS);
  assert(coap_pdu_parse_header_size(proto, data) <= length );

  size_t size = 0;

  if ((proto == COAP_PROTO_TCP || proto==COAP_PROTO_TLS) && length >= 1) {
    uint8_t len = *data >> 4;
    if (len < 13) {
      size = len;
    } else if (length >= 2) {
      if (len==13) {
        size = (size_t)data[1] + COAP_MESSAGE_SIZE_OFFSET_TCP8;
      } else if (length >= 3) {
        if (len==14) {
          size = ((size_t)data[1] << 8) + data[2] + COAP_MESSAGE_SIZE_OFFSET_TCP16;
        } else if (length >= 5) {
          size = ((size_t)data[1] << 24) + ((size_t)data[2] << 16)
               + ((size_t)data[3] << 8) + data[4] + COAP_MESSAGE_SIZE_OFFSET_TCP32;
        }
      }
    }
    size += data[0] & 0x0f;
  }

  return size;
}

int
coap_pdu_parse_header(coap_pdu_t *pdu, coap_proto_t proto) {
  uint8_t *hdr = pdu->token - pdu->hdr_size;
  if (proto == COAP_PROTO_UDP || proto == COAP_PROTO_DTLS) {
    assert(pdu->hdr_size == 4);
    if ((hdr[0] >> 6) != COAP_DEFAULT_VERSION) {
      coap_log(LOG_DEBUG, "coap_pdu_parse: UDP version not supported\n");
      return 0;
    }
    pdu->type = (hdr[0] >> 4) & 0x03;
    pdu->token_length = hdr[0] & 0x0f;
    pdu->code = hdr[1];
    pdu->tid = (uint16_t)hdr[2] << 8 | hdr[3];
  } else if (proto == COAP_PROTO_TCP || proto == COAP_PROTO_TLS) {
    assert(pdu->hdr_size >= 2 && pdu->hdr_size <= 6);
    pdu->type = COAP_MESSAGE_CON;
    pdu->token_length = hdr[0] & 0x0f;
    pdu->code = hdr[pdu->hdr_size-1];
    pdu->tid = 0;
  } else {
    coap_log(LOG_DEBUG, "coap_pdu_parse: unsupported protocol\n");
    return 0;
  }
  if (pdu->token_length > pdu->alloc_size) {
    /* Invalid PDU provided - not wise to assert here though */
    coap_log(LOG_DEBUG, "coap_pdu_parse: PDU header token size broken\n");
    pdu->token_length = (uint8_t)pdu->alloc_size;
    return 0;
  }
  return 1;
}

static int
coap_pdu_parse_opt_csm(coap_pdu_t *pdu, uint16_t len) {
  switch (pdu->code) {
  case COAP_SIGNALING_CSM:
    switch (pdu->max_opt) {
    case COAP_SIGNALING_OPTION_MAX_MESSAGE_SIZE:
      if (len > 4) goto bad;
      break;
    case COAP_SIGNALING_OPTION_BLOCK_WISE_TRANSFER:
      if (len > 0) goto bad;
      break;
    default:
      ;
    }
    break;
  case COAP_SIGNALING_PING:
  case COAP_SIGNALING_PONG:
    switch (pdu->max_opt) {
    case COAP_SIGNALING_OPTION_CUSTODY:
      if (len > 0) goto bad;
      break;
    default:
      ;
    }
    break;
  case COAP_SIGNALING_RELEASE:
    switch (pdu->max_opt) {
    case COAP_SIGNALING_OPTION_ALTERNATIVE_ADDRESS:
      if (len < 1 || len > 255) goto bad;
      break;
    case COAP_SIGNALING_OPTION_HOLD_OFF:
      if (len > 3) goto bad;
      break;
    default:
      ;
    }
    break;
  case COAP_SIGNALING_ABORT:
    switch (pdu->max_opt) {
    case COAP_SIGNALING_OPTION_BAD_CSM_OPTION:
      if (len > 2) goto bad;
      break;
    default:
      ;
    }
    break;
  default:
    ;
  }
  return 1;
bad:
  return 0;
}

int
coap_pdu_parse_opt(coap_pdu_t *pdu) {

  int good = 1;
  /* sanity checks */
  if (pdu->code == 0) {
    if (pdu->used_size != 0 || pdu->token_length) {
      coap_log(LOG_DEBUG, "coap_pdu_parse: empty message is not empty\n");
      return 0;
    }
  }

  if (pdu->token_length > pdu->used_size || pdu->token_length > 8) {
    coap_log(LOG_DEBUG, "coap_pdu_parse: invalid Token\n");
    return 0;
  }

  pdu->max_opt = 0;
  if (pdu->code == 0) {
    /* empty packet */
    pdu->used_size = 0;
    pdu->data = NULL;
  } else {
    /* skip header + token */
    coap_opt_t *opt = pdu->token + pdu->token_length;
    size_t length = pdu->used_size - pdu->token_length;
    uint32_t len;

    while (length > 0 && *opt != COAP_PAYLOAD_START) {
      len = coap_opt_length(opt);
      if ( !next_option_safe( &opt, &length, &pdu->max_opt ) ) {
        coap_log(LOG_DEBUG, "coap_pdu_parse: missing payload start code\n");
        return 0;
      }
      if (COAP_PDU_IS_SIGNALING(pdu)) {
        if (!coap_pdu_parse_opt_csm(pdu, len))
          goto bad;
        continue;
      }
      switch (pdu->max_opt) {
      case COAP_OPTION_IF_MATCH:      if (len > 8) goto bad;              break;
      case COAP_OPTION_URI_HOST:      if (len < 1 || len > 255) goto bad; break;
      case COAP_OPTION_ETAG:          if (len < 1 || len > 8) goto bad;   break;
      case COAP_OPTION_IF_NONE_MATCH: if (len != 0) goto bad;             break;
      case COAP_OPTION_OBSERVE:       if (len > 3) goto bad;              break;
      case COAP_OPTION_URI_PORT:      if (len > 2) goto bad;              break;
      case COAP_OPTION_LOCATION_PATH: if (len > 255) goto bad;            break;
      case COAP_OPTION_OSCORE:        if (len > 255) goto bad;            break;
      case COAP_OPTION_URI_PATH:      if (len > 255) goto bad;            break;
      case COAP_OPTION_CONTENT_FORMAT:if (len > 2) goto bad;              break;
      case COAP_OPTION_MAXAGE:        if (len > 4) goto bad;              break;
      case COAP_OPTION_URI_QUERY:     if (len < 1 || len > 255) goto bad; break;
      case COAP_OPTION_HOP_LIMIT:     if (len < 1 || len > 1) goto bad;   break;
      case COAP_OPTION_ACCEPT:        if (len > 2) goto bad;              break;
      case COAP_OPTION_LOCATION_QUERY:if (len > 255) goto bad;            break;
      case COAP_OPTION_BLOCK2:        if (len > 3) goto bad;              break;
      case COAP_OPTION_BLOCK1:        if (len > 3) goto bad;              break;
      case COAP_OPTION_SIZE2:         if (len > 4) goto bad;              break;
      case COAP_OPTION_PROXY_URI:    if (len < 1 || len > 1034) goto bad; break;
      case COAP_OPTION_PROXY_SCHEME:  if (len < 1 || len > 255) goto bad; break;
      case COAP_OPTION_SIZE1:         if (len > 4) goto bad;              break;
      case COAP_OPTION_NORESPONSE:    if (len > 1) goto bad;              break;
      default:
        ;
      }
      continue;
bad:
      coap_log(LOG_DEBUG,
               "coap_pdu_parse: %d.%02d: option %u has bad length %u\n",
               pdu->code >> 5, pdu->code & 0x1F, pdu->max_opt, len);
      good = 0;
    }

    if (length > 0) {
      assert(*opt == COAP_PAYLOAD_START);
      opt++; length--;

      if (length == 0) {
        coap_log(LOG_DEBUG,
                 "coap_pdu_parse: message ending in payload start marker\n");
        return 0;
      }
    }
    if (length > 0)
                pdu->data = (uint8_t*)opt;
    else
      pdu->data = NULL;
  }

  return good;
}

int
coap_pdu_parse(coap_proto_t proto,
               const uint8_t *data,
               size_t length,
               coap_pdu_t *pdu)
{
  size_t hdr_size;

  if (length == 0)
    return 0;
  hdr_size = coap_pdu_parse_header_size(proto, data);
  if (!hdr_size || hdr_size > length)
    return 0;
  if (hdr_size > pdu->max_hdr_size)
    return 0;
  if (!coap_pdu_resize(pdu, length - hdr_size))
    return 0;
#ifndef WITH_LWIP
  memcpy(pdu->token - hdr_size, data, length);
#endif
  pdu->hdr_size = (uint8_t)hdr_size;
  pdu->used_size = length - hdr_size;
  return coap_pdu_parse_header(pdu, proto) && coap_pdu_parse_opt(pdu);
}

size_t
coap_pdu_encode_header(coap_pdu_t *pdu, coap_proto_t proto) {
  if (proto == COAP_PROTO_UDP || proto == COAP_PROTO_DTLS) {
    assert(pdu->max_hdr_size >= 4);
    if (pdu->max_hdr_size < 4) {
      coap_log(LOG_WARNING,
           "coap_pdu_encode_header: not enough space for UDP-style header\n");
      return 0;
    }
    pdu->token[-4] = COAP_DEFAULT_VERSION << 6
                   | pdu->type << 4
                   | pdu->token_length;
    pdu->token[-3] = pdu->code;
    pdu->token[-2] = (uint8_t)(pdu->tid >> 8);
    pdu->token[-1] = (uint8_t)(pdu->tid);
    pdu->hdr_size = 4;
  } else if (proto == COAP_PROTO_TCP || proto == COAP_PROTO_TLS) {
    size_t len;
    assert(pdu->used_size >= pdu->token_length);
    if (pdu->used_size < pdu->token_length) {
      coap_log(LOG_WARNING, "coap_pdu_encode_header: corrupted PDU\n");
      return 0;
    }
    len = pdu->used_size - pdu->token_length;
    if (len <= COAP_MAX_MESSAGE_SIZE_TCP0) {
      assert(pdu->max_hdr_size >= 2);
      if (pdu->max_hdr_size < 2) {
        coap_log(LOG_WARNING,
              "coap_pdu_encode_header: not enough space for TCP0 header\n");
        return 0;
      }
      pdu->token[-2] = (uint8_t)len << 4
                     | pdu->token_length;
      pdu->token[-1] = pdu->code;
      pdu->hdr_size = 2;
    } else if (len <= COAP_MAX_MESSAGE_SIZE_TCP8) {
      assert(pdu->max_hdr_size >= 3);
      if (pdu->max_hdr_size < 3) {
        coap_log(LOG_WARNING,
              "coap_pdu_encode_header: not enough space for TCP8 header\n");
        return 0;
      }
      pdu->token[-3] = 13 << 4 | pdu->token_length;
      pdu->token[-2] = (uint8_t)(len - COAP_MESSAGE_SIZE_OFFSET_TCP8);
      pdu->token[-1] = pdu->code;
      pdu->hdr_size = 3;
    } else if (len <= COAP_MAX_MESSAGE_SIZE_TCP16) {
      assert(pdu->max_hdr_size >= 4);
      if (pdu->max_hdr_size < 4) {
        coap_log(LOG_WARNING,
              "coap_pdu_encode_header: not enough space for TCP16 header\n");
        return 0;
      }
      pdu->token[-4] = 14 << 4 | pdu->token_length;
      pdu->token[-3] = (uint8_t)((len - COAP_MESSAGE_SIZE_OFFSET_TCP16) >> 8);
      pdu->token[-2] = (uint8_t)(len - COAP_MESSAGE_SIZE_OFFSET_TCP16);
      pdu->token[-1] = pdu->code;
      pdu->hdr_size = 4;
    } else {
      assert(pdu->max_hdr_size >= 6);
      if (pdu->max_hdr_size < 6) {
        coap_log(LOG_WARNING,
              "coap_pdu_encode_header: not enough space for TCP32 header\n");
        return 0;
      }
      pdu->token[-6] = 15 << 4 | pdu->token_length;
      pdu->token[-5] = (uint8_t)((len - COAP_MESSAGE_SIZE_OFFSET_TCP32) >> 24);
      pdu->token[-4] = (uint8_t)((len - COAP_MESSAGE_SIZE_OFFSET_TCP32) >> 16);
      pdu->token[-3] = (uint8_t)((len - COAP_MESSAGE_SIZE_OFFSET_TCP32) >> 8);
      pdu->token[-2] = (uint8_t)(len - COAP_MESSAGE_SIZE_OFFSET_TCP32);
      pdu->token[-1] = pdu->code;
      pdu->hdr_size = 6;
    }
  } else {
    coap_log(LOG_WARNING, "coap_pdu_encode_header: unsupported protocol\n");
  }
  return pdu->hdr_size;
}
