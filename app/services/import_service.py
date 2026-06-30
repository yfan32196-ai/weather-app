import os
from datetime import datetime, timezone

import pandas as pd

from app.extensions import db
from app.models.city import City
from app.models.weather_record import WeatherRecord


class ImportService:
    """Service for parsing and importing weather data from Excel/CSV files."""

    # Required columns for import
    REQUIRED_COLUMNS = {'city_name', 'record_time'}
    # All recognized columns
    RECOGNIZED_COLUMNS = {
        'city_name', 'record_time', 'temperature', 'feels_like', 'humidity',
        'pressure', 'wind_speed', 'wind_direction', 'wind_gust',
        'weather_main', 'weather_desc', 'visibility', 'cloudiness',
        'rain_1h', 'snow_1h', 'uv_index', 'notes'
    }

    def process(self, filepath, user_id):
        """Process an uploaded file and import records.

        Args:
            filepath: Path to the uploaded file.
            user_id: ID of the user performing the import.

        Returns:
            tuple: (success_count, skipped_count, error_messages_list)
        """
        # Read file
        ext = os.path.splitext(filepath)[1].lower()
        try:
            if ext == '.csv':
                df = self._read_csv(filepath)
            else:
                df = pd.read_excel(filepath, engine='openpyxl')
        except Exception as e:
            return 0, 0, [f'文件读取失败: {str(e)}']

        if df.empty:
            return 0, 0, ['文件中没有数据']

        # Validate columns
        columns = set(df.columns)
        missing = self.REQUIRED_COLUMNS - columns
        if missing:
            return 0, 0, [f'缺少必要列: {", ".join(sorted(missing))}']

        # Pre-load city name -> ID mapping
        cities = {c.name.lower(): c.id for c in City.query.all()}

        # Also try matching without "市" suffix
        cities_alt = {}
        for c in City.query.all():
            base_name = c.name.rstrip('市')
            if base_name.lower() not in cities:
                cities_alt[base_name.lower()] = c.id

        success = 0
        skipped = 0
        errors = []

        records_to_insert = []

        for idx, row in df.iterrows():
            row_num = idx + 2  # Excel row number (1-indexed + header row)

            # Resolve city
            city_name = str(row.get('city_name', '')).strip()
            city_id = cities.get(city_name.lower()) or cities_alt.get(city_name.lower())

            if not city_id:
                skipped += 1
                errors.append(f'第{row_num}行: 未找到城市 "{city_name}"，请先在系统中添加该城市')
                continue

            # Parse record_time
            record_time = self._parse_datetime(row.get('record_time'))
            if record_time is None:
                skipped += 1
                errors.append(f'第{row_num}行: 时间格式无效')
                continue

            # Build record
            record = {
                'city_id': city_id,
                'record_time': record_time,
                'temperature': self._safe_float(row.get('temperature')),
                'feels_like': self._safe_float(row.get('feels_like')),
                'humidity': self._safe_int(row.get('humidity')),
                'pressure': self._safe_float(row.get('pressure')),
                'wind_speed': self._safe_float(row.get('wind_speed')),
                'wind_direction': str(row.get('wind_direction', '')).strip() or None,
                'wind_gust': self._safe_float(row.get('wind_gust')),
                'weather_main': str(row.get('weather_main', '')).strip() or None,
                'weather_desc': str(row.get('weather_desc', '')).strip() or None,
                'visibility': self._safe_int(row.get('visibility')),
                'cloudiness': self._safe_int(row.get('cloudiness')),
                'rain_1h': self._safe_float(row.get('rain_1h', 0)) or 0,
                'snow_1h': self._safe_float(row.get('snow_1h', 0)) or 0,
                'uv_index': self._safe_float(row.get('uv_index')),
                'notes': str(row.get('notes', '')).strip() or None,
                'data_source': 'import',
                'created_by': user_id,
            }
            records_to_insert.append(record)
            success += 1

        # Bulk insert in batches
        if records_to_insert:
            batch_size = 500
            for i in range(0, len(records_to_insert), batch_size):
                batch = records_to_insert[i:i + batch_size]
                db.session.execute(
                    WeatherRecord.__table__.insert(),
                    batch
                )
            db.session.commit()

        return success, skipped, errors

    def _read_csv(self, filepath):
        """Read CSV file, trying multiple encodings."""
        for encoding in ['utf-8', 'utf-8-sig', 'gbk', 'gb2312', 'gb18030']:
            try:
                return pd.read_csv(filepath, encoding=encoding)
            except (UnicodeDecodeError, UnicodeError):
                continue
        # Last attempt: try utf-8 with error replacement
        return pd.read_csv(filepath, encoding='utf-8', encoding_errors='replace')

    def _parse_datetime(self, val):
        """Try to parse a datetime value from various formats."""
        if val is None:
            return None
        if isinstance(val, datetime):
            return val
        if isinstance(val, pd.Timestamp):
            return val.to_pydatetime()

        val_str = str(val).strip()
        if not val_str:
            return None

        # Try common formats
        formats = [
            '%Y-%m-%d %H:%M:%S',
            '%Y-%m-%d %H:%M',
            '%Y/%m/%d %H:%M:%S',
            '%Y/%m/%d %H:%M',
            '%Y-%m-%dT%H:%M:%S',
            '%Y-%m-%d',
        ]
        for fmt in formats:
            try:
                return datetime.strptime(val_str, fmt)
            except ValueError:
                continue
        return None

    def _safe_float(self, val):
        """Safely convert to float, returning None on failure."""
        if val is None:
            return None
        try:
            v = float(val)
            return v if pd.notna(val) else None
        except (ValueError, TypeError):
            return None

    def _safe_int(self, val):
        """Safely convert to int, returning None on failure."""
        if val is None:
            return None
        try:
            v = float(val)
            return int(v) if pd.notna(val) else None
        except (ValueError, TypeError):
            return None
