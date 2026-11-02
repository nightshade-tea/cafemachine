#include "serialize.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* machyara_scan serialized format
 *
 *  scan header          rule header          rules ...
 * +-----------+-----------------------------+---------+
 * | uint64_t  | uint64_t uint8_t char[]     |   ...   |
 * | serialsz  | serialsz matched identifier |         |
 * +-----------+-----------------------------+---------+
 */

/* -------------------------------------------------------------------------- */
/* constructor/destructor --------------------------------------------------- */

struct machyara_scan *machyara_scan_create(void)
{
    struct machyara_scan *s;

    if (!(s = calloc(1, sizeof *s))) {
        return NULL;
    }

    s->serialsz = sizeof s->serialsz;

    return s;
}

void machyara_scan_destroy(struct machyara_scan *scan)
{
    struct machyara_rule *r;

    while ((r = scan->head)) {
        scan->head = r->next;
        free(r);
    }

    free(scan);
}

/* -------------------------------------------------------------------------- */
/* rule creation/appending -------------------------------------------------- */

static struct machyara_rule *rule_create(const char *identifier,
                                         uint8_t matched)
{
    struct machyara_rule *r;
    size_t fullsz;

    /* strlen + 1 to account for the null terminator */
    fullsz = sizeof *r + strlen(identifier) + 1;

    if (!(r = calloc(1, fullsz))) {
        return NULL;
    }

    r->serialsz = sizeof r->serialsz + sizeof r->matched + fullsz - sizeof *r;
    r->matched = matched;
    strcpy(r->identifier, identifier);

    return r;
}

int machyara_scan_add_rule(struct machyara_scan *scan, const char *identifier,
                           uint8_t matched)
{
    struct machyara_rule *r;

    if (!(r = rule_create(identifier, matched))) {
        return -ENOMEM;
    }

    if (!scan->head) {
        scan->head = scan->tail = r;
    } else {
        scan->tail = scan->tail->next = r;
    }

    scan->serialsz += r->serialsz;

    return 0;
}

/* -------------------------------------------------------------------------- */
/* serialization ------------------------------------------------------------ */

static size_t write_serialized_scan(void *dest, struct machyara_scan *src)
{
    memcpy(dest, &src->serialsz, sizeof src->serialsz);
    return sizeof src->serialsz;
}

/* caller: check if return value is equal to src->serialsz */
static size_t write_serialized_rule(void *dest, struct machyara_rule *src)
{
    char *buf;
    size_t off;

    buf = dest;
    off = 0;

    memcpy(buf + off, &src->serialsz, sizeof src->serialsz);
    off += sizeof src->serialsz;

    memcpy(buf + off, &src->matched, sizeof src->matched);
    off += sizeof src->matched;

    memcpy(buf + off, src->identifier, src->serialsz - off);

    return src->serialsz;
}

int machyara_serialize(void *dest, struct machyara_scan *src)
{
    struct machyara_rule *r;
    char *buf;
    size_t off;

    buf = dest;
    off = 0;

    off += write_serialized_scan(buf + off, src);
    r = src->head;

    while (r && off + r->serialsz <= src->serialsz) {
        off += write_serialized_rule(buf + off, r);
        r = r->next;
    }

    return 0;
}

int machyara_deserialize(struct machyara_scan *dest, void *src)
{
    uint64_t bufsz;
    char *buf;
    size_t off;

    uint64_t serialsz;
    uint8_t matched;
    const char *identifier;
    size_t next_off;
    int err;

    buf = src;
    off = 0;

    memcpy(&bufsz, buf + off, sizeof bufsz);
    off += sizeof bufsz;

    while (off < bufsz) {
        memcpy(&serialsz, buf + off, sizeof serialsz);

        next_off = off + serialsz;
        off += sizeof serialsz;

        memcpy(&matched, buf + off, sizeof matched);
        off += sizeof matched;

        identifier = buf + off;

        if ((err = machyara_scan_add_rule(dest, identifier, matched))) {
            return err;
        }

        off = next_off;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
