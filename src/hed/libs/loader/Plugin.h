#ifndef __ARC_PLUGIN_H__
#define __ARC_PLUGIN_H__

typedef struct {
    int version;
    void *(*init)(void);
} mcc_descriptor;

#endif /* __ARC_PLUGIN_H__ */
