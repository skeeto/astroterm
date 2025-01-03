#ifndef VERSION_H
#define VERSION_H

// These should be defined via Meson during build time

#ifndef PROJ_NAME
#define PROJ_NAME "[unknown name]"
#endif

#ifndef PROJ_VERSION
#define PROJ_VERSION "[unknown version]"
#endif

#ifndef PROJ_HASH
#define PROJ_HASH "[unknown hash]"
#endif

#endif // VERSION_H
