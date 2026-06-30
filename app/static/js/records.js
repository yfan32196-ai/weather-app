/**
 * Records page - CRUD, search, and pagination.
 */

let currentPage = 1;
let deleteRecordId = null;

$(function() {
    // Load filter dropdowns
    apiGet('/api/cities').then(function(res) {
        res.cities.forEach(function(c) {
            $('#filter-city').append('<option value="' + c.id + '">' + c.name + '</option>');
            $('#record-city').append('<option value="' + c.id + '">' + c.name + '</option>');
            $('#fetch-city').append('<option value="' + c.id + '">' + c.name + '</option>');
        });
    });

    // Set default datetime to now
    var now = new Date();
    now.setMinutes(now.getMinutes() - now.getTimezoneOffset());
    $('#record-time').val(now.toISOString().slice(0, 16));

    // Load initial data
    loadRecords(1);

    // Delete confirmation
    $('#btn-confirm-delete').on('click', function() {
        if (deleteRecordId) {
            apiDelete('/api/weather/records/' + deleteRecordId).then(function() {
                $('#deleteModal').modal('hide');
                showAlert('记录已删除', 'success');
                loadRecords(currentPage);
            }).catch(function(xhr) {
                var msg = xhr.responseJSON ? xhr.responseJSON.error : '删除失败';
                showAlert(msg, 'error');
            });
        }
    });
});

function loadRecords(page) {
    currentPage = page;
    var params = {
        page: page,
        per_page: $('#filter-per-page').val()
    };
    var cityId = $('#filter-city').val();
    var dateFrom = $('#filter-date-from').val();
    var dateTo = $('#filter-date-to').val();
    var source = $('#filter-source').val();

    if (cityId) params.city_id = cityId;
    if (dateFrom) params.date_from = dateFrom;
    if (dateTo) params.date_to = dateTo;
    if (source) params.source = source;

    apiGet('/api/weather/records', params).then(function(res) {
        renderTable(res.records);
        renderPagination(res);
    }).catch(function() {
        $('#records-tbody').html('<tr><td colspan="8" class="text-center py-4 text-danger">加载失败</td></tr>');
    });
}

function renderTable(records) {
    var tbody = $('#records-tbody');
    tbody.empty();

    if (!records.length) {
        tbody.html('<tr><td colspan="8" class="text-center py-4 text-muted">暂无气象记录</td></tr>');
        return;
    }

    records.forEach(function(r) {
        var isAdmin = $('#records-tbody').data('admin');
        var actions = '';
        actions += '<button class="btn btn-sm btn-outline-secondary me-1" onclick="editRecord(' + r.id + ')" title="编辑"><i class="bi bi-pencil"></i></button>';
        actions += '<button class="btn btn-sm btn-outline-danger" onclick="confirmDeleteRecord(' + r.id + ')" title="删除"><i class="bi bi-trash"></i></button>';

        tbody.append(
            '<tr>' +
            '<td>' + $('<div>').text(r.city_name).html() + '</td>' +
            '<td class="small">' + formatDateTime(r.record_time) + '</td>' +
            '<td>' + formatTemp(r.temperature) + '</td>' +
            '<td>' + formatHumidity(r.humidity) + '</td>' +
            '<td>' + (r.wind_speed != null ? r.wind_speed + ' m/s' : '-') + '</td>' +
            '<td>' + (r.weather_desc || r.weather_main || '-') + '</td>' +
            '<td>' + sourceBadge(r.data_source) + '</td>' +
            '<td>' + actions + '</td>' +
            '</tr>'
        );
    });
}

function renderPagination(res) {
    $('#pagination-info').text('共 ' + res.total + ' 条，第 ' + res.page + '/' + res.pages + ' 页');
    var ul = $('#pagination-ul');
    ul.empty();

    if (res.pages <= 1) return;

    // Previous
    ul.append('<li class="page-item' + (res.page <= 1 ? ' disabled' : '') + '"><a class="page-link" href="#" onclick="loadRecords(' + (res.page - 1) + ')">«</a></li>');

    // Page numbers (show max 7)
    var start = Math.max(1, res.page - 3);
    var end = Math.min(res.pages, start + 6);
    start = Math.max(1, end - 6);

    for (var i = start; i <= end; i++) {
        ul.append('<li class="page-item' + (i === res.page ? ' active' : '') + '"><a class="page-link" href="#" onclick="loadRecords(' + i + ')">' + i + '</a></li>');
    }

    // Next
    ul.append('<li class="page-item' + (res.page >= res.pages ? ' disabled' : '') + '"><a class="page-link" href="#" onclick="loadRecords(' + (res.page + 1) + ')">»</a></li>');
}

// --- Record CRUD ---

function clearRecordForm() {
    $('#record-id').val('');
    $('#recordModalTitle').text('添加气象记录');
    $('#record-city').val('');
    $('#record-temp').val('');
    $('#record-feels-like').val('');
    $('#record-humidity').val('');
    $('#record-pressure').val('');
    $('#record-wind-speed').val('');
    $('#record-wind-dir').val('');
    $('#record-wind-gust').val('');
    $('#record-weather-main').val('');
    $('#record-weather-desc').val('');
    $('#record-notes').val('');
}

function editRecord(id) {
    apiGet('/api/weather/records/' + id).then(function(res) {
        var r = res.record;
        $('#record-id').val(r.id);
        $('#recordModalTitle').text('编辑气象记录');
        $('#record-city').val(r.city_id);
        if (r.record_time) {
            var dt = new Date(r.record_time);
            dt.setMinutes(dt.getMinutes() - dt.getTimezoneOffset());
            $('#record-time').val(dt.toISOString().slice(0, 16));
        }
        $('#record-temp').val(r.temperature);
        $('#record-feels-like').val(r.feels_like);
        $('#record-humidity').val(r.humidity);
        $('#record-pressure').val(r.pressure);
        $('#record-wind-speed').val(r.wind_speed);
        $('#record-wind-dir').val(r.wind_direction);
        $('#record-wind-gust').val(r.wind_gust);
        $('#record-weather-main').val(r.weather_main);
        $('#record-weather-desc').val(r.weather_desc);
        $('#record-notes').val(r.notes);
        $('#recordModal').modal('show');
    });
}

function saveRecord() {
    var id = $('#record-id').val();
    var cityId = $('#record-city').val();
    if (!cityId) { showAlert('请选择城市', 'warning'); return; }

    var data = {
        city_id: parseInt(cityId),
        record_time: $('#record-time').val() || null,
        temperature: parseFloatOrNull($('#record-temp').val()),
        feels_like: parseFloatOrNull($('#record-feels-like').val()),
        humidity: parseFloatOrNull($('#record-humidity').val()),
        pressure: parseFloatOrNull($('#record-pressure').val()),
        wind_speed: parseFloatOrNull($('#record-wind-speed').val()),
        wind_direction: $('#record-wind-dir').val(),
        wind_gust: parseFloatOrNull($('#record-wind-gust').val()),
        weather_main: $('#record-weather-main').val(),
        weather_desc: $('#record-weather-desc').val(),
        notes: $('#record-notes').val(),
    };

    var url = id ? '/api/weather/records/' + id : '/api/weather/records';
    var method = id ? apiPut : apiPost;

    method(url, data).then(function(res) {
        $('#recordModal').modal('hide');
        showAlert(res.message, 'success');
        loadRecords(currentPage);
    }).catch(function(xhr) {
        var msg = xhr.responseJSON ? xhr.responseJSON.error : '保存失败';
        showAlert(msg, 'error');
    });
}

function confirmDeleteRecord(id) {
    deleteRecordId = id;
    $('#deleteModal').modal('show');
}

// --- Fetch Weather ---

function fetchWeather() {
    var cityId = $('#fetch-city').val();
    if (!cityId) { showAlert('请选择城市', 'warning'); return; }

    var btn = $('#fetchModal').find('.btn-primary');
    var origText = btn.html();
    btn.prop('disabled', true).html('<span class="spinner-border spinner-border-sm me-2"></span>获取中...');
    $('#fetch-result').addClass('d-none');

    apiPost('/api/weather/fetch', { city_id: parseInt(cityId) }).then(function(res) {
        $('#fetch-result').removeClass('d-none alert-danger')
            .addClass('alert alert-success')
            .html('<i class="bi bi-check-circle me-2"></i>' + res.message);
        $('#fetchModal').modal('hide');
        showAlert(res.message, 'success');
        loadRecords(currentPage);
    }).catch(function(xhr) {
        var msg = xhr.responseJSON ? xhr.responseJSON.error : '获取失败';
        $('#fetch-result').removeClass('d-none alert-success')
            .addClass('alert alert-danger')
            .html('<i class="bi bi-exclamation-triangle me-2"></i>' + msg);
        btn.prop('disabled', false).html(origText);
    });
}

// --- Helpers ---

function parseFloatOrNull(val) {
    if (val === '' || val === null || val === undefined) return null;
    var n = parseFloat(val);
    return isNaN(n) ? null : n;
}
