/**
 * 气象数据分析引擎 (C 语言)
 *
 * 编译: gcc -o weather_analysis weather_analysis.c -lm
 * 用法: ./weather_analysis <input.csv
 *
 * 输入: CSV 格式的气象数据（从 Python 后端传入）
 *       列: city_name,record_time,temperature,feels_like,humidity,
 *            pressure,wind_speed,wind_direction,weather_main,weather_desc
 *
 * 输出: JSON 格式的统计分析结果
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define MAX_LINE    8192
#define MAX_FIELDS  32
#define MAX_RECORDS 10000
#define MAX_CITIES  50

/* ---------- 数据结构 ---------- */

typedef struct {
    char city_name[128];
    char record_time[64];
    double temperature;
    double feels_like;
    int    humidity;
    double pressure;
    double wind_speed;
    char   wind_direction[8];
    char   weather_main[32];
    char   weather_desc[64];
} WeatherRecord;

typedef struct {
    char   name[128];
    int    count;
    double temp_min, temp_max, temp_sum;
    double humid_sum;
    double wind_sum, wind_max;
    int    wind_dir_count[8];  /* N, NE, E, SE, S, SW, W, NW */
    int    weather_count[7];   /* Clear, Clouds, Rain, Drizzle, Snow, Thunderstorm, Fog/Haze */
} CityStats;

/* ---------- 全局变量 ---------- */

static WeatherRecord records[MAX_RECORDS];
static int record_count = 0;

static CityStats city_stats[MAX_CITIES];
static int city_count = 0;

/* 风向对应关系 */
static const char *wind_dirs[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
static const char *weather_types[] = {"Clear", "Clouds", "Rain", "Drizzle", "Snow", "Thunderstorm", "Fog"};

/* ---------- 工具函数 ---------- */

/* 安全的 strcpy */
static void safe_copy(char *dest, const char *src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/* 解析 CSV 行：按逗号分割，处理引号包裹的字段 */
static int parse_csv_line(char *line, char fields[][256], int max_fields) {
    int field_idx = 0;
    int char_idx  = 0;
    int in_quotes = 0;
    char *p = line;

    while (*p && field_idx < max_fields) {
        /* 跳过行首空格 */
        if (!in_quotes) {
            while (*p == ' ') p++;
        }

        if (*p == '"') {
            in_quotes = 1;
            p++;
            char_idx = 0;
            continue;
        }

        if (in_quotes) {
            if (*p == '"') {
                /* 检查是否是转义引号 "" */
                if (*(p + 1) == '"') {
                    fields[field_idx][char_idx++] = '"';
                    p += 2;
                    continue;
                }
                /* 结束引号 */
                fields[field_idx][char_idx] = '\0';
                in_quotes = 0;
                field_idx++;
                p++;
                /* 跳过引号后的逗号和空格 */
                while (*p == ' ' || *p == ',') p++;
                char_idx = 0;
                continue;
            }
        } else {
            if (*p == ',') {
                fields[field_idx][char_idx] = '\0';
                field_idx++;
                p++;
                char_idx = 0;
                continue;
            }
        }

        /* 跳过行尾的 \r\n */
        if (*p == '\r' || *p == '\n') {
            if (char_idx > 0) {
                fields[field_idx][char_idx] = '\0';
                field_idx++;
            }
            break;
        }

        if (char_idx < 255) {
            fields[field_idx][char_idx++] = *p;
        }
        p++;
    }

    /* 最后一个字段（如果没有逗号结束） */
    if (char_idx > 0 && field_idx < max_fields && !in_quotes) {
        fields[field_idx][char_idx] = '\0';
        field_idx++;
    }

    return field_idx;
}

/* 安全解析 double */
static double parse_double(const char *s) {
    if (s == NULL || s[0] == '\0') return 0.0;
    char *end;
    double val = strtod(s, &end);
    if (end == s) return 0.0;
    return val;
}

/* 安全解析 int */
static int parse_int(const char *s) {
    if (s == NULL || s[0] == '\0') return 0;
    return (int)strtol(s, NULL, 10);
}

/* 查找或创建城市统计 */
static CityStats *find_or_create_city(const char *name) {
    int i;
    for (i = 0; i < city_count; i++) {
        if (strcmp(city_stats[i].name, name) == 0) {
            return &city_stats[i];
        }
    }
    /* 新建 */
    if (city_count >= MAX_CITIES) return NULL;
    CityStats *cs = &city_stats[city_count];
    safe_copy(cs->name, name, sizeof(cs->name));
    cs->count      = 0;
    cs->temp_min   = DBL_MAX;
    cs->temp_max   = -DBL_MAX;
    cs->temp_sum   = 0.0;
    cs->humid_sum  = 0.0;
    cs->wind_sum   = 0.0;
    cs->wind_max   = 0.0;
    for (i = 0; i < 8; i++)  cs->wind_dir_count[i] = 0;
    for (i = 0; i < 7; i++)  cs->weather_count[i]  = 0;
    city_count++;
    return cs;
}

/* 获取风向索引 */
static int get_wind_dir_index(const char *dir) {
    int i;
    for (i = 0; i < 8; i++) {
        if (strcmp(dir, wind_dirs[i]) == 0) return i;
    }
    return -1;
}

/* 获取天气类型索引 */
static int get_weather_index(const char *w) {
    int i;
    for (i = 0; i < 7; i++) {
        if (strcmp(w, weather_types[i]) == 0) return i;
    }
    /* Fog/Haze/Mist 归到 Fog 类 (索引6) */
    return 6;
}

/* ---------- 分析函数 ---------- */

/* 读取并解析输入 */
static int read_input(void) {
    char line[MAX_LINE];

    /* 跳过表头行 */
    if (fgets(line, MAX_LINE, stdin) == NULL) {
        fprintf(stderr, "错误: 没有输入数据\n");
        return 0;
    }

    /* 逐行读取 */
    while (fgets(line, MAX_LINE, stdin) != NULL && record_count < MAX_RECORDS) {
        char fields[MAX_FIELDS][256];
        int n = parse_csv_line(line, fields, MAX_FIELDS);

        if (n < 6) continue;  /* 至少要有 city_name, time, temp, humidity, wind_speed */

        WeatherRecord *r = &records[record_count];

        /* 字段映射 (0-based):
           0:city_name  1:record_time  2:temperature  3:feels_like
           4:humidity   5:pressure      6:wind_speed   7:wind_direction
           8:weather_main  9:weather_desc */
        safe_copy(r->city_name,     n > 0 ? fields[0] : "", sizeof(r->city_name));
        safe_copy(r->record_time,   n > 1 ? fields[1] : "", sizeof(r->record_time));
        r->temperature  = parse_double(n > 2 ? fields[2] : "0");
        r->feels_like   = parse_double(n > 3 ? fields[3] : "0");
        r->humidity     = parse_int(n > 4 ? fields[4] : "0");
        r->pressure     = parse_double(n > 5 ? fields[5] : "0");
        r->wind_speed   = parse_double(n > 6 ? fields[6] : "0");
        safe_copy(r->wind_direction, n > 7 ? fields[7] : "", sizeof(r->wind_direction));
        safe_copy(r->weather_main,   n > 8 ? fields[8] : "", sizeof(r->weather_main));
        safe_copy(r->weather_desc,   n > 9 ? fields[9] : "", sizeof(r->weather_desc));

        record_count++;
    }

    return record_count;
}

/* 执行统计分析 */
static void analyze(void) {
    int i;
    for (i = 0; i < record_count; i++) {
        WeatherRecord *r = &records[i];
        CityStats *cs = find_or_create_city(r->city_name);
        if (cs == NULL) continue;

        cs->count++;

        /* 温度统计 */
        if (r->temperature > 0 || r->temperature < 0) {  /* 有效温度 */
            if (r->temperature < cs->temp_min) cs->temp_min = r->temperature;
            if (r->temperature > cs->temp_max) cs->temp_max = r->temperature;
            cs->temp_sum += r->temperature;
        }

        /* 湿度 */
        cs->humid_sum += r->humidity;

        /* 风速 */
        if (r->wind_speed > 0) {
            cs->wind_sum += r->wind_speed;
            if (r->wind_speed > cs->wind_max) cs->wind_max = r->wind_speed;
        }

        /* 风向计数 */
        int wdi = get_wind_dir_index(r->wind_direction);
        if (wdi >= 0) cs->wind_dir_count[wdi]++;

        /* 天气类型计数 */
        int wi = get_weather_index(r->weather_main);
        if (wi >= 0) cs->weather_count[wi]++;
    }
}

/* 找出主导风向 */
static const char *dominant_wind(const CityStats *cs) {
    int i, max_idx = 0, max_val = 0;
    for (i = 0; i < 8; i++) {
        if (cs->wind_dir_count[i] > max_val) {
            max_val = cs->wind_dir_count[i];
            max_idx = i;
        }
    }
    if (max_val == 0) return "--";
    return wind_dirs[max_idx];
}

/* 找出主要天气 */
static const char *dominant_weather(const CityStats *cs) {
    int i, max_idx = 0, max_val = 0;
    for (i = 0; i < 7; i++) {
        if (cs->weather_count[i] > max_val) {
            max_val = cs->weather_count[i];
            max_idx = i;
        }
    }
    if (max_val == 0) return "--";
    return weather_types[max_idx];
}

/* 极端天气检测 */
static void check_extreme(const CityStats *cs,
                          int *has_high_temp, int *has_strong_wind, int *has_storm) {
    *has_high_temp   = (cs->temp_max >= 35.0) ? 1 : 0;
    *has_strong_wind = (cs->wind_max >= 10.5) ? 1 : 0;
    *has_storm       = (cs->weather_count[2]  > cs->count / 3 ||   /* Rain > 1/3 */
                         cs->weather_count[5]  > 0) ? 1 : 0;        /* Thunderstorm */
}

/* ---------- 输出 ---------- */

/* 输出 JSON 格式的完整分析结果 */
static void output_json(void) {
    int i;
    int first_city = 1;

    printf("{\n");
    printf("  \"record_count\": %d,\n", record_count);
    printf("  \"city_count\": %d,\n", city_count);
    printf("  \"cities\": [\n");

    for (i = 0; i < city_count; i++) {
        CityStats *cs = &city_stats[i];
        double avg_temp   = cs->count > 0 ? cs->temp_sum  / cs->count : 0.0;
        double avg_humid  = cs->count > 0 ? cs->humid_sum / cs->count : 0.0;
        double avg_wind   = cs->count > 0 ? cs->wind_sum  / cs->count : 0.0;

        int high_temp, strong_wind, storm;
        check_extreme(cs, &high_temp, &strong_wind, &storm);

        if (!first_city) printf(",\n");
        first_city = 0;

        printf("    {\n");
        printf("      \"name\": \"%s\",\n", cs->name);
        printf("      \"count\": %d,\n", cs->count);
        printf("      \"temperature\": {\n");
        printf("        \"avg\": %.1f,\n", avg_temp);
        printf("        \"min\": %.1f,\n", cs->temp_min == DBL_MAX ? 0.0 : cs->temp_min);
        printf("        \"max\": %.1f\n", cs->temp_max == -DBL_MAX ? 0.0 : cs->temp_max);
        printf("      },\n");
        printf("      \"humidity\":     { \"avg\": %.1f },\n", avg_humid);
        printf("      \"wind\": {\n");
        printf("        \"avg_speed\": %.2f,\n", avg_wind);
        printf("        \"max_speed\": %.2f,\n", cs->wind_max);
        printf("        \"dominant_direction\": \"%s\"\n", dominant_wind(cs));
        printf("      },\n");
        printf("      \"dominant_weather\": \"%s\",\n", dominant_weather(cs));
        printf("      \"alerts\": {\n");
        printf("        \"high_temperature\": %s,\n", high_temp ? "true" : "false");
        printf("        \"strong_wind\": %s,\n",      strong_wind ? "true" : "false");
        printf("        \"storm_risk\": %s\n",         storm ? "true" : "false");
        printf("      }\n");
        printf("    }");
    }

    printf("\n  ]\n}\n");
}

/* ---------- 入口 ---------- */

int main(void) {
    if (read_input() == 0) {
        printf("{\"error\": \"无有效数据\"}\n");
        return 1;
    }

    analyze();
    output_json();

    return 0;
}
