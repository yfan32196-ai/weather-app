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

    # Auto-initialize database and default data
    with app.app_context():
        db.create_all()
        from app.models.user import User
        from app.models.city import City
        if User.query.count() == 0:
            admin = User(username='admin', role='admin', email='admin@weather.local')
            admin.set_password('admin123')
            viewer = User(username='viewer', role='viewer', email='viewer@weather.local')
            viewer.set_password('viewer123')
            db.session.add_all([admin, viewer])
            db.session.commit()
            print("Default users created")
        if City.query.count() == 0:
            cities = [
                City(name='北京', country='中国', state='北京', latitude=39.9042, longitude=116.4074, owm_city_id=1816670),
                City(name='上海', country='中国', state='上海', latitude=31.2304, longitude=121.4737, owm_city_id=1796236),
                City(name='广州', country='中国', state='广东', latitude=23.1291, longitude=113.2644, owm_city_id=1809106),
                City(name='深圳', country='中国', state='广东', latitude=22.5431, longitude=114.0579, owm_city_id=1795565),
                City(name='成都', country='中国', state='四川', latitude=30.5728, longitude=104.0668, owm_city_id=1815286),
                City(name='哈尔滨', country='中国', state='黑龙江', latitude=45.8038, longitude=126.5350, owm_city_id=2037013),
                City(name='拉萨', country='中国', state='西藏', latitude=29.6500, longitude=91.1000, owm_city_id=1280737),
                City(name='乌鲁木齐', country='中国', state='新疆', latitude=43.8256, longitude=87.6168, owm_city_id=1529102),
            ]
            db.session.add_all(cities)
            db.session.commit()
            print("Default cities created")

    return app
