/**
 * stats.c - 统计分析模块
 *
 * 功能：气象数据统计分析、极端天气预警
 * 知识点：循环遍历、条件判断、浮点数累加、最大/最小值查找、
 *         switch-case、数学计算（平均值、频次统计）
 */

#include "weather_system.h"

/* 计算单个城市的统计数据 */
CityStats compute_city_stats(const AppState *app, int city_id) {
    CityStats stats;
    memset(&stats, 0, sizeof(CityStats));

    City *city = find_city_by_id(app, city_id);
    if (city) {
        strcpy(stats.city_name, city->name);
    } else {
        strcpy(stats.city_name, "未知");
    }

    int wind_dir_count[8] = {0};
    int weather_count[9] = {0};
    double temp_sum = 0.0;
    double humid_sum = 0.0;
    double wind_sum = 0.0;
    int temp_count = 0;
    int i;

    stats.temp_min = 999.0;
    stats.temp_max = -999.0;
    stats.wind_max = 0.0;

    for (i = 0; i < app->record_count; i++) {
        const WeatherRecord *r = &app->records[i];
        if (r->city_id != city_id) continue;

        stats.record_count++;

        /* 温度统计 */
        if (r->temperature > -50.0 && r->temperature < 60.0) {
            if (r->temperature < stats.temp_min) stats.temp_min = r->temperature;
            if (r->temperature > stats.temp_max) stats.temp_max = r->temperature;
            temp_sum += r->temperature;
            temp_count++;
        }

        /* 湿度 */
        humid_sum += r->humidity;

        /* 风速 */
        if (r->wind_speed > 0) {
            wind_sum += r->wind_speed;
            if (r->wind_speed > stats.wind_max) stats.wind_max = r->wind_speed;
        }

        /* 风向频次 */
        wind_dir_count[r->wind_dir]++;

        /* 天气类型频次 */
        weather_count[r->weather]++;
    }

    /* 计算平均值 */
    stats.temp_avg = (temp_count > 0) ? temp_sum / temp_count : 0.0;
    stats.humidity_avg = (stats.record_count > 0)
        ? humid_sum / stats.record_count : 0.0;
    stats.wind_avg = (stats.record_count > 0)
        ? wind_sum / stats.record_count : 0.0;

    /* 找出主导风向 */
    int max_wind = 0, max_wind_idx = 0;
    for (i = 0; i < 8; i++) {
        if (wind_dir_count[i] > max_wind) {
            max_wind = wind_dir_count[i];
            max_wind_idx = i;
        }
    }
    stats.dominant_wind = (WindDirection)max_wind_idx;

    /* 找出主要天气类型 */
    int max_weather = 0, max_weather_idx = 0;
    for (i = 0; i < 9; i++) {
        if (weather_count[i] > max_weather) {
            max_weather = weather_count[i];
            max_weather_idx = i;
        }
    }
    stats.dominant_weather = (WeatherCondition)max_weather_idx;

    /* 极端天气检测 */
    stats.high_temp_alert = (stats.temp_max >= 35.0) ? 1 : 0;
    stats.strong_wind_alert = (stats.wind_max >= 10.5) ? 1 : 0;
    stats.storm_alert = (weather_count[WEATHER_THUNDERSTORM] > 0 ||
                         weather_count[WEATHER_RAIN] > stats.record_count / 3) ? 1 : 0;

    return stats;
}

/* 计算所有城市的汇总统计 */
CityStats compute_all_stats(const AppState *app) {
    CityStats stats;
    memset(&stats, 0, sizeof(CityStats));
    strcpy(stats.city_name, "全部城市");

    stats.temp_min = 999.0;
    stats.temp_max = -999.0;

    double temp_sum = 0.0;
    int temp_count = 0;
    int wind_dir_count[8] = {0};
    int weather_count[9] = {0};
    int i;

    for (i = 0; i < app->record_count; i++) {
        const WeatherRecord *r = &app->records[i];
        stats.record_count++;

        if (r->temperature > -50.0 && r->temperature < 60.0) {
            if (r->temperature < stats.temp_min) stats.temp_min = r->temperature;
            if (r->temperature > stats.temp_max) stats.temp_max = r->temperature;
            temp_sum += r->temperature;
            temp_count++;
        }

        stats.humidity_avg += r->humidity;
        stats.wind_avg += r->wind_speed;

        if (r->wind_speed > stats.wind_max) stats.wind_max = r->wind_speed;

        wind_dir_count[r->wind_dir]++;
        weather_count[r->weather]++;
    }

    stats.temp_avg = (temp_count > 0) ? temp_sum / temp_count : 0.0;
    if (stats.record_count > 0) {
        stats.humidity_avg /= stats.record_count;
        stats.wind_avg /= stats.record_count;
    }

    /* 主导风向 */
    int max_wind = 0, max_wind_idx = 0;
    for (i = 0; i < 8; i++) {
        if (wind_dir_count[i] > max_wind) {
            max_wind = wind_dir_count[i];
            max_wind_idx = i;
        }
    }
    stats.dominant_wind = (WindDirection)max_wind_idx;

    /* 主要天气 */
    int max_weather = 0, max_weather_idx = 0;
    for (i = 0; i < 9; i++) {
        if (weather_count[i] > max_weather) {
            max_weather = weather_count[i];
            max_weather_idx = i;
        }
    }
    stats.dominant_weather = (WeatherCondition)max_weather_idx;

    stats.high_temp_alert = (stats.temp_max >= 35.0);
    stats.strong_wind_alert = (stats.wind_max >= 10.5);
    stats.storm_alert = (weather_count[WEATHER_THUNDERSTORM] > 0);

    return stats;
}

/* 展示单个城市或全部统计 */
void show_stats(const AppState *app, int city_id) {
    if (app->record_count == 0) {
        printf("\n📋 暂无数据可供分析\n");
        return;
    }

    if (city_id > 0) {
        /* 单城市统计 */
        CityStats s = compute_city_stats(app, city_id);
        if (s.record_count == 0) {
            printf("📋 该城市暂无记录\n");
            return;
        }

        printf("\n╔══════════════════════════════════════════╗\n");
        printf("║        %s 气象统计                  ║\n", s.city_name);
        printf("╠══════════════════════════════════════════╣\n");
        printf("║ 记录数:    %4d 条                       ║\n", s.record_count);
        printf("║ 平均温度:  %6.1f °C                     ║\n", s.temp_avg);
        printf("║ 最高温度:  %6.1f °C                     ║\n", s.temp_max);
        printf("║ 最低温度:  %6.1f °C                     ║\n", s.temp_min);
        printf("║ 平均湿度:  %6.1f %%                      ║\n", s.humidity_avg);
        printf("║ 平均风速:  %6.2f m/s                    ║\n", s.wind_avg);
        printf("║ 最大风速:  %6.2f m/s                    ║\n", s.wind_max);
        printf("║ 主导风向:  %-8s                        ║\n",
               wind_to_string(s.dominant_wind));
        printf("║ 主要天气:  %-8s                        ║\n",
               weather_to_string(s.dominant_weather));
        printf("╠══════════════════════════════════════════╣\n");
        printf("║ ⚠️ 预警:                                ║\n");
        printf("║   高温: %-3s  强风: %-3s  暴风雨: %-3s ║\n",
               s.high_temp_alert ? "⚠️" : "✅",
               s.strong_wind_alert ? "⚠️" : "✅",
               s.storm_alert ? "⚠️" : "✅");
        printf("╚══════════════════════════════════════════╝\n");
    } else {
        /* 全部城市概览 */
        printf("\n╔══════════════════════════════════════════════════════════════════════╗\n");
        printf("║                        全 部 城 市 统 计                            ║\n");
        printf("╠════════┬──────┬────────┬────────┬────────┬──────┬────────┬──────────╣\n");
        printf("║ 城市   │ 记录 │ 均温   │ 最高   │ 最低   │ 湿度 │ 风速   │ 主导天气 ║\n");
        printf("╠════════╪══════╪════════╪════════╪════════╪══════╪════════╪══════════╣\n");

        int i;
        for (i = 0; i < app->city_count; i++) {
            CityStats s = compute_city_stats(app, app->cities[i].id);
            printf("║ %-6s │ %4d │ %5.1f°C│ %5.1f°C│ %5.1f°C│ %3.0f%% │ %5.2fm│ %-8s ║\n",
                   app->cities[i].name,
                   s.record_count,
                   s.temp_avg, s.temp_max, s.temp_min,
                   s.humidity_avg, s.wind_avg,
                   weather_to_string(s.dominant_weather));
        }
        printf("╚════════╧══════╧════════╧════════╧════════╧══════╧════════╧══════════╝\n");
    }
}

/* 极端天气预警扫描 */
void extreme_weather_alert(const AppState *app) {
    int alert_count = 0;
    int i;

    printf("\n⚠️ === 极端天气预警扫描 === ⚠️\n\n");

    for (i = 0; i < app->city_count; i++) {
        CityStats s = compute_city_stats(app, app->cities[i].id);
        if (s.record_count == 0) continue;

        int has_alert = 0;
        printf("🏙️  %s:\n", s.city_name);

        if (s.high_temp_alert) {
            printf("   🌡️  高温预警：最高气温 %.1f°C（>=35°C）\n", s.temp_max);
            has_alert = 1;
        }
        if (s.strong_wind_alert) {
            printf("   💨 强风预警：最大风速 %.1f m/s（>=10.5m/s）\n", s.wind_max);
            has_alert = 1;
        }
        if (s.storm_alert) {
            printf("   ⛈️  暴风雨预警：存在雷暴或大量降雨记录\n");
            has_alert = 1;
        }
        if (!has_alert) {
            printf("   ✅ 无极端天气预警\n");
        }
        alert_count += has_alert;
        printf("\n");
    }

    if (alert_count == 0) {
        printf("✅ 所有城市无极端天气预警\n");
    } else {
        printf("⚠️ 共 %d 个城市存在极端天气风险\n", alert_count);
    }
}
