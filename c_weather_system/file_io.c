/**
 * file_io.c - 文件读写模块
 *
 * 功能：系统所有数据的文件持久化存储
 * 知识点：fopen/fclose、fread/fwrite 二进制文件操作、
 *         文件指针定位、目录创建
 */

#include "weather_system.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* ---- 内部辅助函数 ---- */

/* 创建数据目录 */
int init_data_dir(void) {
    struct stat st = {0};
    if (stat(DATA_DIR, &st) == -1) {
#ifdef _WIN32
        if (mkdir(DATA_DIR) != 0) {
#else
        if (mkdir(DATA_DIR, 0755) != 0) {
#endif
            printf("❌ 创建数据目录 '%s' 失败: %s\n", DATA_DIR, strerror(errno));
            return 0;
        }
        printf("📁 已创建数据目录: %s\n", DATA_DIR);
    }
    return 1;
}

/* ---- 用户文件操作 ---- */

/* 从文件加载用户数据 */
static int load_users(AppState *app) {
    FILE *fp = fopen(USER_FILE, "rb");
    if (fp == NULL) {
        printf("📝 用户数据文件不存在，将创建默认用户\n");
        return 0;
    }

    /* 先读取用户数量 */
    if (fread(&app->user_count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }
    if (fread(&app->next_user_id, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    /* 读取用户数组 */
    size_t read_count = fread(app->users, sizeof(User), app->user_count, fp);
    fclose(fp);

    if (read_count != (size_t)app->user_count) {
        printf("⚠️ 用户数据读取不完整\n");
        return 0;
    }

    printf("✅ 已加载 %d 个用户\n", app->user_count);
    return 1;
}

/* 保存用户数据到文件 */
int save_users(const AppState *app) {
    FILE *fp = fopen(USER_FILE, "wb");
    if (fp == NULL) {
        printf("❌ 无法写入用户文件: %s\n", USER_FILE);
        return 0;
    }

    fwrite(&app->user_count, sizeof(int), 1, fp);
    fwrite(&app->next_user_id, sizeof(int), 1, fp);
    fwrite(app->users, sizeof(User), app->user_count, fp);
    fclose(fp);
    return 1;
}

/* ---- 城市文件操作 ---- */

static int load_cities(AppState *app) {
    FILE *fp = fopen(CITY_FILE, "rb");
    if (fp == NULL) return 0;

    if (fread(&app->city_count, sizeof(int), 1, fp) != 1) { fclose(fp); return 0; }
    fread(app->cities, sizeof(City), app->city_count, fp);
    fclose(fp);
    printf("✅ 已加载 %d 个城市\n", app->city_count);
    return 1;
}

int save_cities(const AppState *app) {
    FILE *fp = fopen(CITY_FILE, "wb");
    if (fp == NULL) return 0;

    fwrite(&app->city_count, sizeof(int), 1, fp);
    fwrite(app->cities, sizeof(City), app->city_count, fp);
    fclose(fp);
    return 1;
}

/* ---- 气象记录文件操作 ---- */

static int load_records(AppState *app) {
    FILE *fp = fopen(RECORD_FILE, "rb");
    if (fp == NULL) return 0;

    if (fread(&app->record_count, sizeof(int), 1, fp) != 1) { fclose(fp); return 0; }
    if (fread(&app->next_record_id, sizeof(int), 1, fp) != 1) { fclose(fp); return 0; }
    fread(app->records, sizeof(WeatherRecord), app->record_count, fp);
    fclose(fp);
    printf("✅ 已加载 %d 条气象记录\n", app->record_count);
    return 1;
}

int save_records(const AppState *app) {
    FILE *fp = fopen(RECORD_FILE, "wb");
    if (fp == NULL) return 0;

    fwrite(&app->record_count, sizeof(int), 1, fp);
    fwrite(&app->next_record_id, sizeof(int), 1, fp);
    fwrite(app->records, sizeof(WeatherRecord), app->record_count, fp);
    fclose(fp);
    return 1;
}

/* ---- 统一加载/保存 ---- */

int load_all_data(AppState *app) {
    init_data_dir();

    /* 加载用户 */
    if (!load_users(app)) {
        app->user_count = 0;
        app->next_user_id = 1;
    }

    /* 加载城市 */
    if (!load_cities(app)) {
        app->city_count = 0;
    }

    /* 加载气象记录 */
    if (!load_records(app)) {
        app->record_count = 0;
        app->next_record_id = 1;
    }

    /* 更新城市记录计数 */
    int i;
    for (i = 0; i < app->city_count; i++) {
        update_city_record_count(app, app->cities[i].id);
    }

    return 1;
}

int save_all_data(const AppState *app) {
    return save_users(app) && save_cities(app) && save_records(app);
}

/* ---- 操作日志 ---- */

void log_operation(const char *msg) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp == NULL) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, msg);
    fclose(fp);
}
