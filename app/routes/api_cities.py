from flask import Blueprint, request, jsonify
from flask_login import login_required, current_user

from app.extensions import db
from app.models.city import City
from app.utils.decorators import editor_required, admin_required, api_login_required

api_cities_bp = Blueprint('api_cities', __name__, url_prefix='/api/cities')


@api_cities_bp.route('', methods=['GET'])
@login_required
def list_cities():
    """List all cities."""
    cities = City.query.order_by(City.name).all()
    return jsonify({'cities': [c.to_dict() for c in cities]})


@api_cities_bp.route('/<int:city_id>', methods=['GET'])
@login_required
def get_city(city_id):
    """Get a single city."""
    city = db.session.get(City, city_id)
    if not city:
        return jsonify({'error': '城市不存在'}), 404
    return jsonify({'city': city.to_dict()})


@api_cities_bp.route('', methods=['POST'])
@login_required
@editor_required
def create_city():
    """Create a new city."""
    data = request.get_json()
    if not data:
        return jsonify({'error': '请提供城市信息'}), 400

    name = data.get('name', '').strip()
    if not name:
        return jsonify({'error': '城市名称不能为空'}), 400

    try:
        latitude = float(data.get('latitude', 0))
        longitude = float(data.get('longitude', 0))
    except (TypeError, ValueError):
        return jsonify({'error': '经纬度必须是数字'}), 400

    city = City(
        name=name,
        country=data.get('country', '').strip(),
        state=data.get('state', '').strip(),
        latitude=latitude,
        longitude=longitude,
        owm_city_id=data.get('owm_city_id') or None,
        notes=data.get('notes', '').strip() or None,
    )
    db.session.add(city)
    db.session.commit()

    return jsonify({'message': '城市创建成功', 'city': city.to_dict()}), 201


@api_cities_bp.route('/<int:city_id>', methods=['PUT'])
@login_required
@editor_required
def update_city(city_id):
    """Update a city."""
    city = db.session.get(City, city_id)
    if not city:
        return jsonify({'error': '城市不存在'}), 404

    data = request.get_json()
    if not data:
        return jsonify({'error': '请提供更新信息'}), 400

    if 'name' in data:
        name = data['name'].strip()
        if not name:
            return jsonify({'error': '城市名称不能为空'}), 400
        city.name = name
    if 'country' in data:
        city.country = data['country'].strip()
    if 'state' in data:
        city.state = data['state'].strip()
    if 'latitude' in data:
        try:
            city.latitude = float(data['latitude'])
        except (TypeError, ValueError):
            return jsonify({'error': '纬度必须是数字'}), 400
    if 'longitude' in data:
        try:
            city.longitude = float(data['longitude'])
        except (TypeError, ValueError):
            return jsonify({'error': '经度必须是数字'}), 400
    if 'owm_city_id' in data:
        city.owm_city_id = data['owm_city_id'] or None
    if 'notes' in data:
        city.notes = data['notes'].strip() or None

    db.session.commit()
    return jsonify({'message': '城市更新成功', 'city': city.to_dict()})


@api_cities_bp.route('/<int:city_id>', methods=['DELETE'])
@login_required
@admin_required
def delete_city(city_id):
    """Delete a city (admin only, cascades weather records)."""
    city = db.session.get(City, city_id)
    if not city:
        return jsonify({'error': '城市不存在'}), 404

    db.session.delete(city)
    db.session.commit()
    return jsonify({'message': f'城市 "{city.name}" 已删除'})
