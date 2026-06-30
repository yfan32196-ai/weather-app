/**
 * auth.c - 用户认证模块
 *
 * 功能：用户登录/注册管理、默认账户初始化
 * 知识点：结构体数组操作、字符串比较（strcmp）、
 *         函数传址调用（指针作为参数）、循环遍历查找
 */

#include "weather_system.h"

/* 初始化默认用户（首次运行时创建） */
void init_default_users(AppState *app) {
    if (app->user_count > 0) return;  /* 已有用户则跳过 */

    /* 管理员 */
    User *admin = &app->users[0];
    admin->id = app->next_user_id++;
    strcpy(admin->username, "admin");
    strcpy(admin->password, "admin123");
    admin->role = ROLE_ADMIN;
    strcpy(admin->email, "admin@weather.local");
    admin->is_active = 1;
    admin->created_at = (DateTime){2026, 1, 1, 0, 0, 0};

    /* 观察者 */
    User *viewer = &app->users[1];
    viewer->id = app->next_user_id++;
    strcpy(viewer->username, "viewer");
    strcpy(viewer->password, "viewer123");
    viewer->role = ROLE_VIEWER;
    strcpy(viewer->email, "viewer@weather.local");
    viewer->is_active = 1;
    viewer->created_at = (DateTime){2026, 1, 1, 0, 0, 0};

    app->user_count = 2;
    save_users(app);
    printf("✅ 已创建默认用户账号\n");
}

/* 初始化默认城市（首次运行时创建） */
void init_default_cities(AppState *app) {
    if (app->city_count > 0) return;  /* 已有城市则跳过 */

    struct { char *name; char *province; double lat; double lon; } defaults[] = {
        {"北京",     "北京",   39.9042, 116.4074},
        {"上海",     "上海",   31.2304, 121.4737},
        {"广州",     "广东",   23.1291, 113.2644},
        {"深圳",     "广东",   22.5431, 114.0579},
        {"成都",     "四川",   30.5728, 104.0668},
        {"哈尔滨",   "黑龙江", 45.8038, 126.5350},
        {"拉萨",     "西藏",   29.6500,  91.1000},
        {"乌鲁木齐", "新疆",   43.8256,  87.6168},
        {NULL, NULL, 0, 0}
    };

    int i;
    for (i = 0; defaults[i].name != NULL; i++) {
        add_city(app, defaults[i].name, defaults[i].province,
                 defaults[i].lat, defaults[i].lon);
    }
    printf("✅ 已创建 %d 个默认城市\n", app->city_count);
}

/*
 * 登录函数
 *
 * 参数：
 *   app      - 系统全局状态指针
 *   username - 用户名
 *   password - 密码
 *
 * 返回值：
 *   1 - 登录成功
 *   0 - 用户名或密码错误
 *  -1 - 账号已禁用
 */
int login(AppState *app, const char *username, const char *password) {
    int i;
    for (i = 0; i < app->user_count; i++) {
        if (strcmp(app->users[i].username, username) == 0) {
            if (!app->users[i].is_active) {
                return -1;  /* 账号已禁用 */
            }
            if (strcmp(app->users[i].password, password) == 0) {
                app->current_user = &app->users[i];
                app->is_logged_in = 1;

                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg),
                         "用户 '%s' (%s) 登录系统",
                         username, role_to_string(app->users[i].role));
                log_operation(log_msg);
                return 1;  /* 登录成功 */
            }
            return 0;  /* 密码错误 */
        }
    }
    return 0;  /* 用户不存在 */
}

/* 登出 */
void logout(AppState *app) {
    if (app->current_user) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg),
                 "用户 '%s' 退出系统", app->current_user->username);
        log_operation(log_msg);
    }
    app->current_user = NULL;
    app->is_logged_in = 0;
    printf("👋 已退出登录\n");
}

/* 注册新用户（仅管理员可调用） */
int register_user(AppState *app, const char *username,
                  const char *password, UserRole role) {
    if (app->user_count >= MAX_USERS) {
        printf("❌ 用户数已达上限 (%d)\n", MAX_USERS);
        return 0;
    }

    /* 检查用户名是否重复 */
    int i;
    for (i = 0; i < app->user_count; i++) {
        if (strcmp(app->users[i].username, username) == 0) {
            printf("❌ 用户名 '%s' 已存在\n", username);
            return 0;
        }
    }

    User *u = &app->users[app->user_count];
    u->id = app->next_user_id++;
    strcpy(u->username, username);
    strcpy(u->password, password);
    u->role = role;
    u->email[0] = '\0';
    u->is_active = 1;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    u->created_at = (DateTime){
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec
    };

    app->user_count++;
    save_users(app);

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "管理员创建用户 '%s' (角色: %s)", username, role_to_string(role));
    log_operation(log_msg);

    printf("✅ 用户 '%s' 创建成功\n", username);
    return 1;
}
