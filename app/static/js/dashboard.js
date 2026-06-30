/**
 * Dashboard - ECharts initialization and data fetching.
 */

let tempChart, humidityChart, windChart, pressureChart;

$(function() {
    // Load city dropdown
    apiGet('/api/cities').then(function(res) {
        res.cities.forEach(function(c) {
            $('#city-select').append('<option value="' + c.id + '">' + c.name + '</option>');
        });
        refreshDashboard();
    });

    // Bind filter changes
    $('#city-select, #days-select').on('change', function() {
        refreshDashboard();
    });

    // Initialize charts
    initCharts();
});

function initCharts() {
    tempChart = echarts.init(document.getElementById('chart-temperature'));
    windChart = echarts.init(document.getElementById('chart-wind'));
    humidityChart = echarts.init(document.getElementById('chart-humidity'));
    pressureChart = echarts.init(document.getElementById('chart-pressure'));

    // Resize on window resize
    $(window).on('resize', function() {
        tempChart.resize();
        windChart.resize();
        humidityChart.resize();
        pressureChart.resize();
    });
}

function refreshDashboard() {
    var cityId = $('#city-select').val();
    var days = $('#days-select').val();

    loadSummary(cityId, days);
    loadTemperatureChart(cityId, days);
    loadHumidityChart(cityId, days);
    loadWindChart(cityId, days);
    loadPressureChart(cityId, days);
}

function loadSummary(cityId, days) {
    var params = { days: days };
    if (cityId) params.city_id = cityId;

    apiGet('/api/stats/summary', params).then(function(res) {
        var s = res.summary;
        $('#stat-avg-temp').text(s.avg_temperature != null ? s.avg_temperature + '°C' : '--°C');
        $('#stat-max-temp').text(s.max_temperature != null ? s.max_temperature + '°C' : '--°C');
        $('#stat-min-temp').text(s.min_temperature != null ? s.min_temperature + '°C' : '--°C');
        $('#stat-avg-humidity').text(s.avg_humidity != null ? s.avg_humidity + '%' : '--%');
    });
}

function loadTemperatureChart(cityId, days) {
    var params = { days: days };
    if (cityId) params.city_id = cityId;

    apiGet('/api/stats/temperature', params).then(function(res) {
        var times = res.data.map(function(d) {
            var t = new Date(d.time);
            return t.toLocaleString('zh-CN', { month: 'short', day: 'numeric', hour: '2-digit' });
        });
        var temps = res.data.map(function(d) { return d.temperature; });
        var feels = res.data.map(function(d) { return d.feels_like; });

        tempChart.setOption({
            tooltip: { trigger: 'axis' },
            legend: { data: ['温度', '体感温度'], bottom: 0 },
            grid: { left: 50, right: 30, top: 20, bottom: 40 },
            xAxis: { type: 'category', data: times, axisLabel: { rotate: 45, fontSize: 10 } },
            yAxis: { type: 'value', name: '°C' },
            series: [
                { name: '温度', type: 'line', data: temps, smooth: true, lineStyle: { width: 2 }, itemStyle: { color: '#e74c3c' } },
                { name: '体感温度', type: 'line', data: feels, smooth: true, lineStyle: { type: 'dashed' }, itemStyle: { color: '#f39c12' } }
            ]
        });
    });
}

function loadHumidityChart(cityId, days) {
    var params = { days: days };
    if (cityId) params.city_id = cityId;

    apiGet('/api/stats/humidity', params).then(function(res) {
        var times = res.data.map(function(d) {
            var t = new Date(d.time);
            return t.toLocaleString('zh-CN', { month: 'short', day: 'numeric', hour: '2-digit' });
        });
        var humidity = res.data.map(function(d) { return d.humidity; });
        var pressure = res.data.map(function(d) { return d.pressure; });

        humidityChart.setOption({
            tooltip: { trigger: 'axis' },
            legend: { data: ['湿度', '气压'], bottom: 0 },
            grid: { left: 50, right: 50, top: 20, bottom: 40 },
            xAxis: { type: 'category', data: times, axisLabel: { rotate: 45, fontSize: 10 } },
            yAxis: [
                { type: 'value', name: '%', min: 0, max: 100 },
                { type: 'value', name: 'hPa' }
            ],
            series: [
                { name: '湿度', type: 'bar', data: humidity, itemStyle: { color: '#3498db' }, yAxisIndex: 0 },
                { name: '气压', type: 'line', data: pressure, smooth: true, itemStyle: { color: '#9b59b6' }, yAxisIndex: 1 }
            ]
        });
    });
}

function loadWindChart(cityId, days) {
    var params = { days: days };
    if (cityId) params.city_id = cityId;

    apiGet('/api/stats/wind', params).then(function(res) {
        // Aggregate wind direction counts
        var dirCount = {};
        var dirSpeed = {};
        res.data.forEach(function(d) {
            var dir = d.wind_direction || 'N';
            dirCount[dir] = (dirCount[dir] || 0) + 1;
            if (d.wind_speed) {
                dirSpeed[dir] = (dirSpeed[dir] || 0) + d.wind_speed;
            }
        });

        var dirs = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
        var labels = ['北', '东北', '东', '东南', '南', '西南', '西', '西北'];
        var counts = dirs.map(function(d) { return dirCount[d] || 0; });
        var speeds = dirs.map(function(d) {
            return dirSpeed[d] ? (dirSpeed[d] / (dirCount[d] || 1)).toFixed(1) : 0;
        });

        windChart.setOption({
            tooltip: { trigger: 'axis' },
            legend: { data: ['观测次数', '平均风速(m/s)'], bottom: 0 },
            grid: { left: 50, right: 50, top: 20, bottom: 40 },
            xAxis: { type: 'category', data: labels },
            yAxis: [
                { type: 'value', name: '次数' },
                { type: 'value', name: 'm/s' }
            ],
            series: [
                { name: '观测次数', type: 'bar', data: counts, itemStyle: { color: '#1abc9c' }, yAxisIndex: 0 },
                { name: '平均风速(m/s)', type: 'line', data: speeds, smooth: true, itemStyle: { color: '#e67e22' }, yAxisIndex: 1 }
            ]
        });
    });
}

function loadPressureChart(cityId, days) {
    var params = { days: days };
    if (cityId) params.city_id = cityId;

    apiGet('/api/stats/humidity', params).then(function(res) {
        var times = res.data.map(function(d) {
            var t = new Date(d.time);
            return t.toLocaleString('zh-CN', { month: 'short', day: 'numeric', hour: '2-digit' });
        });
        var pressure = res.data.map(function(d) { return d.pressure; });

        pressureChart.setOption({
            tooltip: { trigger: 'axis' },
            grid: { left: 60, right: 30, top: 20, bottom: 40 },
            xAxis: { type: 'category', data: times, axisLabel: { rotate: 45, fontSize: 10 } },
            yAxis: { type: 'value', name: 'hPa' },
            series: [{
                name: '气压',
                type: 'line',
                data: pressure,
                smooth: true,
                areaStyle: { opacity: 0.3 },
                itemStyle: { color: '#9b59b6' }
            }]
        });
    });
}
