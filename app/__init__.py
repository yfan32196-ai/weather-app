import os

from flask import Flask

from app.config import config
from app.extensions import db, login_manager, migrate


def create_app(config_name=None):
    """Application factory function."""
    if config_name is None:
        config_name = os.environ.get('FLASK_CONFIG', 'default')

    app = Flask(__name__)
    app.config.from_object(config[config_name])

    # Ensure instance and upload folders exist
    os.makedirs(app.instance_path, exist_ok=True)
    os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)

    # Initialize extensions
    db.init_app(app)
    login_manager.init_app(app)
    migrate.init_app(app, db)

    # Register user loader
    from app.models.user import User

    @login_manager.user_loader
    def load_user(user_id):
        return db.session.get(User, int(user_id))

    # Register blueprints
    from app.routes.auth import auth_bp
    from app.routes.main import main_bp
    from app.routes.api_users import api_users_bp
    from app.routes.api_cities import api_cities_bp
    from app.routes.api_weather import api_weather_bp
    from app.routes.api_fetch import api_fetch_bp
    from app.routes.api_stats import api_stats_bp

    app.register_blueprint(auth_bp)
    app.register_blueprint(main_bp)
    app.register_blueprint(api_users_bp)
    app.register_blueprint(api_cities_bp)
    app.register_blueprint(api_weather_bp)
    app.register_blueprint(api_fetch_bp)
    app.register_blueprint(api_stats_bp)

    # Import/export blueprint registered separately in routes
    from app.routes.api_import_export import import_export_bp
    app.register_blueprint(import_export_bp)

    # C analysis blueprint
    from app.routes.api_c_analysis import api_c_analysis_bp
    app.register_blueprint(api_c_analysis_bp)

    return app
