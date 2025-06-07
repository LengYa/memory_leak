#define _GNU_SOURCE

#include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <link.h>

// hook malloc and free
typedef void *(*malloc_t)(size_t size);
typedef void (*free_t)(void *ptr);

malloc_t real_malloc = NULL;
free_t real_free = NULL;

int flag_malloc = 1;
size_t LINE_N = 0;

void *TranslateToSymbol(void *addr);
void *malloc(size_t size);
void free(void *ptr);
int main()
{
    size_t size = 1024;

    LINE_N = __LINE__;
    void *p1 = malloc(size);

    LINE_N = __LINE__;
    void *p2 = malloc(size * 2);

    LINE_N = __LINE__;
    void *p3 = malloc(size * 3);

    free(p1);
    free(p2);

    return 0;
}

/**
 * @brief 自定义的malloc函数，用于在分配内存时记录分配信息
 *
 * 在调用实际的malloc函数分配内存之前，如果flag_malloc标志位为1，则记录分配的内存地址、调用者地址以及分配的大小到文件中。
 *
 * @param size 要分配的内存大小（以字节为单位）
 * @return 分配的内存地址指针，如果分配失败则返回NULL
 */
void *malloc(size_t size)
{
    //使用dlsym函数从动态链接库中获取真实的malloc函数地址
    if (!real_malloc)
    {
        real_malloc = (malloc_t)dlsym(RTLD_NEXT, "malloc");
    }

    void *ptr = NULL;

    if (flag_malloc)
    {
        flag_malloc = 0;

        ptr = real_malloc(size);

        void *caller = __builtin_return_address(0);     //使用__builtin_return_address(0)获取调用者地址是GCC特有的内建函数
        char buff[1024] = {0};

        snprintf(buff, 1024, "./%p.mem", ptr);

        FILE *fp = fopen(buff, "w");
        if (!fp)
        {
            free(ptr);
            return NULL;
        }
        fprintf(fp, "[+][%p] %p: %ld malloc, line: %d\n", TranslateToSymbol(caller), ptr, size, LINE_N);

        fflush(fp);

        flag_malloc = 1;
    }
    else
    {
        ptr = real_malloc(size);
    }

    return ptr;
}

/**
 * @brief 自定义的内存释放函数
 *
 * 该函数通过动态链接库函数 dlsym 获取标准库中的 free 函数指针，并使用该指针释放传入的内存指针。
 * 在释放内存之前，会尝试删除与内存指针对应的临时文件（如果文件存在），以避免重复释放内存。
 *
 * @param ptr 需要释放的内存指针
 */
void free(void *ptr)
{
    if (!real_free)
    {
        real_free = (free_t)dlsym(RTLD_NEXT, "free");
    }

    char buff[1024] = {0};
    snprintf(buff, 1024, "./%p.mem", ptr);

    if (unlink(buff) < 0)
    { // if file not exist, unlink will return -1
        printf("double free: %p\n", ptr);
        return;
    }

    real_free(ptr);
}

/**
 * @brief 将内存地址转换为符号地址
 *
 * 将传入的内存地址转换为相对于共享库起始地址的偏移量。
 *
 * @param addr 传入的内存地址
 * @return 转换后的符号地址，如果转换失败则返回NULL
 */
void *TranslateToSymbol(void *addr)
{
    Dl_info info;
    struct link_map *l = NULL;
    if (dladdr1(addr, &info, (void **)&l, RTLD_DL_LINKMAP))
    {
        return (void *)(addr - l->l_addr);
    }
    return NULL;
}
