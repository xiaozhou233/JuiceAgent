#ifndef JUICEAGENT_H
#define JUICEAGENT_H

#define INJECT_PATH_MAX 4096

typedef struct InjectParams{
    char JuiceLoaderJarPath[INJECT_PATH_MAX];
    char ConfigDir[INJECT_PATH_MAX];
} InjectParams;

#endif // JUICEAGENT_H