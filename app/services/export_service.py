import io
from datetime import datetime

import pandas as pd


class ExportService:
    """Service for exporting weather records to Excel/CSV."""

    # Columns to export, in order
    EXPORT_COLUMNS = [
        'city_name', 'record_time', 'temperature', 'feels_like', 'humidity',
        'pressure', 'wind_speed', 'wind_direction', 'wind_gust',
        'weather_main', 'weather_desc', 'visibility', 'cloudiness',
        'rain_1h', 'snow_1h', 'uv_index', 'data_source', 'notes'
    ]

    # Chinese column headers
    COLUMN_HEADERS = {
        'city_name': '城市',
        'record_time': '记录时间',
        'temperature': '温度(°C)',
        'feels_like': '体感温度(°C)',
        'humidity': '湿度(%)',
        'pressure': '气压(hPa)',
        'wind_speed': '风速(m/s)',
        'wind_direction': '风向',
        'wind_gust': '阵风(m/s)',
        'weather_main': '天气状况',
        'weather_desc': '天气描述',
        'visibility': '能见度(m)',
        'cloudiness': '云量(%)',
        'rain_1h': '降雨量(mm/1h)',
        'snow_1h': '降雪量(mm/1h)',
        'uv_index': '紫外线指数',
        'data_source': '数据来源',
        'notes': '备注',
    }

    def _records_to_dataframe(self, records):
        """Convert query result to a pandas DataFrame."""
        rows = []
        for r in records:
            record_time = r.record_time
            if isinstance(record_time, datetime):
                record_time = record_time.strftime('%Y-%m-%d %H:%M:%S')

            row = {
                'city_name': r.city.name if r.city else '',
                'record_time': record_time,
                'temperature': r.temperature,
                'feels_like': r.feels_like,
                'humidity': r.humidity,
                'pressure': r.pressure,
                'wind_speed': r.wind_speed,
                'wind_direction': r.wind_direction,
                'wind_gust': r.wind_gust,
                'weather_main': r.weather_main,
                'weather_desc': r.weather_desc,
                'visibility': r.visibility,
                'cloudiness': r.cloudiness,
                'rain_1h': r.rain_1h,
                'snow_1h': r.snow_1h,
                'uv_index': r.uv_index,
                'data_source': r.data_source,
                'notes': r.notes,
            }
            rows.append(row)

        df = pd.DataFrame(rows, columns=self.EXPORT_COLUMNS)
        # Rename columns to Chinese headers
        df.rename(columns=self.COLUMN_HEADERS, inplace=True)
        return df

    def export_excel(self, records):
        """Export records to an Excel file.

        Returns:
            tuple: (BytesIO buffer, mime_type)
        """
        df = self._records_to_dataframe(records)
        buffer = io.BytesIO()

        with pd.ExcelWriter(buffer, engine='openpyxl') as writer:
            df.to_excel(writer, sheet_name='气象记录', index=False)
            # Auto-adjust column widths
            worksheet = writer.sheets['气象记录']
            for col_idx, col_name in enumerate(df.columns, 1):
                max_len = max(
                    len(str(col_name)),
                    df[col_name].astype(str).str.len().max() if len(df) > 0 else 0
                )
                worksheet.column_dimensions[chr(64 + col_idx) if col_idx <= 26 else 'A'].width = min(max_len + 4, 30)

        buffer.seek(0)
        return buffer, 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'

    def export_csv(self, records):
        """Export records to a CSV file.

        Returns:
            tuple: (BytesIO buffer, mime_type)
        """
        df = self._records_to_dataframe(records)
        buffer = io.BytesIO()
        df.to_csv(buffer, index=False, encoding='utf-8-sig')
        buffer.seek(0)
        return buffer, 'text/csv'
