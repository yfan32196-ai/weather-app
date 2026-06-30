from datetime import datetime, timezone

from app.extensions import db


class WeatherRecord(db.Model):
    __tablename__ = 'weather_records'

    id = db.Column(db.Integer, primary_key=True)
    city_id = db.Column(db.Integer, db.ForeignKey('cities.id', ondelete='CASCADE'), nullable=False, index=True)
    record_time = db.Column(db.DateTime, nullable=False, index=True)
    temperature = db.Column(db.Float)           # Celsius
    feels_like = db.Column(db.Float)            # Celsius
    humidity = db.Column(db.Integer)            # 0-100 %
    pressure = db.Column(db.Float)              # hPa
    wind_speed = db.Column(db.Float)            # m/s
    wind_direction = db.Column(db.String(10))   # N, NE, E, SE, S, SW, W, NW
    wind_gust = db.Column(db.Float)             # m/s
    weather_main = db.Column(db.String(50))     # Clear, Clouds, Rain, Snow, etc.
    weather_desc = db.Column(db.String(120))
    visibility = db.Column(db.Integer)          # meters
    cloudiness = db.Column(db.Integer)          # 0-100 %
    rain_1h = db.Column(db.Float, default=0)    # mm last 1 hour
    snow_1h = db.Column(db.Float, default=0)    # mm last 1 hour
    uv_index = db.Column(db.Float)
    data_source = db.Column(db.String(30), nullable=False, default='manual',
                            index=True)          # manual, api_openweather, api_hefeng, import
    notes = db.Column(db.Text)
    created_by = db.Column(db.Integer, db.ForeignKey('users.id'))
    created_at = db.Column(db.DateTime, nullable=False, default=lambda: datetime.now(timezone.utc))
    updated_at = db.Column(db.DateTime, onupdate=lambda: datetime.now(timezone.utc))

    __table_args__ = (
        db.Index('idx_weather_city_time', 'city_id', 'record_time'),
        db.Index('idx_weather_source', 'data_source'),
    )

    def to_dict(self):
        return {
            'id': self.id,
            'city_id': self.city_id,
            'city_name': self.city.name if self.city else '',
            'record_time': self.record_time.isoformat() if self.record_time else None,
            'temperature': self.temperature,
            'feels_like': self.feels_like,
            'humidity': self.humidity,
            'pressure': self.pressure,
            'wind_speed': self.wind_speed,
            'wind_direction': self.wind_direction or '',
            'wind_gust': self.wind_gust,
            'weather_main': self.weather_main or '',
            'weather_desc': self.weather_desc or '',
            'visibility': self.visibility,
            'cloudiness': self.cloudiness,
            'rain_1h': self.rain_1h or 0,
            'snow_1h': self.snow_1h or 0,
            'uv_index': self.uv_index,
            'data_source': self.data_source,
            'notes': self.notes or '',
            'created_by': self.created_by,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'updated_at': self.updated_at.isoformat() if self.updated_at else None,
        }

    def __repr__(self):
        return f'<WeatherRecord {self.city.name if self.city else "?"} @ {self.record_time}>'
