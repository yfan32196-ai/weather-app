/**
 * city.c - 城市管理模块
 *
 * 功能：城市的增删改查操作
 * 知识点：结构体数组的增删改查、线性查找、memmove数组元素移动
 */

#include "weather_system.h"

/* 添加城市 */
int add_city(AppState *app, const char *name, const char *province,
             double lat, double lon) {
    if (app->city_count >= MAX_CITIES) {
        printf("❌ 城市数已达上限 (%d)\n", MAX_CITIES);
        return 0;
    }

    /* 检查名称是否重复 */
    if (find_city_by_name(app, name) != NULL) {
        printf("❌ 城市 '%s' 已存在\n", name);
        return 0;
    }

    City *c = &app->cities[app->city_count];
    c->id = app->city_count + 1;  /* 简单ID分配 */
    strcpy(c->name, name);
    strcpy(c->province, province);
    c->latitude = lat;
    c->longitude = lon;
    c->record_count = 0;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    c->created_at = (DateTime){
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec
    };

    app->city_count++;
    save_cities(app);
    printf("✅ 城市 '%s' 添加成功\n", name);
    return 1;
}

/* 更新城市信息 */
int update_city(AppState *app, int city_id, const char *name,
                const char *province, double lat, double lon) {
    City *c = find_city_by_id(app, city_id);
    if (c == NULL) {
        printf("❌ 城市ID %d 不存在\n", city_id);
        return 0;
    }

    /* 如果修改了名称，检查是否与其他城市重名 */
    if (strcmp(c->name, name) != 0 && find_city_by_name(app, name) != NULL) {
        printf("❌ 城市名称 '%s' 已被使用\n", name);
        return 0;
    }

    strcpy(c->name, name);
    strcpy(c->province, province);
    c->latitude = lat;
    c->longitude = lon;

    save_cities(app);
    printf("✅ 城市 '%s' 更新成功\n", name);
    return 1;
}

/* 删除城市（同时删除关联的气象记录） */
int delete_city(AppState *app, int city_id) {
    City *c = find_city_by_id(app, city_id);
    if (c == NULL) {
        printf("❌ 城市ID %d 不存在\n", city_id);
        return 0;
    }

    /* 删除该城市的所有气象记录 */
    int i, removed = 0;
    for (i = app->record_count - 1; i >= 0; i--) {
        if (app->records[i].city_id == city_id) {
            /* 将后续元素前移一位 */
            if (i < app->record_count - 1) {
                memmove(&app->records[i], &app->records[i + 1],
                        (app->record_count - i - 1) * sizeof(WeatherRecord));
            }
            app->record_count--;
            removed++;
        }
    }

    /* 从城市数组中删除该城市 */
    int idx = (int)(c - app->cities);  /* 计算索引 */
    if (idx < app->city_count - 1) {
        memmove(&app->cities[idx], &app->cities[idx + 1],
                (app->city_count - idx - 1) * sizeof(City));
    }
    app->city_count--;

    save_cities(app);
    save_records(app);

    printf("✅ 城市 '%s' 已删除，同时删除 %d 条关联记录\n", c->name, removed);
    return 1;
}

/* 按ID查找城市（线性查找） */
City *find_city_by_id(const AppState *app, int city_id) {
    int i;
    for (i = 0; i < app->city_count; i++) {
        if (app->cities[i].id == city_id) {
            return (City *)&app->cities[i];
        }
    }
    return NULL;
}

/* 按名称查找城市（线性查找 + 字符串比较） */
City *find_city_by_name(const AppState *app, const char *name) {
    int i;
    for (i = 0; i < app->city_count; i++) {
        if (strcmp(app->cities[i].name, name) == 0) {
            return (City *)&app->cities[i];
        }
    }
    return NULL;
}

/* 列出所有城市 */
void list_cities(const AppState *app) {
    if (app->city_count == 0) {
        printf("\n📋 暂无城市数据，请先添加城市\n");
        return;
    }

    printf("\n┌──────┬──────────────────┬────────────┬──────────┬───────────┬────────┐\n");
    printf("│  ID  │ 城市名称         │ 省份       │ 纬度     │ 经度      │ 记录数 │\n");
    printf("├──────┼──────────────────┼────────────┼──────────┼───────────┼────────┤\n");

    int i;
    for (i = 0; i < app->city_count; i++) {
        City *c = (City *)&app->cities[i];
        printf("│ %-4d │ %-16s │ %-10s │ %8.4f │ %9.4f │ %6d │\n",
               c->id, c->name, c->province,
               c->latitude, c->longitude, c->record_count);
    }
    printf("└──────┴──────────────────┴────────────┴──────────┴───────────┴────────┘\n");
    printf("共 %d 个城市\n", app->city_count);
}

/* 更新城市关联的记录计数 */
void update_city_record_count(AppState *app, int city_id) {
    City *c = find_city_by_id(app, city_id);
    if (c == NULL) return;
    c->record_count = count_records_by_city(app, city_id);
}
