# OpenWeatherMap API 调用说明

## 一、注册获取 API Key

1. 打开 https://openweathermap.org 注册账号
2. 登录后进入 https://home.openweathermap.org/api_keys
3. 复制默认生成的 API Key（免费版每分钟可调用60次）

## 二、API 接口说明

接口地址：https://api.openweathermap.org/data/2.5/weather

请求方式：GET

请求参数：

| 参数 | 说明 | 示例 |
|------|------|------|
| id | 城市ID（OpenWeatherMap内置） | 1816670（北京） |
| lat | 纬度（无城市ID时使用） | 39.9042 |
| lon | 经度（无城市ID时使用） | 116.4074 |
| appid | API Key | 你的密钥 |
| units | 温度单位 | metric（摄氏度） |
| lang | 返回语言 | zh_cn（中文） |

请求示例（获取北京天气）：
```
https://api.openweathermap.org/data/2.5/weather?id=1816670&appid=你的密钥&units=metric&lang=zh_cn
```

## 三、返回 JSON 数据解析

API 返回 JSON 格式数据，关键字段如下：

```json
{
  "dt": 1751376000,           // 观测时间戳（Unix秒）
  "main": {
    "temp": 30.17,            // 温度（摄氏度）
    "feels_like": 30.98,      // 体感温度
    "humidity": 48,           // 湿度（%）
    "pressure": 1006          // 气压（hPa）
  },
  "wind": {
    "speed": 2.3,             // 风速（m/s）
    "deg": 180,               // 风向角度（0-360）
    "gust": 2.98              // 阵风（m/s）
  },
  "weather": [{
    "main": "Clouds",         // 天气状况（英文）
    "description": "阴，多云"  // 天气描述（中文）
  }],
  "visibility": 10000,        // 能见度（米）
  "clouds": {"all": 100}      // 云量（%）
}
```

## 四、Python 调用代码（核心）

```python
import requests

class WeatherAPIService:
    BASE_URL = 'https://api.openweathermap.org/data/2.5/weather'

    def get_current_weather(self, city):
        # 1. 构建请求参数
        params = {
            'appid': '你的API_KEY',       # 从环境变量读取
            'units': 'metric',            # 摄氏度
            'lang': 'zh_cn',              # 中文描述
        }
        if city.owm_city_id:
            params['id'] = city.owm_city_id   # 优先用城市ID
        else:
            params['lat'] = city.latitude     # 否则用经纬度
            params['lon'] = city.longitude

        # 2. 发送 GET 请求
        resp = requests.get(self.BASE_URL, params=params, timeout=10)
        resp.raise_for_status()  # HTTP错误则抛异常

        # 3. 解析 JSON 返回数据
        data = resp.json()

        # 4. 风速角度转风向（如 180° -> "S"）
        wind_deg = data['wind'].get('deg', 0)
        directions = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW']
        wind_direction = directions[round(wind_deg / 45) % 8]

        # 5. 组装为系统内部结构
        weather_data = {
            'temperature':   data['main']['temp'],
            'feels_like':    data['main']['feels_like'],
            'humidity':      data['main']['humidity'],
            'pressure':      data['main']['pressure'],
            'wind_speed':    data['wind']['speed'],
            'wind_direction': wind_direction,
            'weather_main':  data['weather'][0]['main'],
            'weather_desc':  data['weather'][0]['description'],
            'visibility':    data.get('visibility'),
            'cloudiness':    data['clouds']['all'],
        }
        return weather_data
```

## 五、系统中完整调用流程

```
用户点击"获取实时天气" → 前端 POST /api/weather/fetch {city_id}
    → 后端 fetch_and_save() 函数
        → WeatherAPIService.get_current_weather(city)  // 调用API
        → 将返回数据存入 WeatherRecord 结构体
        → db.session.add(record)  // 写入数据库
        → 返回成功消息给前端
```

## 六、注意事项

1. 免费版每分钟最多60次调用，代码中做了缓存（同一城市10分钟内不重复请求）和频率限制（两次请求间隔至少1.2秒）
2. API Key 通过环境变量 OWM_API_KEY 传入，不硬编码在代码中
3. OpenWeatherMap 新注册的 Key 需要等待几分钟到几小时才能激活
