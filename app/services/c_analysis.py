"""
Python 调用 C 语言气象分析引擎的桥接模块。

C 程序路径: analysis/weather_analysis
工作方式: Python 将气象数据导出为 CSV，通过 stdin 传给 C 程序，
         C 程序分析后输出 JSON 结果，Python 解析后使用。
"""

import json
import os
import subprocess
import csv
import io

from app.models.weather_record import WeatherRecord
from app.models.city import City


# C 编译后程序的路径
C_PROGRAM = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))),
                         'analysis', 'weather_analysis')


def run_c_analysis(city_id=None):
    """
    运行 C 语言分析引擎。

    1. 从数据库查询气象记录
    2. 转为 CSV 格式
    3. 通过 stdin 传给 C 程序
    4. 解析 C 程序输出的 JSON 结果

    返回: dict — C 分析引擎的完整结果
    """
    # 1. 查询数据
    query = WeatherRecord.query
    if city_id:
        query = query.filter_by(city_id=city_id)
    records = query.order_by(WeatherRecord.record_time.asc()).all()

    if not records:
        return {'error': '没有数据可供分析'}

    # 2. 转换为 CSV
    csv_buffer = io.StringIO()
    writer = csv.writer(csv_buffer)
    # 写表头
    writer.writerow([
        'city_name', 'record_time', 'temperature', 'feels_like', 'humidity',
        'pressure', 'wind_speed', 'wind_direction', 'weather_main', 'weather_desc'
    ])
    # 写数据
    for r in records:
        writer.writerow([
            r.city.name if r.city else '未知',
            r.record_time.isoformat() if r.record_time else '',
            r.temperature or '',
            r.feels_like or '',
            r.humidity or '',
            r.pressure or '',
            r.wind_speed or '',
            r.wind_direction or '',
            r.weather_main or '',
            r.weather_desc or '',
        ])

    csv_data = csv_buffer.getvalue()

    # 3. 调用 C 程序
    try:
        result = subprocess.run(
            [C_PROGRAM],
            input=csv_data,
            capture_output=True,
            text=True,
            timeout=10,
        )

        if result.returncode != 0:
            return {
                'error': f'C 分析程序异常退出 (code {result.returncode})',
                'stderr': result.stderr,
            }

    except FileNotFoundError:
        return {
            'error': 'C 分析程序未编译，请运行: cd analysis && gcc -o weather_analysis weather_analysis.c -lm'
        }
    except subprocess.TimeoutExpired:
        return {'error': 'C 分析程序运行超时'}

    # 4. 解析 C 输出
    try:
        analysis = json.loads(result.stdout)
    except json.JSONDecodeError as e:
        return {
            'error': f'C 分析结果解析失败: {str(e)}',
            'raw_output': result.stdout[:500],
        }

    # 添加城市元信息
    if analysis.get('cities'):
        for city_stat in analysis['cities']:
            city_obj = City.query.filter_by(name=city_stat['name']).first()
            if city_obj:
                city_stat['latitude'] = city_obj.latitude
                city_stat['longitude'] = city_obj.longitude
                city_stat['owm_city_id'] = city_obj.owm_city_id

    return analysis
