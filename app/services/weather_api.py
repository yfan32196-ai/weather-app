import time
from datetime import datetime, timezone

import requests
from flask import current_app


class WeatherAPIService:
    """Client for OpenWeatherMap API."""

    BASE_URL = 'https://api.openweathermap.org/data/2.5/weather'

    # Simple in-memory rate limiter: track last call timestamp
    _last_call_time = 0
    _min_interval = 1.2  # seconds between calls (60 calls/min = 1s, add buffer)

    # Simple in-memory cache for current weather (10 minutes TTL)
    _cache = {}
    _cache_ttl = 600  # seconds

    def __init__(self):
        self.api_key = current_app.config.get('OWM_API_KEY', '')
        if not self.api_key:
            raise ValueError(
                'OpenWeatherMap API Key 未配置。'
                '请设置环境变量 OWM_API_KEY=your_api_key'
            )

    def _rate_limit(self):
        """Simple rate limiter to respect API limits."""
        now = time.time()
        elapsed = now - self._last_call_time
        if elapsed < self._min_interval:
            time.sleep(self._min_interval - elapsed)
        self._last_call_time = time.time()

    def _cache_key(self, city):
        return f"city_{city.id}"

    def _get_cached(self, city):
        key = self._cache_key(city)
        if key in self._cache:
            data, timestamp = self._cache[key]
            if time.time() - timestamp < self._cache_ttl:
                return data
        return None

    def _set_cache(self, city, data):
        key = self._cache_key(city)
        self._cache[key] = (data, time.time())

    def get_current_weather(self, city):
        """Fetch current weather for a city.

        Args:
            city: City model instance with owm_city_id and/or lat/lon.

        Returns:
            dict with normalized weather data matching WeatherRecord fields.

        Raises:
            ValueError: if API key is missing.
            requests.RequestException: on network/API errors.
        """
        # Check cache
        cached = self._get_cached(city)
        if cached:
            return cached

        # Build request params
        params = {
            'appid': self.api_key,
            'units': 'metric',   # Celsius
            'lang': 'zh_cn',     # Chinese descriptions
        }

        if city.owm_city_id:
            params['id'] = city.owm_city_id
        else:
            params['lat'] = city.latitude
            params['lon'] = city.longitude

        self._rate_limit()

        try:
            resp = requests.get(self.BASE_URL, params=params, timeout=10)
            resp.raise_for_status()
        except requests.HTTPError as e:
            if resp.status_code == 401:
                raise ValueError('API Key 无效或未激活')
            elif resp.status_code == 404:
                raise ValueError(f'未找到城市 "{city.name}" 的天气数据')
            elif resp.status_code == 429:
                raise ValueError('API 调用频率超限，请稍后再试')
            raise

        data = resp.json()

        # Normalize to our WeatherRecord schema
        wind_direction = ''
        wind_deg = data.get('wind', {}).get('deg')
        if wind_deg is not None:
            directions = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW']
            idx = round(wind_deg / 45) % 8
            wind_direction = directions[idx]

        weather_list = data.get('weather', [{}])
        weather_main = weather_list[0].get('main', '') if weather_list else ''
        weather_desc = weather_list[0].get('description', '') if weather_list else ''

        rain = data.get('rain', {})
        snow = data.get('snow', {})

        result = {
            'record_time': datetime.fromtimestamp(data['dt'], tz=timezone.utc),
            'temperature': data['main'].get('temp'),
            'feels_like': data['main'].get('feels_like'),
            'humidity': data['main'].get('humidity'),
            'pressure': data['main'].get('pressure'),
            'wind_speed': data.get('wind', {}).get('speed'),
            'wind_direction': wind_direction,
            'wind_gust': data.get('wind', {}).get('gust'),
            'weather_main': weather_main,
            'weather_desc': weather_desc,
            'visibility': data.get('visibility'),
            'cloudiness': data.get('clouds', {}).get('all'),
            'rain_1h': rain.get('1h', 0),
            'snow_1h': snow.get('1h', 0),
            'uv_index': None,  # Not available in free tier current weather
            'raw_response': data,  # Keep raw response for debugging
        }

        # Cache the result
        self._set_cache(city, result)
        return result
