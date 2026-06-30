from flask import Blueprint, request, jsonify, redirect, url_for, flash
from flask_login import login_user, logout_user, login_required, current_user

from app.models.user import User

auth_bp = Blueprint('auth', __name__)


@auth_bp.route('/api/auth/login', methods=['POST'])
def api_login():
    """Login API endpoint. Accepts JSON with username and password."""
    data = request.get_json()
    if not data:
        return jsonify({'error': '请提供登录信息'}), 400

    username = data.get('username', '').strip()
    password = data.get('password', '')

    if not username or not password:
        return jsonify({'error': '用户名和密码不能为空'}), 400

    user = User.query.filter_by(username=username).first()

    if user is None or not user.check_password(password):
        return jsonify({'error': '用户名或密码错误'}), 401

    if not user.is_active:
        return jsonify({'error': '账号已被禁用'}), 403

    login_user(user, remember=data.get('remember', False))
    return jsonify({'message': '登录成功', 'user': user.to_dict()})


@auth_bp.route('/api/auth/logout', methods=['POST'])
@login_required
def api_logout():
    """Logout API endpoint."""
    logout_user()
    return jsonify({'message': '已退出登录'})


@auth_bp.route('/api/auth/me', methods=['GET'])
@login_required
def api_me():
    """Return current user info."""
    return jsonify({'user': current_user.to_dict()})
