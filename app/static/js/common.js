/**
 * Common JavaScript utilities for the Weather Information Management System.
 */

// --- AJAX helpers ---

function apiGet(url, params) {
    return $.ajax({
        url: url,
        method: 'GET',
        data: params,
        dataType: 'json'
    });
}

function apiPost(url, data) {
    return $.ajax({
        url: url,
        method: 'POST',
        contentType: 'application/json',
        data: JSON.stringify(data),
        dataType: 'json'
    });
}

function apiPut(url, data) {
    return $.ajax({
        url: url,
        method: 'PUT',
        contentType: 'application/json',
        data: JSON.stringify(data),
        dataType: 'json'
    });
}

function apiDelete(url) {
    return $.ajax({
        url: url,
        method: 'DELETE',
        dataType: 'json'
    });
}

// --- Flash message helper ---

function showAlert(message, type) {
    type = type || 'info';
    var alertClass = {
        'success': 'alert-success',
        'error': 'alert-danger',
        'warning': 'alert-warning',
        'info': 'alert-info'
    }[type] || 'alert-info';

    var html = '<div class="alert ' + alertClass + ' alert-dismissible fade show" role="alert">' +
        message +
        '<button type="button" class="btn-close" data-bs-dismiss="alert"></button>' +
        '</div>';

    var container = $('#flash-container');
    if (!container.length) {
        container = $('<div id="flash-container"></div>');
        $('main').prepend(container);
    }
    container.append(html);

    // Auto dismiss after 5 seconds
    setTimeout(function() {
        container.find('.alert').last().fadeOut(500, function() { $(this).remove(); });
    }, 5000);
}

// --- Logout ---

function logout() {
    apiPost('/api/auth/logout').then(function() {
        window.location.href = '/login';
    }).catch(function() {
        window.location.href = '/login';
    });
}

// --- Select population helper ---

function populateSelect(selector, url, valueField, textField, selectedValue) {
    return apiGet(url).then(function(res) {
        var $select = $(selector);
        $select.empty();
        $select.append('<option value="">-- 请选择 --</option>');
        var items = res.cities || res.users || res.records || res;
        if (!Array.isArray(items)) items = [];
        items.forEach(function(item) {
            var val = item[valueField];
            var txt = item[textField];
            var selected = (selectedValue !== undefined && val == selectedValue) ? ' selected' : '';
            $select.append('<option value="' + val + '"' + selected + '>' + txt + '</option>');
        });
    });
}

// --- Format helpers ---

function formatDateTime(isoStr) {
    if (!isoStr) return '-';
    var d = new Date(isoStr);
    return d.toLocaleString('zh-CN');
}

function formatTemp(val) {
    if (val == null || val === '') return '-';
    return parseFloat(val).toFixed(1) + '°C';
}

function formatHumidity(val) {
    if (val == null || val === '') return '-';
    return val + '%';
}

function sourceBadge(source) {
    var map = {
        'manual': '<span class="badge badge-source-manual">手动录入</span>',
        'api_openweather': '<span class="badge badge-source-api_openweather">API获取</span>',
        'api_hefeng': '<span class="badge badge-source-api_openweather">和风天气</span>',
        'import': '<span class="badge badge-source-import">批量导入</span>'
    };
    return map[source] || '<span class="badge bg-secondary">' + source + '</span>';
}
