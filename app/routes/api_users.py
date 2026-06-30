from flask import Blueprint, request, jsonify
from flask_login import login_required, current_user

from app.extensions import db
from app.models.user import User
from app.utils.decorators import admin_required

api_users_bp = Blueprint('api_users', __name__, url_prefix='/api/users')


@api_users_bp.route('', methods=['GET'])
@login_required
@admin_required
def list_users():
    """List all users."""
    users = User.query.order_by(User.created_at.desc()).all()
    return jsonify({'users': [u.to_dict() for u in users]})


@api_users_bp.route('', methods=['POST'])
@login_required
@admin_required
def create_user():
    """Create a new user."""
    data = request.get_json()
    if not data:
        return jsonify({'error': '请提供用户信息'}), 400

    username = data.get('username', '').strip()
    password = data.get('password', '')
    role = data.get('role', 'viewer').strip()

    if not username or not password:
        return jsonify({'error': '用户名和密码不能为空'}), 400

    if User.query.filter_by(username=username).first():
        return jsonify({'error': '用户名已存在'}), 409

    if role not in ('admin', 'editor', 'viewer'):
        return jsonify({'error': '无效的角色'}), 400

    user = User(
        username=username,
        role=role,
        email=data.get('email', '').strip() or None,
        is_active=data.get('is_active', True)
    )
    user.set_password(password)
    db.session.add(user)
    db.session.commit()

    return jsonify({'message': '用户创建成功', 'user': user.to_dict()}), 201


@api_users_bp.route('/<int:user_id>', methods=['PUT'])
@login_required
@admin_required
def update_user(user_id):
    """Update a user."""
    user = db.session.get(User, user_id)
    if not user:
        return jsonify({'error': '用户不存在'}), 404

    data = request.get_json()
    if not data:
        return jsonify({'error': '请提供更新信息'}), 400

    # Update username (check uniqueness)
    new_username = data.get('username', '').strip()
    if new_username and new_username != user.username:
        if User.query.filter_by(username=new_username).first():
            return jsonify({'error': '用户名已存在'}), 409
        user.username = new_username

    # Update password if provided
    new_password = data.get('password', '')
    if new_password:
        user.set_password(new_password)

    # Update role
    new_role = data.get('role', '').strip()
    if new_role:
        if new_role not in ('admin', 'editor', 'viewer'):
            return jsonify({'error': '无效的角色'}), 400
        user.role = new_role

    # Update other fields
    if 'email' in data:
        user.email = data['email'].strip() or None
    if 'is_active' in data:
        user.is_active = data['is_active']

    db.session.commit()
    return jsonify({'message': '用户更新成功', 'user': user.to_dict()})


@api_users_bp.route('/<int:user_id>', methods=['DELETE'])
@login_required
@admin_required
def delete_user(user_id):
    """Delete a user."""
    if user_id == current_user.id:
        return jsonify({'error': '不能删除自己'}), 400

    user = db.session.get(User, user_id)
    if not user:
        return jsonify({'error': '用户不存在'}), 404

    # Prevent deleting the last admin
    if user.is_admin():
        admin_count = User.query.filter_by(role='admin').count()
        if admin_count <= 1:
            return jsonify({'error': '不能删除最后一个管理员'}), 400

    db.session.delete(user)
    db.session.commit()
    return jsonify({'message': '用户已删除'})
