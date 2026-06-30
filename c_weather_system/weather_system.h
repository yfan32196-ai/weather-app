/**
 * 气象信息管理系统 - 头文件
 *
 * 常州大学 C 语言课程设计
 * 功能：气象数据的增删改查、统计分析、文件持久化存储
 * 知识点：结构体、枚举、共用体、数组、文件操作、模块化编程
 */

#ifndef WEATHER_SYSTEM_H
#define WEATHER_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

/* ============================================================
 * 宏定义 - 系统常量
 * ============================================================ */

#define MAX_CITIES     50      /* 最大城市数           */
#define MAX_RECORDS    5000    /* 最大气象记录数        */
#define MAX_USERS      20      /* 最大用户数           */
#define MAX_NAME_LEN   64      /* 名称最大长度          */
#define MAX_PWD_LEN    32      /* 密码最大长度          */
#define MAX_DESC_LEN   128     /* 描述最大长度          */
#define MAX_LINE_LEN   512     /* 输入缓冲最大长度       */
#define DATA_DIR       "data"  /* 数据文件目录          */

/* 数据文件路径 */
#define CITY_FILE      "data/cities.dat"
#define RECORD_FILE    "data/records.dat"
#define USER_FILE      "data/users.dat"
#define LOG_FILE       "data/system.log"

/* 导入导出支持的格式 */
#define EXPORT_FORMAT_CSV   1
#define EXPORT_FORMAT_TXT   2

/* ============================================================
 * 枚举类型
 * ============================================================ */

/* 用户角色 */
typedef enum {
    ROLE_ADMIN  = 0,  /* 管理员 - 全部权限      */
    ROLE_EDITOR = 1,  /* 编辑者 - 可增删改查     */
    ROLE_VIEWER = 2   /* 观察者 - 只能查看       */
} UserRole;

/* 天气状况 */
typedef enum {
    WEATHER_SUNNY       = 0,  /* 晴                  */
    WEATHER_CLOUDY      = 1,  /* 多云                */
    WEATHER_OVERCAST    = 2,  /* 阴                  */
    WEATHER_RAIN        = 3,  /* 雨                  */
    WEATHER_DRIZZLE     = 4,  /* 小雨/毛毛雨          */
    WEATHER_SNOW        = 5,  /* 雪                  */
    WEATHER_THUNDERSTORM= 6,  /* 雷暴                */
    WEATHER_FOG         = 7,  /* 雾                  */
    WEATHER_HAZE        = 8   /* 霾                  */
} WeatherCondition;

/* 数据来源 */
typedef enum {
    SOURCE_MANUAL       = 0,  /* 手动录入             */
    SOURCE_API          = 1,  /* API 获取             */
    SOURCE_IMPORT       = 2   /* 批量导入             */
} DataSource;

/* 风向 */
typedef enum {
    WIND_N  = 0,  /* 北      */
    WIND_NE = 1,  /* 东北    */
    WIND_E  = 2,  /* 东      */
    WIND_SE = 3,  /* 东南    */
    WIND_S  = 4,  /* 南      */
    WIND_SW = 5,  /* 西南    */
    WIND_W  = 6,  /* 西      */
    WIND_NW = 7   /* 西北    */
} WindDirection;

/* 排序方式 */
typedef enum {
    SORT_BY_TIME_DESC   = 0,  /* 按时间降序           */
    SORT_BY_TIME_ASC    = 1,  /* 按时间升序           */
    SORT_BY_TEMP_DESC   = 2,  /* 按温度降序           */
    SORT_BY_TEMP_ASC    = 3,  /* 按温度升序           */
    SORT_BY_CITY        = 4   /* 按城市名称           */
} SortOrder;

/* ============================================================
 * 共用体 - 不同数据来源的附加信息
 * ============================================================ */

typedef union {
    char operator_name[MAX_NAME_LEN];  /* 手动录入：操作员姓名    */
    char api_source[32];               /* API获取：来源标识       */
    char import_filename[MAX_NAME_LEN];/* 导入：原始文件名        */
} SourceDetail;

/* ============================================================
 * 结构体 - 核心数据模型
 * ============================================================ */

/* 日期时间 */
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} DateTime;

/* 用户 */
typedef struct {
    int    id;                  /* 用户ID（唯一）       */
    char   username[MAX_NAME_LEN];  /* 用户名               */
    char   password[MAX_PWD_LEN];   /* 密码（明文存储）      */
    UserRole role;              /* 角色（枚举）          */
    char   email[MAX_NAME_LEN]; /* 邮箱                  */
    int    is_active;           /* 是否激活：1激活 0禁用  */
    DateTime created_at;        /* 创建时间              */
} User;

/* 城市 */
typedef struct {
    int    id;                  /* 城市ID（唯一）       */
    char   name[MAX_NAME_LEN];  /* 城市名称              */
    char   province[MAX_NAME_LEN];  /* 省份               */
    double latitude;            /* 纬度                  */
    double longitude;           /* 经度                  */
    int    record_count;        /* 关联记录数            */
    DateTime created_at;        /* 创建时间              */
} City;

/* 气象记录 */
typedef struct {
    int    id;                  /* 记录ID（唯一）       */
    int    city_id;             /* 关联城市ID           */
    DateTime record_time;       /* 记录时间              */
    double temperature;         /* 温度（摄氏度）        */
    double feels_like;          /* 体感温度              */
    int    humidity;            /* 湿度（0-100%）        */
    double pressure;            /* 气压（hPa）           */
    double wind_speed;          /* 风速（m/s）           */
    WindDirection wind_dir;     /* 风向（枚举）          */
    double wind_gust;           /* 阵风（m/s）           */
    WeatherCondition weather;   /* 天气状况（枚举）      */
    char   weather_desc[MAX_DESC_LEN]; /* 天气文字描述   */
    int    visibility;          /* 能见度（米）          */
    int    cloudiness;          /* 云量（0-100%）        */
    double rain_1h;             /* 1小时降雨量（mm）     */
    double snow_1h;             /* 1小时降雪量（mm）     */
    DataSource source;          /* 数据来源（枚举）      */
    SourceDetail source_detail; /* 来源详情（共用体）    */
    char   notes[MAX_DESC_LEN]; /* 备注                  */
    int    created_by;          /* 创建用户ID            */
    DateTime created_at;        /* 创建时间              */
} WeatherRecord;

/* 统计分析结果 */
typedef struct {
    char   city_name[MAX_NAME_LEN];
    int    record_count;
    double temp_avg;
    double temp_min;
    double temp_max;
    double humidity_avg;
    double wind_avg;
    double wind_max;
    WindDirection dominant_wind;
    WeatherCondition dominant_weather;
    int    high_temp_alert;     /* 高温预警：>=35°C     */
    int    strong_wind_alert;   /* 强风预警：>=10.5m/s  */
    int    storm_alert;         /* 暴风雨预警           */
} CityStats;

/* 系统全局状态（单例模式用结构体模拟） */
typedef struct {
    City    cities[MAX_CITIES];
    int     city_count;

    WeatherRecord records[MAX_RECORDS];
    int     record_count;
    int     next_record_id;

    User    users[MAX_USERS];
    int     user_count;
    int     next_user_id;

    User    *current_user;      /* 当前登录用户指针      */
    int     is_logged_in;       /* 登录状态             */
} AppState;

/* ============================================================
 * 全局函数声明
 * ============================================================ */

/* ---- file_io.c ---- */
int  init_data_dir(void);
int  load_all_data(AppState *app);
int  save_all_data(const AppState *app);
int  save_cities(const AppState *app);
int  save_records(const AppState *app);
int  save_users(const AppState *app);
void log_operation(const char *msg);

/* ---- auth.c ---- */
void init_default_users(AppState *app);
void init_default_cities(AppState *app);
int  login(AppState *app, const char *username, const char *password);
void logout(AppState *app);
int  register_user(AppState *app, const char *username,
                   const char *password, UserRole role);

/* ---- city.c ---- */
int  add_city(AppState *app, const char *name, const char *province,
              double lat, double lon);
int  update_city(AppState *app, int city_id, const char *name,
                 const char *province, double lat, double lon);
int  delete_city(AppState *app, int city_id);
City *find_city_by_id(const AppState *app, int city_id);
City *find_city_by_name(const AppState *app, const char *name);
void list_cities(const AppState *app);
void update_city_record_count(AppState *app, int city_id);

/* ---- weather.c ---- */
int  add_record(AppState *app, int city_id, DateTime time,
                double temp, double feels_like, int humidity,
                double pressure, double wind_speed, WindDirection dir,
                double gust, WeatherCondition weather,
                const char *desc, DataSource source);
int  update_record(AppState *app, int record_id, const WeatherRecord *new_data);
int  delete_record(AppState *app, int record_id);
WeatherRecord *find_record_by_id(const AppState *app, int record_id);
void list_records(const AppState *app, int city_id,
                  SortOrder sort, int page, int page_size);
int  count_records_by_city(const AppState *app, int city_id);
void filter_records(const AppState *app, int city_id,
                    const DateTime *from, const DateTime *to,
                    DataSource source_filter);

/* ---- stats.c ---- */
CityStats compute_city_stats(const AppState *app, int city_id);
CityStats compute_all_stats(const AppState *app);
void    show_stats(const AppState *app, int city_id);
void    extreme_weather_alert(const AppState *app);

/* ---- main.c ---- */
void clear_screen(void);
void pause_screen(void);
int  get_int_input(const char *prompt, int min, int max);
double get_double_input(const char *prompt);
void get_string_input(const char *prompt, char *buffer, int max_len);
DateTime get_datetime_input(const char *prompt);
const char *role_to_string(UserRole role);
const char *weather_to_string(WeatherCondition w);
const char *wind_to_string(WindDirection d);
const char *source_to_string(DataSource s);

#endif /* WEATHER_SYSTEM_H */
