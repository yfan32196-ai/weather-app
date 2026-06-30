from datetime import datetime, timedelta, timezone

from flask import Blueprint, request, jsonify
from flask_login import login_required
from sqlalchemy import func

from app.extensions import db
from app.models.weather_record import WeatherRecord
from app.models.city import City

api_stats_bp = Blueprint('api_stats', __name__, url_prefix='/api/stats')


def _get_date_range(days):
    """Calculate start date based on requested days."""
    try:
        days = int(days)
    except (TypeError, ValueError):
        days = 7
    days = max(1, min(days, 365))
    end = datetime.now(timezone.utc)
    start = end - timedelta(days=days)
    return start, end, days


@api_stats_bp.route('/temperature', methods=['GET'])
@login_required
def temperature_stats():
    """Get temperature time-series data."""
    city_id = request.args.get('city_id', type=int)
    days = request.args.get('days', 7)
    start, end, days = _get_date_range(days)

    query = WeatherRecord.query.filter(
        WeatherRecord.record_time >= start,
        WeatherRecord.record_time <= end
    )
    if city_id:
        query = query.filter_by(city_id=city_id)

    records = query.order_by(WeatherRecord.record_time.asc()).all()

    data = [
        {
            'time': r.record_time.isoformat(),
            'temperature': r.temperature,
            'feels_like': r.feels_like,
        }
        for r in records
    ]
    return jsonify({'data': data})


@api_stats_bp.route('/humidity', methods=['GET'])
@login_required
def humidity_stats():
    """Get humidity time-series data."""
    city_id = request.args.get('city_id', type=int)
    days = request.args.get('days', 7)
    start, end, days = _get_date_range(days)

    query = WeatherRecord.query.filter(
        WeatherRecord.record_time >= start,
        WeatherRecord.record_time <= end
    )
    if city_id:
        query = query.filter_by(city_id=city_id)

    records = query.order_by(WeatherRecord.record_time.asc()).all()

    data = [
        {
            'time': r.record_time.isoformat(),
            'humidity': r.humidity,
            'pressure': r.pressure,
        }
        for r in records
    ]
    return jsonify({'data': data})


@api_stats_bp.route('/wind', methods=['GET'])
@login_required
def wind_stats():
    """Get wind time-series data."""
    city_id = request.args.get('city_id', type=int)
    days = request.args.get('days', 7)
    start, end, days = _get_date_range(days)

    query = WeatherRecord.query.filter(
        WeatherRecord.record_time >= start,
        WeatherRecord.record_time <= end
    )
    if city_id:
        query = query.filter_by(city_id=city_id)

    records = query.order_by(WeatherRecord.record_time.asc()).all()

    data = [
        {
            'time': r.record_time.isoformat(),
            'wind_speed': r.wind_speed,
            'wind_direction': r.wind_direction,
            'wind_gust': r.wind_gust,
        }
        for r in records
    ]
    return jsonify({'data': data})


@api_stats_bp.route('/summary', methods=['GET'])
@login_required
def summary_stats():
    """Get aggregated summary statistics."""
    city_id = request.args.get('city_id', type=int)
    days = request.args.get('days', 7)
    start, end, days = _get_date_range(days)

    stats = db.session.query(
        func.avg(WeatherRecord.temperature).label('avg_temp'),
        func.max(WeatherRecord.temperature).label('max_temp'),
        func.min(WeatherRecord.temperature).label('min_temp'),
        func.avg(WeatherRecord.humidity).label('avg_humidity'),
        func.avg(WeatherRecord.wind_speed).label('avg_wind'),
        func.max(WeatherRecord.wind_speed).label('max_wind'),
        func.avg(WeatherRecord.pressure).label('avg_pressure'),
        func.count(WeatherRecord.id).label('record_count'),
    ).filter(
        WeatherRecord.record_time >= start,
        WeatherRecord.record_time <= end
    )
    if city_id:
        stats = stats.filter_by(city_id=city_id)

    row = stats.first()

    city_name = ''
    if city_id:
        city = db.session.get(City, city_id)
        if city:
            city_name = city.name

    return jsonify({
        'summary': {
            'city_name': city_name,
            'days': days,
            'record_count': row.record_count or 0,
            'avg_temperature': round(row.avg_temp, 1) if row.avg_temp else None,
            'max_temperature': round(row.max_temp, 1) if row.max_temp else None,
            'min_temperature': round(row.min_temp, 1) if row.min_temp else None,
            'avg_humidity': round(row.avg_humidity, 1) if row.avg_humidity else None,
            'avg_wind_speed': round(row.avg_wind, 2) if row.avg_wind else None,
            'max_wind_speed': round(row.max_wind, 2) if row.max_wind else None,
            'avg_pressure': round(row.avg_pressure, 1) if row.avg_pressure else None,
        }
    })
