#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <link.h>

// 重定义malloc和free

void *Malloc(size_t size, const char *file, int line, const char *func);
void Free(void *ptr, const char *file, int line, const char *func);
void *Malloc(size_t size, const char *file, int line, const char *func)
{
    void *ptr = malloc(size);
    char buff[1024] = {0};
    printf("%p\n", ptr);

    snprintf(buff, 1024, "./%p.mem", ptr);

    FILE *fp = fopen(buff, "w");
    if (!fp)
    {
        free(ptr);
        return NULL;
    }
    fprintf(fp, "[+][%s:%s:%d] %p: %ld malloc\n", file, func, line, ptr, size);

    fflush(fp);
    fclose(fp);

    return ptr;
}

void Free(void *ptr, const char *file, int line, const char *func)
{
    char buff[1024] = {0};

    free(ptr);
    snprintf(buff, 1024, "./%p.mem", ptr);

    if (unlink(buff) < 0)
    { // if file not exist, unlink will return -1
        printf("double free: %p\n", ptr);
        return;
    }
}


#define malloc(size) Malloc(size, __FILE__, __LINE__, __func__)
#define free(ptr) Free(ptr, __FILE__, __LINE__, __func__)

int main()
{
    size_t size = 1024;

    void *p1 = malloc(size);
    void *p2 = malloc(size * 2);
    void *p3 = malloc(size * 3);

    free(p1);
    free(p2);

    return 0;
}


