from functools import wraps

from flask import abort, jsonify
from flask_login import current_user


def admin_required(f):
    """Decorator that requires the current user to be an admin."""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if not current_user.is_authenticated:
            return jsonify({'error': '请先登录'}), 401
        if not current_user.is_admin():
            return jsonify({'error': '需要管理员权限'}), 403
        return f(*args, **kwargs)
    return decorated_function


def editor_required(f):
    """Decorator that requires the current user to be an editor or admin."""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if not current_user.is_authenticated:
            return jsonify({'error': '请先登录'}), 401
        if not current_user.is_editor():
            return jsonify({'error': '需要编辑者以上权限'}), 403
        return f(*args, **kwargs)
    return decorated_function


def api_login_required(f):
    """Decorator for API routes that require login. Returns JSON instead of redirect."""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if not current_user.is_authenticated:
            return jsonify({'error': '请先登录'}), 401
        return f(*args, **kwargs)
    return decorated_function
