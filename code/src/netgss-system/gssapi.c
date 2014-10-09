/* This file is included into netgss_bindings_stubs.c */

#include <gssapi.h>

/* In the following tag=0 means that the value was allocated by the GSSAPI
   provider, and needs to be deallocated by the special GSSAPI function for
   this purpose. tag=1 means that we allocated the value, and are also
   responsible for freeing it. tag=2 means something special.
*/

static value twrap_gss_buffer_t(long tag, gss_buffer_t buf);
static value twrap_gss_OID(long tag, gss_OID oid);
static value twrap_gss_OID_set(long tag, gss_OID_set set);
static value  wrap_gss_channel_bindings_t(gss_channel_bindings_t cb);
static value  wrap_gss_ctx_id_t(gss_ctx_id_t ctx);
static value  wrap_gss_cred_id_t(gss_cred_id_t cred);

static gss_buffer_t unwrap_gss_buffer_t(value);
static gss_OID      unwrap_gss_OID(value);
static gss_OID_set  unwrap_gss_OID_set(value);

static long         tag_gss_buffer_t(value);

static void attach_gss_buffer_t(value, value);


typedef OM_uint32 flags;
typedef int status_type_t;


#define raise_null_pointer netgss_null_pointer

static void netgss_null_pointer(void) {
    caml_raise_constant(*caml_named_value
                          ("Netgss_bindings.Null_pointer"));
}


static void netgss_free_buffer_contents(long tag, gss_buffer_t buf) {
    if (buf->value != NULL) {
        if (tag == 0) {
            OM_uint32 major, minor;
            major = gss_release_buffer(&minor, buf);
        } else {
            if (tag == 1) {
                stat_free(buf->value);
            }
        }
    }
    buf->value = NULL;
    buf->length = 0;
}


static void netgss_free_buffer(long tag, gss_buffer_t buf) {
    netgss_free_buffer_contents(tag, buf);
    /* The descriptor is always allocated by us: */
    stat_free(buf);
}


static gss_buffer_t netgss_alloc_buffer(void) {
    gss_buffer_t buf;
    buf = (gss_buffer_t) stat_alloc(sizeof(gss_buffer_desc));
    buf->value = NULL;
    buf->length = 0;
    return buf;
}


static void init_gss_buffer_t(gss_buffer_t *p) {
    gss_buffer_t buf;
    buf = netgss_alloc_buffer();
    buf->length = 0;
    buf->value = NULL;
    *p = buf;
}


CAMLprim value netgss_release_buffer(value b) {
    gss_buffer_t buf;
    buf = unwrap_gss_buffer_t(b);
    netgss_free_buffer_contents(tag_gss_buffer_t(b), buf);
    return Val_unit;
}


CAMLprim value netgss_buffer_of_string(value s) {
    gss_buffer_t buf;
    buf = netgss_alloc_buffer();
    buf->length = caml_string_length(s);
    buf->value = stat_alloc(buf->length);
    memcpy(buf->value, String_val(s), buf->length);
    return twrap_gss_buffer_t(1, buf);
}


CAMLprim value netgss_buffer_of_memory(value m) {
    value r;
    gss_buffer_t buf;
    buf = netgss_alloc_buffer();
    buf->length = caml_ba_byte_size(Caml_ba_array_val(m));
    buf->value = Caml_ba_data_val(m);
    r = twrap_gss_buffer_t(2, buf);
    attach_gss_buffer_t(r, m);
    return r;
}


CAMLprim value netgss_string_of_buffer(value b) {
    gss_buffer_t buf;
    value s;
    buf = unwrap_gss_buffer_t(b);
    if (buf->value == NULL) netgss_null_pointer();
    s = caml_alloc_string(buf->length);
    memcpy(String_val(s), buf->value, buf->length);
    return s;
}


CAMLprim value netgss_memory_of_buffer(value b) {
    gss_buffer_t buf;
    value m;
    buf = unwrap_gss_buffer_t(b);
    if (buf->value == NULL) netgss_null_pointer();
    m = caml_ba_alloc_dims(CAML_BA_UINT8 | CAML_BA_C_LAYOUT, 1,
                           buf->value, buf->length);
    return m;
}


static void netgss_free_oid(long tag, gss_OID buf) {
    if (tag == 0) {
        /* OIDs from the provider are to be considered as read-only */
    } else {
        stat_free(buf->elements);
        stat_free(buf);
    }
}


static gss_OID netgss_alloc_oid(void) {
    return (gss_OID) stat_alloc(sizeof(gss_OID_desc));
}


static gss_OID netgss_copy_oid(gss_OID buf) {
    gss_OID out;
    out = netgss_alloc_oid();
    out->length = buf->length;
    out->elements = stat_alloc(buf->length);
    memcpy(out->elements, buf->elements, buf->length);
    return out;
}


CAMLprim value netgss_oid_of_string(value s) {
    gss_OID buf;
    buf = netgss_alloc_oid();
    buf->length = caml_string_length(s);
    buf->elements = stat_alloc(buf->length);
    memcpy(buf->elements, String_val(s), buf->length);
    return twrap_gss_OID(1, buf);
}


CAMLprim value netgss_string_of_oid(value b) {
    gss_OID buf;
    value s;
    buf = unwrap_gss_OID(b);
    s = caml_alloc_string(buf->length);
    memcpy(String_val(s), buf->elements, buf->length);
    return s;
}


static void netgss_free_oid_set(long tag, gss_OID_set set) {
    if (tag == 0) {
        OM_uint32 major, minor;
        major = gss_release_oid_set(&minor, &set);
    } else {
        size_t k;
        for (k=0; k < set->count; k++) {
            netgss_free_oid(1, set->elements+k);
        }
        stat_free(set->elements);
        stat_free(set);
    }
}


static gss_OID_set netgss_alloc_oid_set(void) {
    return (gss_OID_set) stat_alloc(sizeof(gss_OID_set_desc));
}


CAMLprim value netgss_array_of_oid_set(value varg) {
    CAMLparam1(varg);
    CAMLlocal2(v1, v2);
    gss_OID_set set;
    size_t k;
    set = unwrap_gss_OID_set(varg);
    /* no other way than to always copy the members */
    v1 = caml_alloc(set->count, 0);
    for (k=0; k<set->count; k++) {
        v2 = twrap_gss_OID(1, netgss_copy_oid(set->elements+k));
        Store_field(v1, k, v2);
    }
    CAMLreturn(v1);
}


CAMLprim value netgss_oid_set_of_array(value varg) {
    gss_OID_set set;
    gss_OID buf;
    gss_OID *p;
    size_t k;
    value v1;
    set = netgss_alloc_oid_set();
    set->count = Wosize_val(varg);
    set->elements = stat_alloc(sizeof(gss_OID) * set->count);
    for (k=0; k<set->count; k++) {
        v1 = Field(varg, k);
        buf = unwrap_gss_OID(v1);
        p = &(set->elements) + k;
        *p = netgss_copy_oid(buf);
    }
    return twrap_gss_OID_set(1, set);
}


static void netgss_free_cred_id(long tag, gss_cred_id_t x) {
    OM_uint32 major, minor;
    major = gss_release_cred(&minor, &x);
}


static void netgss_free_ctx_id(long tag, gss_ctx_id_t x) {
    OM_uint32 major, minor;
    major = gss_delete_sec_context(&minor, &x, GSS_C_NO_BUFFER);
}


static void netgss_free_name(long tag, gss_name_t x) {
    OM_uint32 major, minor;
    major = gss_release_name(&minor, &x);
}


CAMLprim value netgss_no_cb(value dummy) {
    return wrap_gss_channel_bindings_t(GSS_C_NO_CHANNEL_BINDINGS);
}


CAMLprim value netgss_no_ctx(value dummy) {
    return wrap_gss_ctx_id_t(GSS_C_NO_CONTEXT);
}


CAMLprim value netgss_no_cred(value dummy) {
    return wrap_gss_cred_id_t(GSS_C_NO_CREDENTIAL);
}


static void netgss_free_cb(gss_channel_bindings_t x) {
}
