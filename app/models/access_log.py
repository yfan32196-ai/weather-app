from datetime import datetime, timezone

from app.extensions import db


class AccessLog(db.Model):
    __tablename__ = 'access_logs'

    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80))
    action = db.Column(db.String(20))        # login / logout
    ip_address = db.Column(db.String(45))
    created_at = db.Column(db.DateTime, nullable=False, default=lambda: datetime.now(timezone.utc))

    def to_dict(self):
        return {
            'id': self.id,
            'username': self.username or '',
            'action': self.action or '',
            'ip_address': self.ip_address or '',
            'created_at': self.created_at.isoformat() if self.created_at else None,
        }
