#ifndef MACCHIATO_SERIALIZE_H
#define MACCHIATO_SERIALIZE_H

#include <stdint.h>

struct machyara_rule {
    uint64_t serialsz; /* serialized size of this rule */
    uint8_t matched;   /* zero -> not matched, not zero -> matched */
    struct machyara_rule *next;
    char identifier[]; /* rule name string */
};

struct machyara_scan {
    uint64_t serialsz; /* serialized size of rule chain (dest buffer size) */
    struct machyara_rule *head;
    struct machyara_rule *tail;
};

struct machyara_scan *machyara_scan_create(void);
void machyara_scan_destroy(struct machyara_scan *scan);

int machyara_scan_add_rule(struct machyara_scan *scan, const char *identifier,
                           uint8_t matched);

int machyara_serialize(void *dest, struct machyara_scan *src);

int machyara_deserialize(struct machyara_scan *dest, void *src);

#endif /* MACCHIATO_SERIALIZE_H */
