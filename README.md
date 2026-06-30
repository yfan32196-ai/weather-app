# 气象信息管理系统 🌤️

一个基于 Python Flask 的 Web 气象信息管理系统，支持气象数据的手动录入、API 实时获取、数据可视化、用户权限管理和数据导入导出。

## 功能特性

- 🔐 **用户认证与权限管理** — 三级角色（管理员 / 编辑者 / 观察者），Session 登录
- 📊 **数据可视化仪表板** — ECharts 图表：温度趋势、湿度变化、风速风向、气压变化
- 📝 **气象记录 CRUD** — 温度、湿度、气压、风速、风向、天气状况等完整字段
- 🌍 **多城市管理** — 城市经纬度、OpenWeatherMap ID 管理
- ☁️ **实时天气获取** — 对接 OpenWeatherMap API，一键获取并保存当前天气
- 📥 **数据导入** — 支持 Excel (.xlsx) 和 CSV 格式批量导入
- 📤 **数据导出** — 支持筛选条件导出为 Excel 或 CSV
- 📄 **分页与筛选** — 按城市、时间范围、数据来源灵活查询

## 技术栈

| 层级 | 技术 |
|------|------|
| 后端框架 | Python 3.11 + Flask 3.1 |
| 数据库 | SQLite (Flask-SQLAlchemy) |
| 认证 | Flask-Login (Session-based) |
| 前端 | Bootstrap 5.3 + jQuery + ECharts 5 |
| 导入导出 | pandas + openpyxl |
| 天气 API | OpenWeatherMap |

## 快速开始

### 1. 安装依赖

```bash
pip install -r requirements.txt
```

### 2. 初始化数据库（含演示数据）

```bash
python seed_data.py
```

### 3. 启动应用

```bash
python run.py
```

应用默认运行在 `http://localhost:5050`

### 4. 登录

| 角色 | 用户名 | 密码 |
|------|--------|------|
| 管理员 | admin | admin123 |
| 编辑者 | editor | editor123 |
| 观察者 | viewer | viewer123 |

## 配置天气 API（可选）

设置环境变量以启用实时天气获取功能：

```bash
export OWM_API_KEY=your_openweathermap_api_key
```

免费 API Key 可在 [OpenWeatherMap](https://openweathermap.org/api) 注册获取。

## 项目结构

```
fooo/
├── run.py                    # 应用入口
├── seed_data.py              # 种子数据脚本
├── requirements.txt          # Python 依赖
├── README.md
├── app/
│   ├── __init__.py           # Flask 工厂函数
│   ├── config.py             # 配置类
│   ├── extensions.py         # 扩展初始化
│   ├── models/               # 数据模型
│   │   ├── user.py           # 用户模型
│   │   ├── city.py           # 城市模型
│   │   ├── weather_record.py # 气象记录模型
│   │   └── import_log.py     # 导入日志模型
│   ├── routes/               # 路由与 API
│   │   ├── auth.py           # 认证 API
│   │   ├── main.py           # 页面路由
│   │   ├── api_users.py      # 用户管理 API
│   │   ├── api_cities.py     # 城市管理 API
│   │   ├── api_weather.py    # 气象记录 CRUD API
│   │   ├── api_fetch.py      # 天气获取 API
│   │   ├── api_stats.py      # 统计数据 API
│   │   └── api_import_export.py # 导入导出 API
│   ├── services/             # 业务逻辑
│   │   ├── weather_api.py    # OpenWeatherMap 客户端
│   │   ├── import_service.py # 文件导入服务
│   │   └── export_service.py # 文件导出服务
│   ├── templates/            # Jinja2 模板
│   ├── static/               # 静态文件 (CSS/JS)
│   └── utils/                # 工具函数 (装饰器)
└── uploads/                  # 临时上传目录
```

## 数据导入模板格式

导入文件（Excel/CSV）应包含以下列：

| 列名 | 必需 | 说明 |
|------|------|------|
| city_name | 是 | 城市名称（需在系统中已存在） |
| record_time | 是 | 时间，格式 YYYY-MM-DD HH:MM:SS |
| temperature | 否 | 温度 (°C) |
| humidity | 否 | 湿度 (0-100) |
| pressure | 否 | 气压 (hPa) |
| wind_speed | 否 | 风速 (m/s) |
| wind_direction | 否 | 风向 (N/NE/E/SE/S/SW/W/NW) |
| weather_desc | 否 | 天气描述 |

## 角色权限说明

| 功能 | 管理员 | 编辑者 | 观察者 |
|------|--------|--------|--------|
| 查看仪表板 | ✅ | ✅ | ✅ |
| 查看气象记录 | ✅ | ✅ | ✅ |
| 添加/编辑记录 | ✅ | ✅ | ❌ |
| 删除记录 | ✅ | ❌ | ❌ |
| 管理城市 | ✅ | ✅ | ❌ |
| 管理用户 | ✅ | ❌ | ❌ |
| 导入数据 | ✅ | ✅ | ❌ |
| 导出数据 | ✅ | ✅ | ✅ |
| 获取实时天气 | ✅ | ✅ | ❌ |
