from datetime import datetime, timezone

from app.extensions import db


class City(db.Model):
    __tablename__ = 'cities'

    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(120), nullable=False)
    country = db.Column(db.String(80), default='')
    state = db.Column(db.String(80), default='')
    latitude = db.Column(db.Float, nullable=False)
    longitude = db.Column(db.Float, nullable=False)
    owm_city_id = db.Column(db.Integer)  # OpenWeatherMap city ID
    notes = db.Column(db.Text)
    created_at = db.Column(db.DateTime, nullable=False, default=lambda: datetime.now(timezone.utc))

    # Relationships
    weather_records = db.relationship(
        'WeatherRecord', backref='city', lazy='dynamic',
        cascade='all, delete-orphan'
    )

    def to_dict(self):
        return {
            'id': self.id,
            'name': self.name,
            'country': self.country or '',
            'state': self.state or '',
            'latitude': self.latitude,
            'longitude': self.longitude,
            'owm_city_id': self.owm_city_id,
            'notes': self.notes or '',
            'record_count': self.weather_records.count(),
            'created_at': self.created_at.isoformat() if self.created_at else None,
        }

    def __repr__(self):
        return f'<City {self.name}>'
