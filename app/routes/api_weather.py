from datetime import datetime, timezone

from flask import Blueprint, request, jsonify
from flask_login import login_required, current_user

from app.extensions import db
from app.models.weather_record import WeatherRecord
from app.models.city import City
from app.utils.decorators import editor_required, admin_required, api_login_required

api_weather_bp = Blueprint('api_weather', __name__, url_prefix='/api/weather')


def _parse_datetime(s):
    """Parse ISO datetime string."""
    if not s:
        return None
    try:
        return datetime.fromisoformat(s.replace('Z', '+00:00'))
    except (ValueError, TypeError):
        return None


@api_weather_bp.route('/records', methods=['GET'])
@login_required
def list_records():
    """List weather records with optional filtering and pagination."""
    page = request.args.get('page', 1, type=int)
    per_page = request.args.get('per_page', 20, type=int)
    city_id = request.args.get('city_id', type=int)
    date_from = request.args.get('date_from')
    date_to = request.args.get('date_to')
    source = request.args.get('source')

    query = WeatherRecord.query

    if city_id:
        query = query.filter_by(city_id=city_id)
    if date_from:
        dt = _parse_datetime(date_from)
        if dt:
            query = query.filter(WeatherRecord.record_time >= dt)
    if date_to:
        dt = _parse_datetime(date_to)
        if dt:
            query = query.filter(WeatherRecord.record_time <= dt)
    if source:
        query = query.filter_by(data_source=source)

    query = query.order_by(WeatherRecord.record_time.desc())

    pagination = query.paginate(page=page, per_page=per_page, error_out=False)
    records = [r.to_dict() for r in pagination.items]

    return jsonify({
        'records': records,
        'page': pagination.page,
        'per_page': pagination.per_page,
        'total': pagination.total,
        'pages': pagination.pages,
    })


@api_weather_bp.route('/records/<int:record_id>', methods=['GET'])
@login_required
def get_record(record_id):
    """Get a single weather record."""
    record = db.session.get(WeatherRecord, record_id)
    if not record:
        return jsonify({'error': '记录不存在'}), 404
    return jsonify({'record': record.to_dict()})


@api_weather_bp.route('/records', methods=['POST'])
@login_required
@editor_required
def create_record():
    """Create a new weather record manually."""
    data = request.get_json()
    if not data:
        return jsonify({'error': '请提供气象数据'}), 400

    city_id = data.get('city_id')
    if not city_id:
        return jsonify({'error': '请选择城市'}), 400

    city = db.session.get(City, city_id)
    if not city:
        return jsonify({'error': '城市不存在'}), 404

    record_time = _parse_datetime(data.get('record_time')) or datetime.now(timezone.utc)

    record = WeatherRecord(
        city_id=city_id,
        record_time=record_time,
        temperature=data.get('temperature'),
        feels_like=data.get('feels_like'),
        humidity=data.get('humidity'),
        pressure=data.get('pressure'),
        wind_speed=data.get('wind_speed'),
        wind_direction=data.get('wind_direction', '').strip() or None,
        wind_gust=data.get('wind_gust'),
        weather_main=data.get('weather_main', '').strip() or None,
        weather_desc=data.get('weather_desc', '').strip() or None,
        visibility=data.get('visibility'),
        cloudiness=data.get('cloudiness'),
        rain_1h=data.get('rain_1h', 0),
        snow_1h=data.get('snow_1h', 0),
        uv_index=data.get('uv_index'),
        data_source='manual',
        notes=data.get('notes', '').strip() or None,
        created_by=current_user.id,
    )
    db.session.add(record)
    db.session.commit()

    return jsonify({'message': '记录创建成功', 'record': record.to_dict()}), 201


@api_weather_bp.route('/records/<int:record_id>', methods=['PUT'])
@login_required
@editor_required
def update_record(record_id):
    """Update a weather record."""
    record = db.session.get(WeatherRecord, record_id)
    if not record:
        return jsonify({'error': '记录不存在'}), 404

    data = request.get_json()
    if not data:
        return jsonify({'error': '请提供更新信息'}), 400

    updatable_fields = [
        'temperature', 'feels_like', 'humidity', 'pressure',
        'wind_speed', 'wind_direction', 'wind_gust',
        'weather_main', 'weather_desc', 'visibility', 'cloudiness',
        'rain_1h', 'snow_1h', 'uv_index', 'notes'
    ]

    for field in updatable_fields:
        if field in data:
            val = data[field]
            if isinstance(val, str):
                val = val.strip() or None
            setattr(record, field, val)

    if 'record_time' in data:
        dt = _parse_datetime(data['record_time'])
        if dt:
            record.record_time = dt

    if 'city_id' in data:
        city = db.session.get(City, data['city_id'])
        if not city:
            return jsonify({'error': '城市不存在'}), 404
        record.city_id = data['city_id']

    record.updated_at = datetime.now(timezone.utc)
    db.session.commit()

    return jsonify({'message': '记录更新成功', 'record': record.to_dict()})


@api_weather_bp.route('/records/<int:record_id>', methods=['DELETE'])
@login_required
@admin_required
def delete_record(record_id):
    """Delete a weather record (admin only)."""
    record = db.session.get(WeatherRecord, record_id)
    if not record:
        return jsonify({'error': '记录不存在'}), 404

    db.session.delete(record)
    db.session.commit()
    return jsonify({'message': '记录已删除'})
