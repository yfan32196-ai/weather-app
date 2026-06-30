from flask_sqlalchemy import SQLAlchemy
from flask_login import LoginManager
from flask_migrate import Migrate

db = SQLAlchemy()
login_manager = LoginManager()
migrate = Migrate()

login_manager.login_view = 'main.login_page'
login_manager.login_message = '请先登录以访问此页面。'
login_manager.login_message_category = 'warning'
