from datetime import datetime, timezone

from app.extensions import db


class ImportLog(db.Model):
    __tablename__ = 'import_logs'

    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'), nullable=False)
    filename = db.Column(db.String(256), nullable=False)
    record_count = db.Column(db.Integer, default=0)
    skipped_count = db.Column(db.Integer, default=0)
    status = db.Column(db.String(20), nullable=False, default='pending')
    # status: pending, processing, success, partial, failed
    error_detail = db.Column(db.Text)
    created_at = db.Column(db.DateTime, nullable=False, default=lambda: datetime.now(timezone.utc))

    def to_dict(self):
        return {
            'id': self.id,
            'user_id': self.user_id,
            'username': self.user.username if self.user else '',
            'filename': self.filename,
            'record_count': self.record_count,
            'skipped_count': self.skipped_count,
            'status': self.status,
            'error_detail': self.error_detail or '',
            'created_at': self.created_at.isoformat() if self.created_at else None,
        }

    def __repr__(self):
        return f'<ImportLog {self.filename} ({self.status})>'
