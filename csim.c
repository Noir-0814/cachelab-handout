#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

int hit_count = 0, miss_count = 0, eviction_count = 0;
int verbose = 0;
char *trace;

typedef struct cache_block
{
    unsigned int valid;      // 有效位
    unsigned int tag;        // 标记位
    unsigned int time_stamp; // 距离上次引用时间
} Cache_block;

typedef struct cache
{
    int S;               // 组数
    int E;               // 每组几行
    int B;               // 每块大小
    Cache_block **block; //
} Cache;

Cache *cache;

// 初始化cache
void init_cache(int s, int E, int b)
{
    int S = 1 << s;
    int B = 1 << b;
    cache = (Cache *)malloc(sizeof(Cache));
    cache->S = S;
    cache->E = E;
    cache->B = B;
    cache->block = (Cache_block **)malloc(sizeof(Cache_block *) * S);
    for (int i = 0; i < S; i++)
    {
        cache->block[i] = (Cache_block *)malloc(sizeof(Cache_block) * E);
        for (int j = 0; j < E; j++)
        {
            cache->block[i][j].valid = 0;
            cache->block[i][j].tag = 0;
            cache->block[i][j].time_stamp = 0;
        }
    }
}
// free所有malloc的空间并将指针置空
void free_cache()
{
    for (int i = 0; i < cache->S; i++)
    {
        free(cache->block[i]);
        cache->block[i] = NULL;
    }
    free(cache->block);
    cache->block = NULL;
    free(cache);
    cache = NULL;
}
// 判断是否全为valid
int is_full(unsigned int opt_set, int *index)
{
    for (int j = 0; j < cache->E; j++)
    {
        if (!cache->block[opt_set][j].valid)
        {
            *index = j;
            return 0;
        }
    }
    return 1;
}

int findLRU(unsigned int opt_set)
{
    int max = cache->block[opt_set][0].time_stamp;
    int max_index = 0;
    for (int index = 1; index < cache->E; index++)
    {
        if (cache->block[opt_set][index].time_stamp > max)
        {
            max = cache->block[opt_set][index].time_stamp;
            max_index = index;
        }
    }
    return max_index;
}

void update(unsigned int opt_set)
{
    for (int j = 0; j < cache->E; j++)
    {
        if (cache->block[opt_set][j].valid)
        {
            cache->block[opt_set][j].time_stamp++;
        }
    }
}

void process_cache(unsigned int opt_tag, unsigned int opt_set)
{
    for (int j = 0; j < cache->E; j++)
    {
        if (cache->block[opt_set][j].valid && cache->block[opt_set][j].tag == opt_tag)
        {
            update(opt_set);
            hit_count++;
            cache->block[opt_set][j].time_stamp = 0;
            return;
        }
    }
    int index = 0;
    if (!is_full(opt_set, &index))
    {
        update(opt_set);
        cache->block[opt_set][index].valid = 1;
        cache->block[opt_set][index].tag = opt_tag;
        cache->block[opt_set][index].time_stamp = 0;
        miss_count++;
        return;
    }
    else
    {
        update(opt_set);
        index = findLRU(opt_set);
        cache->block[opt_set][index].tag = opt_tag;
        cache->block[opt_set][index].time_stamp = 0;
        miss_count++;
        eviction_count++;
        return;
    }
}
// 获取trace文件并执行操作
void process_trace(int s, int E, int b)
{
    FILE *file;
    file = fopen(trace, "r");
    if (file == NULL)
    {
        exit(-1);
    }

    char opt;
    unsigned int addr;
    int size;

    while (fscanf(file, " %c %x,%d", &opt, &addr, &size) > 0)
    {
        int t = (sizeof(unsigned int) * 8) - (s + b);
        unsigned int opt_tag = addr >> (s + b);
        unsigned int opt_set = (addr << t) >> (t + b);
        /*
        input: [space]operation address,size
        L: data load
        S: data store
        M: data modify
        */
        switch (opt)
        {
        case 'L':
            process_cache(opt_tag, opt_set);
            break;
        case 'S':
            process_cache(opt_tag, opt_set);
            break;
        case 'M':
            process_cache(opt_tag, opt_set);
            process_cache(opt_tag, opt_set);
            break;
        default:
            break;
        }
    }
    fclose(file);
}

void print_help()
{
    fprintf(stderr, "Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    return;
}

int main(int argc, char **argv)
{
    char opt;
    int s, E, b;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            print_help();
            exit(0);
        case 'v':
            verbose = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            trace = optarg;
            break;
        default:
            print_help();
            exit(-1);
        }
    }
    init_cache(s, E, b);
    process_trace(s, E, b);
    free_cache();
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
