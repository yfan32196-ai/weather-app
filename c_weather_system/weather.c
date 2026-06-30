/**
 * weather.c - 气象记录管理模块
 *
 * 功能：气象记录的增删改查、排序、筛选、分页
 * 知识点：结构体数组操作、排序算法（冒泡排序）、
 *         条件过滤、分页、枚举类型在条件匹配中的使用
 */

#include "weather_system.h"

/* 添加气象记录 */
int add_record(AppState *app, int city_id, DateTime record_dt,
               double temp, double feels_like, int humidity,
               double pressure, double wind_speed, WindDirection dir,
               double gust, WeatherCondition weather,
               const char *desc, DataSource source) {
    if (app->record_count >= MAX_RECORDS) {
        printf("❌ 记录数已达上限 (%d)\n", MAX_RECORDS);
        return 0;
    }

    City *city = find_city_by_id(app, city_id);
    if (city == NULL) {
        printf("❌ 城市ID %d 不存在\n", city_id);
        return 0;
    }

    WeatherRecord *r = &app->records[app->record_count];
    r->id = app->next_record_id++;
    r->city_id = city_id;
    r->record_time = record_dt;
    r->temperature = temp;
    r->feels_like = feels_like;
    r->humidity = humidity;
    r->pressure = pressure;
    r->wind_speed = wind_speed;
    r->wind_dir = dir;
    r->wind_gust = gust;
    r->weather = weather;
    if (desc != NULL) {
        strcpy(r->weather_desc, desc);
    } else {
        r->weather_desc[0] = '\0';
    }
    r->visibility = 10000;
    r->cloudiness = 0;
    r->rain_1h = (weather == WEATHER_RAIN || weather == WEATHER_DRIZZLE) ? 1.0 : 0.0;
    r->snow_1h = (weather == WEATHER_SNOW) ? 1.0 : 0.0;
    r->source = source;
    r->source_detail.operator_name[0] = '\0';
    r->notes[0] = '\0';

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    r->created_at = (DateTime){
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec
    };

    if (app->current_user) {
        r->created_by = app->current_user->id;
    } else {
        r->created_by = 0;
    }

    app->record_count++;
    update_city_record_count(app, city_id);
    save_records(app);
    save_cities(app);

    printf("✅ 气象记录添加成功 (ID: %d, 城市: %s)\n", r->id, city->name);
    return 1;
}

/* 更新气象记录 */
int update_record(AppState *app, int record_id, const WeatherRecord *new_data) {
    WeatherRecord *r = find_record_by_id(app, record_id);
    if (r == NULL) {
        printf("❌ 记录ID %d 不存在\n", record_id);
        return 0;
    }

    /* 如果城市变了，需要更新两个城市的计数 */
    int old_city_id = r->city_id;

    r->city_id         = new_data->city_id;
    r->record_time     = new_data->record_time;
    r->temperature     = new_data->temperature;
    r->feels_like      = new_data->feels_like;
    r->humidity        = new_data->humidity;
    r->pressure        = new_data->pressure;
    r->wind_speed      = new_data->wind_speed;
    r->wind_dir        = new_data->wind_dir;
    r->wind_gust       = new_data->wind_gust;
    r->weather         = new_data->weather;
    strcpy(r->weather_desc, new_data->weather_desc);
    r->visibility      = new_data->visibility;
    r->cloudiness      = new_data->cloudiness;
    r->rain_1h         = new_data->rain_1h;
    r->snow_1h         = new_data->snow_1h;
    r->source          = new_data->source;
    strcpy(r->notes, new_data->notes);

    if (old_city_id != r->city_id) {
        update_city_record_count(app, old_city_id);
    }
    update_city_record_count(app, r->city_id);

    save_records(app);
    save_cities(app);
    printf("✅ 记录 #%d 更新成功\n", record_id);
    return 1;
}

/* 删除气象记录 */
int delete_record(AppState *app, int record_id) {
    WeatherRecord *r = find_record_by_id(app, record_id);
    if (r == NULL) {
        printf("❌ 记录ID %d 不存在\n", record_id);
        return 0;
    }

    int city_id = r->city_id;
    int idx = (int)(r - app->records);

    /* 将后续元素前移 */
    if (idx < app->record_count - 1) {
        memmove(&app->records[idx], &app->records[idx + 1],
                (app->record_count - idx - 1) * sizeof(WeatherRecord));
    }
    app->record_count--;

    update_city_record_count(app, city_id);
    save_records(app);
    save_cities(app);
    printf("✅ 记录 #%d 已删除\n", record_id);
    return 1;
}

/* 按ID查找记录 */
WeatherRecord *find_record_by_id(const AppState *app, int record_id) {
    int i;
    for (i = 0; i < app->record_count; i++) {
        if (app->records[i].id == record_id) {
            return (WeatherRecord *)&app->records[i];
        }
    }
    return NULL;
}

/* 统计某城市的记录数 */
int count_records_by_city(const AppState *app, int city_id) {
    int count = 0, i;
    for (i = 0; i < app->record_count; i++) {
        if (app->records[i].city_id == city_id) count++;
    }
    return count;
}

/* ---- 排序相关 ---- */

/* 比较函数：用于冒泡排序 */
static int record_compare(const WeatherRecord *a, const WeatherRecord *b, SortOrder sort) {
    switch (sort) {
        case SORT_BY_TIME_DESC:
            return (a->record_time.year  < b->record_time.year)  ||
                   (a->record_time.year  == b->record_time.year &&
                    a->record_time.month < b->record_time.month);
        case SORT_BY_TIME_ASC:
            return (a->record_time.year  > b->record_time.year)  ||
                   (a->record_time.year  == b->record_time.year &&
                    a->record_time.month > b->record_time.month);
        case SORT_BY_TEMP_DESC:
            return a->temperature < b->temperature;
        case SORT_BY_TEMP_ASC:
            return a->temperature > b->temperature;
        case SORT_BY_CITY:
            return a->city_id > b->city_id;
        default:
            return 0;
    }
}

/* 冒泡排序（适合中小规模数据，体现算法思想） */
static void bubble_sort(WeatherRecord *arr, int count, SortOrder sort) {
    int i, j;
    for (i = 0; i < count - 1; i++) {
        int swapped = 0;
        for (j = 0; j < count - 1 - i; j++) {
            if (record_compare(&arr[j], &arr[j + 1], sort)) {
                WeatherRecord temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
                swapped = 1;
            }
        }
        if (!swapped) break;  /* 优化：如果本轮无交换，说明已有序 */
    }
}

/* ---- 列表展示 ---- */

void list_records(const AppState *app, int city_id,
                  SortOrder sort, int page, int page_size) {
    /* 先收集符合条件的记录（复制到一个临时数组） */
    WeatherRecord filtered[MAX_RECORDS];
    int filtered_count = 0;
    int i;

    for (i = 0; i < app->record_count; i++) {
        if (city_id == 0 || app->records[i].city_id == city_id) {
            filtered[filtered_count++] = app->records[i];
        }
    }

    if (filtered_count == 0) {
        printf("\n📋 暂无气象记录\n");
        return;
    }

    /* 排序 */
    bubble_sort(filtered, filtered_count, sort);

    /* 分页计算 */
    int total_pages = (filtered_count + page_size - 1) / page_size;
    if (page < 1) page = 1;
    if (page > total_pages) page = total_pages;
    int start = (page - 1) * page_size;
    int end = start + page_size;
    if (end > filtered_count) end = filtered_count;

    /* 输出表头 */
    printf("\n┌──────┬──────────┬──────────────────┬────────┬──────┬──────┬──────────┬──────────┬──────────┐\n");
    printf("│  ID  │ 城市     │ 时间             │ 温度   │ 湿度 │ 风速 │ 天气     │ 风向     │ 来源     │\n");
    printf("├──────┼──────────┼──────────────────┼────────┼──────┼──────┼──────────┼──────────┼──────────┤\n");

    for (i = start; i < end; i++) {
        WeatherRecord *r = &filtered[i];
        City *city = find_city_by_id(app, r->city_id);
        const char *city_name = city ? city->name : "未知";

        printf("│ %4d │ %-8s │ %04d-%02d-%02d %02d:%02d  │ %5.1f°C│ %3d%% │%4.1fm│ %-8s │ %-8s │ %-8s │\n",
               r->id, city_name,
               r->record_time.year, r->record_time.month, r->record_time.day,
               r->record_time.hour, r->record_time.minute,
               r->temperature, r->humidity, r->wind_speed,
               weather_to_string(r->weather),
               wind_to_string(r->wind_dir),
               source_to_string(r->source));
    }

    printf("└──────┴──────────┴──────────────────┴────────┴──────┴──────┴──────────┴──────────┴──────────┘\n");
    printf("第 %d/%d 页，共 %d 条记录\n", page, total_pages, filtered_count);
}

/* 条件筛选 */
void filter_records(const AppState *app, int city_id,
                    const DateTime *from, const DateTime *to,
                    DataSource source_filter) {
    int count = 0;
    int i;

    printf("\n🔍 筛选结果：\n");
    printf("┌──────┬──────────┬──────────────────┬────────┬──────┬──────────┬──────────┐\n");
    printf("│  ID  │ 城市     │ 时间             │ 温度   │ 湿度 │ 天气     │ 来源     │\n");
    printf("├──────┼──────────┼──────────────────┼────────┼──────┼──────────┼──────────┤\n");

    for (i = 0; i < app->record_count; i++) {
        const WeatherRecord *r = &app->records[i];

        /* 城市过滤 */
        if (city_id > 0 && r->city_id != city_id) continue;

        /* 时间范围过滤 */
        if (from != NULL) {
            if (r->record_time.year < from->year) continue;
            if (r->record_time.year == from->year &&
                r->record_time.month < from->month) continue;
            if (r->record_time.year == from->year &&
                r->record_time.month == from->month &&
                r->record_time.day < from->day) continue;
        }
        if (to != NULL) {
            if (r->record_time.year > to->year) continue;
            if (r->record_time.year == to->year &&
                r->record_time.month > to->month) continue;
            if (r->record_time.year == to->year &&
                r->record_time.month == to->month &&
                r->record_time.day > to->day) continue;
        }

        /* 数据来源过滤 */
        if (source_filter >= 0 && r->source != source_filter) continue;

        City *city = find_city_by_id(app, r->city_id);
        const char *city_name = city ? city->name : "未知";

        printf("│ %4d │ %-8s │ %04d-%02d-%02d %02d:%02d  │ %5.1f°C│ %3d%% │ %-8s │ %-8s │\n",
               r->id, city_name,
               r->record_time.year, r->record_time.month, r->record_time.day,
               r->record_time.hour, r->record_time.minute,
               r->temperature, r->humidity,
               weather_to_string(r->weather),
               source_to_string(r->source));
        count++;
    }

    printf("└──────┴──────────┴──────────────────┴────────┴──────┴──────────┴──────────┘\n");
    printf("共找到 %d 条匹配记录\n", count);
}
