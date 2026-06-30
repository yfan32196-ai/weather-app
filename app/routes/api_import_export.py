import os

from flask import Blueprint, request, jsonify, send_file
from flask_login import login_required, current_user

from app.extensions import db
from app.models.import_log import ImportLog
from app.models.weather_record import WeatherRecord
from app.services.import_service import ImportService
from app.services.export_service import ExportService
from app.utils.decorators import editor_required

import_export_bp = Blueprint('import_export', __name__, url_prefix='/api')


@import_export_bp.route('/import/upload', methods=['POST'])
@login_required
@editor_required
def upload_import():
    """Handle file upload for data import."""
    if 'file' not in request.files:
        return jsonify({'error': '请选择文件'}), 400

    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': '请选择文件'}), 400

    # Validate file extension
    ext = os.path.splitext(file.filename)[1].lower()
    if ext not in ('.xlsx', '.xls', '.csv'):
        return jsonify({'error': '仅支持 .xlsx、.xls 和 .csv 格式'}), 400

    # Save uploaded file
    upload_folder = db.get_app().config['UPLOAD_FOLDER']
    import uuid
    safe_name = f"{uuid.uuid4().hex}_{file.filename}"
    filepath = os.path.join(upload_folder, safe_name)
    file.save(filepath)

    # Create import log
    import_log = ImportLog(
        user_id=current_user.id,
        filename=file.filename,
        status='processing'
    )
    db.session.add(import_log)
    db.session.commit()

    try:
        service = ImportService()
        success, skipped, errors = service.process(filepath, current_user.id)

        import_log.record_count = success
        import_log.skipped_count = skipped
        import_log.status = 'success' if skipped == 0 and errors == 0 else ('partial' if success > 0 else 'failed')
        if errors:
            import_log.error_detail = '\n'.join(errors[:50])  # Keep first 50 errors
        db.session.commit()

        return jsonify({
            'message': f'导入完成：成功 {success} 条，跳过 {skipped} 条',
            'import_log': import_log.to_dict(),
        })
    except Exception as e:
        import_log.status = 'failed'
        import_log.error_detail = str(e)
        db.session.commit()
        return jsonify({'error': f'导入失败: {str(e)}'}), 500
    finally:
        # Clean up temp file
        try:
            os.remove(filepath)
        except OSError:
            pass


@import_export_bp.route('/import/logs', methods=['GET'])
@login_required
def list_import_logs():
    """List import history."""
    logs = ImportLog.query.order_by(ImportLog.created_at.desc()).limit(50).all()
    return jsonify({'logs': [log.to_dict() for log in logs]})


@import_export_bp.route('/export/excel', methods=['GET'])
@login_required
def export_excel():
    """Export weather records as Excel file."""
    return _do_export('excel')


@import_export_bp.route('/export/csv', methods=['GET'])
@login_required
def export_csv():
    """Export weather records as CSV file."""
    return _do_export('csv')


def _do_export(format_type):
    """Common export logic."""
    city_id = request.args.get('city_id', type=int)
    date_from = request.args.get('date_from')
    date_to = request.args.get('date_to')
    source = request.args.get('source')

    query = WeatherRecord.query

    if city_id:
        query = query.filter_by(city_id=city_id)
    if date_from:
        from datetime import datetime
        try:
            dt = datetime.fromisoformat(date_from.replace('Z', '+00:00'))
            query = query.filter(WeatherRecord.record_time >= dt)
        except (ValueError, TypeError):
            pass
    if date_to:
        from datetime import datetime
        try:
            dt = datetime.fromisoformat(date_to.replace('Z', '+00:00'))
            query = query.filter(WeatherRecord.record_time <= dt)
        except (ValueError, TypeError):
            pass
    if source:
        query = query.filter_by(data_source=source)

    records = query.order_by(WeatherRecord.record_time.desc()).all()

    service = ExportService()
    if format_type == 'excel':
        buffer, mimetype = service.export_excel(records)
        filename = 'weather_records.xlsx'
    else:
        buffer, mimetype = service.export_csv(records)
        filename = 'weather_records.csv'

    buffer.seek(0)
    return send_file(
        buffer,
        mimetype=mimetype,
        as_attachment=True,
        download_name=filename,
    )
