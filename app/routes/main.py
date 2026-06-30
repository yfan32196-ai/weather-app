from flask import Blueprint, render_template, redirect, url_for, flash
from flask_login import login_required, current_user

from app.models.user import User
from app.utils.decorators import admin_required

main_bp = Blueprint('main', __name__)


@main_bp.route('/')
@login_required
def index():
    """Redirect to dashboard."""
    return redirect(url_for('main.dashboard_page'))


@main_bp.route('/login')
def login_page():
    """Render login page."""
    if current_user.is_authenticated:
        return redirect(url_for('main.dashboard_page'))
    return render_template('login.html')


@main_bp.route('/dashboard')
@login_required
def dashboard_page():
    """Render the main dashboard page."""
    return render_template('dashboard.html')


@main_bp.route('/records')
@login_required
def records_page():
    """Render the weather records management page."""
    return render_template('records.html')


@main_bp.route('/cities')
@login_required
def cities_page():
    """Render the cities management page."""
    return render_template('cities.html')


@main_bp.route('/users')
@login_required
@admin_required
def users_page():
    """Render the user management page (admin only)."""
    return render_template('users.html')


@main_bp.route('/import')
@login_required
def import_page():
    """Render the data import page."""
    if not current_user.is_editor():
        return redirect(url_for('main.dashboard_page'))
    return render_template('import.html')


@main_bp.route('/export')
@login_required
def export_page():
    """Render the data export page."""
    return render_template('export.html')


@main_bp.route('/c-analysis')
@login_required
def c_analysis_page():
    """Render the C language analysis page."""
    return render_template('c_analysis.html')
