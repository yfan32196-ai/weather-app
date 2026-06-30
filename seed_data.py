#!/usr/bin/env python3
"""Seed script: creates admin user and sample cities with demo weather data."""

import random
from datetime import datetime, timedelta, timezone

from app import create_app
from app.extensions import db
from app.models import User, City, WeatherRecord


def seed():
    app = create_app()
    with app.app_context():
        # Drop all tables and recreate
        db.drop_all()
        db.create_all()

        print("📦 创建用户...")
        admin = User(username='admin', role='admin', email='admin@weather.local')
        admin.set_password('admin123')
        editor = User(username='editor', role='editor', email='editor@weather.local')
        editor.set_password('editor123')
        viewer = User(username='viewer', role='viewer', email='viewer@weather.local')
        viewer.set_password('viewer123')
        db.session.add_all([admin, editor, viewer])
        db.session.commit()
        print("  ✅ admin / admin123 (管理员)")
        print("  ✅ editor / editor123 (编辑者)")
        print("  ✅ viewer / viewer123 (观察者)")

        print("\n🏙️  创建城市...")
        cities_data = [
            {'name': '北京', 'country': '中国', 'state': '北京', 'lat': 39.9042, 'lon': 116.4074, 'owm': 1816670},
            {'name': '上海', 'country': '中国', 'state': '上海', 'lat': 31.2304, 'lon': 121.4737, 'owm': 1796236},
            {'name': '广州', 'country': '中国', 'state': '广东', 'lat': 23.1291, 'lon': 113.2644, 'owm': 1809106},
            {'name': '成都', 'country': '中国', 'state': '四川', 'lat': 30.5728, 'lon': 104.0668, 'owm': 1815286},
            {'name': '哈尔滨', 'country': '中国', 'state': '黑龙江', 'lat': 45.8038, 'lon': 126.5350, 'owm': 2037013},
            {'name': '拉萨', 'country': '中国', 'state': '西藏', 'lat': 29.6500, 'lon': 91.1000, 'owm': 1280737},
            {'name': '乌鲁木齐', 'country': '中国', 'state': '新疆', 'lat': 43.8256, 'lon': 87.6168, 'owm': 1529102},
            {'name': '深圳', 'country': '中国', 'state': '广东', 'lat': 22.5431, 'lon': 114.0579, 'owm': 1795565},
        ]
        cities = []
        for cd in cities_data:
            city = City(
                name=cd['name'], country=cd['country'], state=cd['state'],
                latitude=cd['lat'], longitude=cd['lon'], owm_city_id=cd['owm'],
            )
            db.session.add(city)
            cities.append(city)
        db.session.commit()
        for c in cities:
            print(f"  ✅ {c.name} ({c.latitude}, {c.longitude})")

        print("\n🌤️  生成示例气象数据...")
        weather_main_options = ['Clear', 'Clouds', 'Rain', 'Drizzle', 'Haze', 'Fog']
        weather_desc_map = {
            'Clear': '晴', 'Clouds': '多云', 'Rain': '雨',
            'Drizzle': '小雨', 'Haze': '霾', 'Fog': '雾',
        }
        wind_dirs = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW']

        # Generate hourly records for the past 3 days
        now = datetime.now(timezone.utc)
        records = []
        for city in cities:
            # Base temperature varies by city (roughly by latitude)
            base_temp = 35 - abs(city.latitude) * 0.5  # Hotter near equator
            base_humidity = 60 + random.uniform(-15, 15)

            for hours_ago in range(72, 0, -1):
                record_time = now - timedelta(hours=hours_ago)

                # Add some randomness and daily cycle
                hour_of_day = record_time.hour
                daily_cycle = -5 * abs(hour_of_day - 14) / 10  # Peak at 2pm
                temp = base_temp + daily_cycle + random.uniform(-3, 3)
                humidity = min(100, max(10, base_humidity + random.uniform(-10, 10)))
                pressure = 1013 + random.uniform(-10, 10)
                wind_speed = random.uniform(0, 10)

                weather_main = random.choice(weather_main_options)

                record = WeatherRecord(
                    city_id=city.id,
                    record_time=record_time,
                    temperature=round(temp, 1),
                    feels_like=round(temp - random.uniform(0, 3), 1),
                    humidity=int(humidity),
                    pressure=round(pressure, 1),
                    wind_speed=round(wind_speed, 1),
                    wind_direction=random.choice(wind_dirs),
                    wind_gust=round(wind_speed * random.uniform(1.0, 1.8), 1),
                    weather_main=weather_main,
                    weather_desc=weather_desc_map.get(weather_main, ''),
                    visibility=random.randint(2000, 10000),
                    cloudiness=random.randint(0, 100),
                    rain_1h=round(random.uniform(0, 3), 1) if weather_main in ('Rain', 'Drizzle') else 0,
                    data_source='manual',
                    created_by=admin.id,
                )
                records.append(record)

            # Every 24 records, commit a batch
            if len(records) >= 200:
                db.session.bulk_save_objects(records)
                db.session.commit()
                records = []

        # Commit remaining
        if records:
            db.session.bulk_save_objects(records)
            db.session.commit()

        total = WeatherRecord.query.count()
        print(f"  ✅ 已生成 {total} 条气象记录")

        print("\n🎉 种子数据创建完成！")
        print("=" * 50)
        print("启动应用: python run.py")
        print("访问: http://localhost:5000/login")
        print("管理员: admin / admin123")


if __name__ == '__main__':
    seed()
