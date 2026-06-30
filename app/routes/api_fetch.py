from flask import Blueprint, request, jsonify
from flask_login import login_required, current_user

from app.extensions import db
from app.models.city import City
from app.models.weather_record import WeatherRecord
from app.services.weather_api import WeatherAPIService
from app.utils.decorators import editor_required

api_fetch_bp = Blueprint('api_fetch', __name__, url_prefix='/api/weather')


@api_fetch_bp.route('/current/<int:city_id>', methods=['GET'])
@login_required
def get_current_weather(city_id):
    """Fetch current weather for a city from API (does NOT save to DB)."""
    city = db.session.get(City, city_id)
    if not city:
        return jsonify({'error': '城市不存在'}), 404

    try:
        service = WeatherAPIService()
        weather_data = service.get_current_weather(city)
        return jsonify({'weather': weather_data})
    except ValueError as e:
        return jsonify({'error': str(e)}), 400
    except Exception as e:
        return jsonify({'error': f'获取天气数据失败: {str(e)}'}), 500


@api_fetch_bp.route('/fetch', methods=['POST'])
@login_required
@editor_required
def fetch_and_save():
    """Fetch current weather from API and save as a new record."""
    data = request.get_json()
    if not data or not data.get('city_id'):
        return jsonify({'error': '请指定城市'}), 400

    city_id = data['city_id']
    city = db.session.get(City, city_id)
    if not city:
        return jsonify({'error': '城市不存在'}), 404

    try:
        service = WeatherAPIService()
        weather_data = service.get_current_weather(city)

        record = WeatherRecord(
            city_id=city.id,
            record_time=weather_data['record_time'],
            temperature=weather_data.get('temperature'),
            feels_like=weather_data.get('feels_like'),
            humidity=weather_data.get('humidity'),
            pressure=weather_data.get('pressure'),
            wind_speed=weather_data.get('wind_speed'),
            wind_direction=weather_data.get('wind_direction'),
            wind_gust=weather_data.get('wind_gust'),
            weather_main=weather_data.get('weather_main'),
            weather_desc=weather_data.get('weather_desc'),
            visibility=weather_data.get('visibility'),
            cloudiness=weather_data.get('cloudiness'),
            rain_1h=weather_data.get('rain_1h', 0),
            snow_1h=weather_data.get('snow_1h', 0),
            uv_index=weather_data.get('uv_index'),
            data_source='api_openweather',
            created_by=current_user.id,
        )
        db.session.add(record)
        db.session.commit()

        return jsonify({'message': f'已从 API 获取 {city.name} 的实时天气', 'record': record.to_dict()}), 201
    except ValueError as e:
        return jsonify({'error': str(e)}), 400
    except Exception as e:
        return jsonify({'error': f'获取天气数据失败: {str(e)}'}), 500
