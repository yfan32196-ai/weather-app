/**
 * main.c - 气象信息管理系统 主程序
 *
 * 常州大学 C 语言课程设计
 * 功能：菜单驱动的控制台交互界面
 * 知识点：主菜单循环、函数调用、分支选择（switch-case）、
 *         用户输入处理、模块集成
 *
 * 编译：gcc -o weather_system main.c auth.c city.c weather.c stats.c file_io.c -lm
 * 运行：./weather_system
 */

#include "weather_system.h"

/* ---- 全局应用状态 ---- */
static AppState app;

/* ============================================================
 * 工具函数实现
 * ============================================================ */

void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pause_screen(void) {
    printf("\n按 Enter 键继续...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    /* Consume the newline if we got EOF */
}

int get_int_input(const char *prompt, int min, int max) {
    int val;
    char buffer[MAX_LINE_LEN];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            /* EOF - exit gracefully */
            printf("\n👋 输入结束，退出系统\n");
            exit(0);
        }
        if (sscanf(buffer, "%d", &val) == 1 && val >= min && val <= max) {
            return val;
        }
        printf("❌ 请输入 %d-%d 之间的整数\n", min, max);
    }
}

double get_double_input(const char *prompt) {
    double val;
    char buffer[MAX_LINE_LEN];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("\n👋 输入结束，退出系统\n");
            exit(0);
        }
        if (sscanf(buffer, "%lf", &val) == 1) {
            return val;
        }
        printf("❌ 请输入有效的数字\n");
    }
}

void get_string_input(const char *prompt, char *buffer, int max_len) {
    printf("%s", prompt);
    if (fgets(buffer, max_len, stdin) == NULL) {
        printf("\n👋 输入结束，退出系统\n");
        exit(0);
    }
    /* 去掉行尾换行符 */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
}

DateTime get_datetime_input(const char *prompt) {
    DateTime dt;
    printf("%s\n", prompt);
    dt.year   = get_int_input("  年 (如 2026): ", 2000, 2100);
    dt.month  = get_int_input("  月 (1-12): ", 1, 12);
    dt.day    = get_int_input("  日 (1-31): ", 1, 31);
    dt.hour   = get_int_input("  时 (0-23): ", 0, 23);
    dt.minute = get_int_input("  分 (0-59): ", 0, 59);
    dt.second = 0;
    return dt;
}

/* ---- 枚举转字符串 ---- */

const char *role_to_string(UserRole role) {
    switch (role) {
        case ROLE_ADMIN:  return "管理员";
        default:          return "观察者";
    }
}

const char *weather_to_string(WeatherCondition w) {
    switch (w) {
        case WEATHER_SUNNY:        return "晴";
        case WEATHER_CLOUDY:       return "多云";
        case WEATHER_OVERCAST:     return "阴";
        case WEATHER_RAIN:         return "雨";
        case WEATHER_DRIZZLE:      return "小雨";
        case WEATHER_SNOW:         return "雪";
        case WEATHER_THUNDERSTORM: return "雷暴";
        case WEATHER_FOG:          return "雾";
        case WEATHER_HAZE:         return "霾";
        default:                   return "--";
    }
}

const char *wind_to_string(WindDirection d) {
    switch (d) {
        case WIND_N:  return "北";
        case WIND_NE: return "东北";
        case WIND_E:  return "东";
        case WIND_SE: return "东南";
        case WIND_S:  return "南";
        case WIND_SW: return "西南";
        case WIND_W:  return "西";
        case WIND_NW: return "西北";
        default:      return "--";
    }
}

const char *source_to_string(DataSource s) {
    switch (s) {
        case SOURCE_MANUAL: return "手动录入";
        case SOURCE_API:    return "API获取";
        case SOURCE_IMPORT: return "批量导入";
        default:            return "未知";
    }
}

/* ============================================================
 * 菜单界面
 * ============================================================ */

/* 主菜单 */
static void show_main_menu(void) {
    UserRole role = app.current_user ? app.current_user->role : ROLE_VIEWER;

    printf("\n");
    printf("╔═══════════════════════════════════╗\n");
    printf("║     气 象 信 息 管 理 系 统       ║\n");
    printf("╠═══════════════════════════════════╣\n");
    printf("║  当前用户: %-8s  [%s] ║\n",
           app.current_user->username, role_to_string(role));
    printf("╠═══════════════════════════════════╣\n");
    printf("║  [1] 📊 气象记录管理              ║\n");
    printf("║  [2] 🏙️  城市管理                  ║\n");
    printf("║  [3] 📈 统计分析                  ║\n");
    printf("║  [4] ⚠️  极端天气预警              ║\n");
    printf("║  [5] 📥 数据导入 (CSV/手动)     ║\n");
    printf("║  [6] 📤 数据导出 (CSV)           ║\n");

    if (role == ROLE_ADMIN) {
        printf("║  [7] 👥 用户管理                  ║\n");
    }

    printf("╠═══════════════════════════════════╣\n");
    printf("║  [0] 🚪 退出系统                  ║\n");
    printf("╚═══════════════════════════════════╝\n");
}

/* 记录管理子菜单 */
static void show_record_menu(void) {
    printf("\n┌─── 气象记录管理 ───┐\n");
    printf("│ [1] 查看全部记录     │\n");
    printf("│ [2] 按城市筛选       │\n");
    printf("│ [3] 按条件筛选       │\n");
    printf("│ [4] 添加记录         │\n");
    printf("│ [5] 编辑记录         │\n");
    printf("│ [6] 删除记录         │\n");
    printf("│ [0] 返回主菜单       │\n");
    printf("└─────────────────────┘\n");
}

/* ============================================================
 * 功能模块调用
 * ============================================================ */

/* 气象记录管理 */
static void handle_records(void) {
    int running = 1;
    while (running) {
        clear_screen();
        show_record_menu();
        int choice = get_int_input("请选择: ", 0, 6);

        switch (choice) {
            case 1: /* 查看全部 */
                list_records(&app, 0, SORT_BY_TIME_DESC, 1, 20);
                pause_screen();
                break;
            case 2: { /* 按城市筛选 */
                list_cities(&app);
                int cid = get_int_input("请输入城市ID (0=全部): ", 0, 999);
                list_records(&app, cid, SORT_BY_TIME_DESC, 1, 20);
                pause_screen();
                break;
            }
            case 3: { /* 按条件筛选 */
                int cid = get_int_input("城市ID (0=全部): ", 0, 999);
                DateTime from = get_datetime_input("--- 开始日期 ---");
                DateTime to   = get_datetime_input("--- 结束日期 ---");
                int src = get_int_input("数据来源 (0=手动 1=API 2=导入 -1=全部): ",
                                        -1, 2);
                filter_records(&app, cid, &from, &to, (DataSource)src);
                pause_screen();
                break;
            }
            case 4: { /* 添加记录 */
                UserRole role = app.current_user->role;
                if (role == ROLE_VIEWER) {
                    printf("❌ 观察者无权限添加记录\n");
                    pause_screen();
                    break;
                }
                list_cities(&app);
                if (app.city_count == 0) {
                    printf("⚠️ 请先添加城市\n");
                    pause_screen();
                    break;
                }
                int cid = get_int_input("选择城市ID: ", 1, app.city_count);
                DateTime time = get_datetime_input("--- 记录时间 ---");
                double temp = get_double_input("温度 (°C): ");
                double feels = get_double_input("体感温度 (°C): ");
                int humidity = get_int_input("湿度 (0-100): ", 0, 100);
                double pressure = get_double_input("气压 (hPa): ");
                double wind = get_double_input("风速 (m/s): ");
                printf("风向: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW\n");
                int dir_int = get_int_input("选择风向: ", 0, 7);
                printf("天气: 0=晴 1=多云 2=阴 3=雨 4=小雨 5=雪 6=雷暴 7=雾 8=霾\n");
                int w_int = get_int_input("选择天气: ", 0, 8);

                add_record(&app, cid, time, temp, feels, humidity,
                          pressure, wind, (WindDirection)dir_int, 0.0,
                          (WeatherCondition)w_int, "", SOURCE_MANUAL);
                pause_screen();
                break;
            }
            case 5: { /* 编辑记录 */
                UserRole role = app.current_user->role;
                if (role == ROLE_VIEWER) {
                    printf("❌ 观察者无权限编辑记录\n");
                    pause_screen();
                    break;
                }
                list_records(&app, 0, SORT_BY_TIME_DESC, 1, 5);
                int rid = get_int_input("请输入要编辑的记录ID: ", 1, 99999);
                WeatherRecord *r = find_record_by_id(&app, rid);
                if (r == NULL) {
                    printf("❌ 记录不存在\n");
                } else {
                    printf("当前温度: %.1f°C\n", r->temperature);
                    r->temperature = get_double_input("新温度 (°C): ");
                    printf("当前湿度: %d%%\n", r->humidity);
                    r->humidity = get_int_input("新湿度 (0-100): ", 0, 100);
                    printf("当前风速: %.1f m/s\n", r->wind_speed);
                    r->wind_speed = get_double_input("新风速 (m/s): ");
                    save_records(&app);
                    printf("✅ 记录更新成功\n");
                }
                pause_screen();
                break;
            }
            case 6: { /* 删除记录 */
                UserRole role = app.current_user->role;
                if (role != ROLE_ADMIN) {
                    printf("❌ 仅管理员可删除记录\n");
                    pause_screen();
                    break;
                }
                list_records(&app, 0, SORT_BY_TIME_DESC, 1, 5);
                int rid = get_int_input("请输入要删除的记录ID: ", 1, 99999);
                delete_record(&app, rid);
                pause_screen();
                break;
            }
            case 0:
                running = 0;
                break;
        }
    }
}

/* 城市管理 */
static void handle_cities(void) {
    clear_screen();
    list_cities(&app);

    UserRole role = app.current_user->role;
    if (role == ROLE_VIEWER) {
        printf("\n观察者只能查看城市信息\n");
        pause_screen();
        return;
    }

    printf("\n┌── 城市管理 ──┐\n");
    printf("│ [1] 添加城市    │\n");
    printf("│ [2] 编辑城市    │\n");
    if (role == ROLE_ADMIN) {
        printf("│ [3] 删除城市    │\n");
    }
    printf("│ [0] 返回        │\n");
    printf("└───────────────┘\n");

    int max_opt = (role == ROLE_ADMIN) ? 3 : 2;
    int choice = get_int_input("请选择: ", 0, max_opt);

    switch (choice) {
        case 1: {
            char name[MAX_NAME_LEN], province[MAX_NAME_LEN];
            get_string_input("城市名称: ", name, MAX_NAME_LEN);
            get_string_input("省份: ", province, MAX_NAME_LEN);
            double lat = get_double_input("纬度: ");
            double lon = get_double_input("经度: ");
            add_city(&app, name, province, lat, lon);
            break;
        }
        case 2: {
            int cid = get_int_input("要编辑的城市ID: ", 1, app.city_count);
            char name[MAX_NAME_LEN], province[MAX_NAME_LEN];
            get_string_input("新名称: ", name, MAX_NAME_LEN);
            get_string_input("新省份: ", province, MAX_NAME_LEN);
            double lat = get_double_input("新纬度: ");
            double lon = get_double_input("新经度: ");
            update_city(&app, cid, name, province, lat, lon);
            break;
        }
        case 3:
            if (role == ROLE_ADMIN) {
                int cid = get_int_input("要删除的城市ID: ", 1, app.city_count);
                printf("⚠️ 删除城市将同时删除其所有气象记录！\n");
                int confirm = get_int_input("确认删除？(1=是 0=否): ", 0, 1);
                if (confirm) delete_city(&app, cid);
            }
            break;
    }
    pause_screen();
}

/* 统计分析 */
static void handle_stats(void) {
    clear_screen();
    list_cities(&app);

    printf("\n┌── 统计分析 ──┐\n");
    printf("│ [1] 全部城市概览 │\n");
    printf("│ [2] 单城市详细统计│\n");
    printf("│ [0] 返回          │\n");
    printf("└──────────────────┘\n");

    int choice = get_int_input("请选择: ", 0, 2);

    switch (choice) {
        case 1:
            show_stats(&app, 0);
            break;
        case 2: {
            int cid = get_int_input("请输入城市ID: ", 1, app.city_count);
            show_stats(&app, cid);
            break;
        }
    }
    pause_screen();
}

/* 用户管理（仅管理员） */
static void handle_users(void) {
    clear_screen();

    printf("\n┌──────────────────────────────────────────────────┐\n");
    printf("│ ID │ 用户名      │ 角色     │ 状态 │ 邮箱              │\n");
    printf("├────┼──────────┼──────────┼──────┼──────────────────┤\n");
    int i;
    for (i = 0; i < app.user_count; i++) {
        User *u = &app.users[i];
        printf("│ %2d │ %-10s │ %-8s │ %-4s │ %-16s │\n",
               u->id, u->username, role_to_string(u->role),
               u->is_active ? "激活" : "禁用",
               strlen(u->email) > 0 ? u->email : "-");
    }
    printf("└────┴──────────┴──────────┴──────┴──────────────────┘\n");

    printf("\n┌── 用户管理 ──┐\n");
    printf("│ [1] 添加用户    │\n");
    printf("│ [0] 返回        │\n");
    printf("└───────────────┘\n");

    int choice = get_int_input("请选择: ", 0, 1);

    if (choice == 1) {
        char username[MAX_NAME_LEN], password[MAX_PWD_LEN];
        get_string_input("用户名: ", username, MAX_NAME_LEN);
        get_string_input("密码: ", password, MAX_PWD_LEN);
        printf("角色: 0=管理员 1=观察者\n");
        int role_int = get_int_input("选择角色: ", 0, 1);
        if (role_int == 1) role_int = 2;  /* 映射到 ROLE_VIEWER */
        register_user(&app, username, password, (UserRole)role_int);
    }
    pause_screen();
}

/* 手动逐条输入气象数据 */
static void manual_batch_input(void) {
    if (app.city_count == 0) {
        printf("❌ 请先添加城市\n");
        return;
    }

    printf("\n📝 === 手动逐条录入气象数据 ===\n");
    printf("（输入城市ID为 0 时结束录入）\n\n");

    int count = 0;
    while (1) {
        list_cities(&app);
        int cid = get_int_input("\n选择城市ID (0=结束): ", 0, app.city_count);
        if (cid == 0) break;

        DateTime time = get_datetime_input("--- 记录时间 ---");
        double temp    = get_double_input("温度 (°C): ");
        double feels   = get_double_input("体感温度 (°C): ");
        int humidity   = get_int_input("湿度 (0-100): ", 0, 100);
        double pressure = get_double_input("气压 (hPa): ");
        double wind    = get_double_input("风速 (m/s): ");
        printf("风向: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW\n");
        int dir_int    = get_int_input("选择风向: ", 0, 7);
        double gust    = get_double_input("阵风 (m/s): ");
        printf("天气: 0=晴 1=多云 2=阴 3=雨 4=小雨 5=雪 6=雷暴 7=雾 8=霾\n");
        int w_int      = get_int_input("选择天气: ", 0, 8);
        int src        = get_int_input("数据来源 (0=手动录入 1=API获取 2=批量导入): ", 0, 2);

        add_record(&app, cid, time, temp, feels, humidity,
                  pressure, wind, (WindDirection)dir_int, gust,
                  (WeatherCondition)w_int, "", (DataSource)src);
        count++;
        printf("✅ 第 %d 条记录已添加\n\n", count);
    }
    printf("📊 本次共手动录入 %d 条记录\n", count);
}

/* 数据导入 */
static void handle_import(void) {
    clear_screen();

    printf("\n┌── 数据导入 ────┐\n");
    printf("│ [1] CSV文件导入   │\n");
    printf("│ [2] 手动逐条录入  │\n");
    printf("│ [0] 返回          │\n");
    printf("└──────────────────┘\n");

    int choice = get_int_input("请选择: ", 0, 2);

    if (choice == 1) {
        /* CSV 导入 */
        printf("\n╔════════════════════════════════════╗\n");
        printf("║         CSV 文 件 导 入           ║\n");
        printf("╠════════════════════════════════════╣\n");
        printf("║ 格式: city_name,time,temp,        ║\n");
        printf("║ humidity,pressure,wind_speed,     ║\n");
        printf("║ wind_dir,weather,source           ║\n");
        printf("║ 示例: 北京,2026-06-30 14:00,      ║\n");
        printf("║ 30.5,55,1013,3.2,0,0,0           ║\n");
        printf("╚════════════════════════════════════╝\n\n");

        char filepath[MAX_NAME_LEN];
        get_string_input("CSV文件路径: ", filepath, MAX_NAME_LEN);

        FILE *fp = fopen(filepath, "r");
        if (fp == NULL) {
            printf("❌ 无法打开文件: %s\n", filepath);
            pause_screen();
            return;
        }

        char line[MAX_LINE_LEN];
        int imported = 0, skipped = 0;
        fgets(line, sizeof(line), fp);  /* 跳过表头 */

        while (fgets(line, sizeof(line), fp) != NULL) {
            char city_name[MAX_NAME_LEN], time_str[32];
            double temp, pressure, wind_speed;
            int humidity, wind_dir_int, weather_int, source_int;

            int n = sscanf(line, "%[^,],%[^,],%lf,%d,%lf,%lf,%d,%d,%d",
                           city_name, time_str, &temp, &humidity,
                           &pressure, &wind_speed, &wind_dir_int,
                           &weather_int, &source_int);

            if (n < 9) { skipped++; continue; }

            City *city = find_city_by_name(&app, city_name);
            if (city == NULL) { skipped++; continue; }

            DateTime dt = {2026, 6, 30, 12, 0, 0};
            sscanf(time_str, "%d-%d-%d %d:%d",
                   &dt.year, &dt.month, &dt.day, &dt.hour, &dt.minute);

            add_record(&app, city->id, dt, temp, temp - 2.0, humidity,
                      pressure, wind_speed, (WindDirection)wind_dir_int,
                      0.0, (WeatherCondition)weather_int, "",
                      (DataSource)source_int);
            imported++;
        }
        fclose(fp);
        printf("\n✅ CSV导入完成：成功 %d 条，跳过 %d 条\n", imported, skipped);
    } else if (choice == 2) {
        /* 手动逐条录入 */
        manual_batch_input();
    }

    pause_screen();
}

/* 数据导出 */
static void handle_export(void) {
    clear_screen();
    printf("╔════════════════════════════════════╗\n");
    printf("║        数 据 导 出 (CSV)          ║\n");
    printf("╠════════════════════════════════════╣\n");
    printf("║ 导出路径: data/export.csv         ║\n");
    printf("╚════════════════════════════════════╝\n\n");

    int confirm = get_int_input("确认导出全部记录？(1=是 0=否): ", 0, 1);
    if (!confirm) return;

    FILE *fp = fopen("data/export.csv", "w");
    if (fp == NULL) {
        printf("❌ 无法创建导出文件\n");
        pause_screen();
        return;
    }

    /* 写表头 */
    fprintf(fp, "ID,城市,时间,温度(°C),体感温度(°C),湿度(%%)"
            ",气压(hPa),风速(m/s),风向,天气,来源,备注\n");

    int i;
    for (i = 0; i < app.record_count; i++) {
        WeatherRecord *r = &app.records[i];
        City *city = find_city_by_id(&app, r->city_id);

        fprintf(fp, "%d,%s,%04d-%02d-%02d %02d:%02d:00,"
                "%.1f,%.1f,%d,%.1f,%.1f,%s,%s,%s,%s\n",
                r->id,
                city ? city->name : "未知",
                r->record_time.year, r->record_time.month,
                r->record_time.day, r->record_time.hour,
                r->record_time.minute,
                r->temperature, r->feels_like, r->humidity,
                r->pressure, r->wind_speed,
                wind_to_string(r->wind_dir),
                weather_to_string(r->weather),
                source_to_string(r->source),
                r->notes);
    }

    fclose(fp);
    printf("✅ 已导出 %d 条记录到 data/export.csv\n", app.record_count);
    pause_screen();
}

/* ============================================================
 * 主函数入口
 * ============================================================ */

int main(void) {
    /* 初始化 */
    memset(&app, 0, sizeof(AppState));
    init_data_dir();
    load_all_data(&app);
    init_default_users(&app);
    init_default_cities(&app);

    /* === 登录 === */
    clear_screen();
    printf("\n");
    printf("╔═══════════════════════════════════╗\n");
    printf("║     气 象 信 息 管 理 系 统       ║\n");
    printf("║          C 语言课程设计           ║\n");
    printf("╚═══════════════════════════════════╝\n\n");

    int login_ok = 0;
    while (!login_ok) {
        char username[MAX_NAME_LEN], password[MAX_PWD_LEN];
        get_string_input("用户名: ", username, MAX_NAME_LEN);
        get_string_input("密码: ", password, MAX_PWD_LEN);

        int result = login(&app, username, password);
        switch (result) {
            case 1:
                printf("✅ 登录成功！欢迎 %s\n\n", username);
                login_ok = 1;
                break;
            case -1:
                printf("❌ 账号已被禁用，请联系管理员\n\n");
                break;
            default:
                printf("❌ 用户名或密码错误，请重试\n\n");
                break;
        }
    }

    /* === 主循环 === */
    int running = 1;
    while (running) {
        clear_screen();
        show_main_menu();

        UserRole role = app.current_user->role;
        int max_choice = (role == ROLE_ADMIN) ? 7 : 6;
        int choice = get_int_input("请选择操作: ", 0, max_choice);

        switch (choice) {
            case 1: handle_records();    break;
            case 2: handle_cities();     break;
            case 3: handle_stats();      break;
            case 4:
                clear_screen();
                extreme_weather_alert(&app);
                pause_screen();
                break;
            case 5:
                if (role != ROLE_VIEWER) handle_import();
                else printf("❌ 权限不足\n");
                pause_screen();
                break;
            case 6:
                handle_export();
                break;
            case 7:
                if (role == ROLE_ADMIN) handle_users();
                break;
            case 0:
                running = 0;
                break;
        }
    }

    /* 退出 */
    logout(&app);
    save_all_data(&app);
    printf("\n👋 感谢使用气象信息管理系统！\n");
    return 0;
}
