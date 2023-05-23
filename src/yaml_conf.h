#ifndef YAML_CONF
#define YAML_CONF

#include <cyaml/cyaml.h>

/*

Defining IMAP CONFIG FILE
USING A TAML C LIBRARY FROM: https://github.com/tlsa/libcyamls

*/

struct imap_config {
    char *imap_server;
    char *imap_username;
    int  *imap_port;
    int  *use_ssl;
    char *imap_password;

};

/* CYAML mapping schema fields array for the top level mapping. */
static const cyaml_schema_field_t top_mapping_schema[] = {
	CYAML_FIELD_STRING_PTR(
		"imap_server", CYAML_FLAG_POINTER, struct imap_config, imap_server, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR(
        "imap_username", CYAML_FLAG_POINTER, struct imap_config, imap_username, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR(
        "imap_password", CYAML_FLAG_POINTER, struct imap_config, imap_password, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT_PTR(
        "imap_port", CYAML_FLAG_POINTER, struct imap_config, imap_port),
    CYAML_FIELD_INT_PTR(
        "use_ssl", CYAML_FLAG_POINTER, struct imap_config, use_ssl),
    CYAML_FIELD_END
};

/* CYAML value schema for the top level mapping. */
static const cyaml_schema_value_t top_schema = {
	CYAML_VALUE_MAPPING(
		CYAML_FLAG_POINTER, struct imap_config, top_mapping_schema),
};

/* Create our CYAML configuration. */
static const cyaml_config_t config = {
	.log_fn = cyaml_log,            /* Use the default logging function. */
	.mem_fn = cyaml_mem,            /* Use the default memory allocator. */
	.log_level = CYAML_LOG_ERROR, /* Logging errors and warnings only. */
};

#endif