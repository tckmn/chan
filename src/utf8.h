#ifndef __UTF8_H__
#define __UTF8_H__

#define bytelen(x) ((x) < 0xc0 ? 1 : \
                    (x) < 0xe0 ? 2 : \
                    (x) < 0xf0 ? 3 : 4)

#endif
