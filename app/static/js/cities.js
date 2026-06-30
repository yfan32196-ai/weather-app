/**
 * Cities page - CRUD operations.
 */

let deleteCityId = null;

$(function() {
    loadCities();

    $('#btn-confirm-city-delete').on('click', function() {
        if (deleteCityId) {
            apiDelete('/api/cities/' + deleteCityId).then(function() {
                $('#deleteCityModal').modal('hide');
                showAlert('城市已删除', 'success');
                loadCities();
            }).catch(function(xhr) {
                var msg = xhr.responseJSON ? xhr.responseJSON.error : '删除失败';
                showAlert(msg, 'error');
            });
        }
    });
});

function loadCities() {
    apiGet('/api/cities').then(function(res) {
        renderCities(res.cities);
    });
}

function renderCities(cities) {
    var tbody = $('#cities-tbody');
    tbody.empty();

    if (!cities.length) {
        tbody.html('<tr><td colspan="8" class="text-center py-4 text-muted">暂无城市数据</td></tr>');
        return;
    }

    cities.forEach(function(c) {
        var actions = '';
        actions += '<button class="btn btn-sm btn-outline-secondary me-1" onclick="editCity(' + c.id + ')" title="编辑"><i class="bi bi-pencil"></i></button>';
        actions += '<button class="btn btn-sm btn-outline-danger" onclick="confirmDeleteCity(' + c.id + ')" title="删除"><i class="bi bi-trash"></i></button>';

        tbody.append(
            '<tr>' +
            '<td><strong>' + $('<div>').text(c.name).html() + '</strong></td>' +
            '<td>' + $('<div>').text(c.country).html() + '</td>' +
            '<td>' + $('<div>').text(c.state).html() + '</td>' +
            '<td>' + c.latitude.toFixed(4) + '</td>' +
            '<td>' + c.longitude.toFixed(4) + '</td>' +
            '<td>' + (c.owm_city_id || '-') + '</td>' +
            '<td><span class="badge bg-secondary">' + c.record_count + '</span></td>' +
            '<td>' + actions + '</td>' +
            '</tr>'
        );
    });
}

function clearCityForm() {
    $('#city-id').val('');
    $('#cityModalTitle').text('添加城市');
    $('#city-name').val('');
    $('#city-country').val('中国');
    $('#city-state').val('');
    $('#city-lat').val('');
    $('#city-lon').val('');
    $('#city-owm').val('');
    $('#city-notes').val('');
}

function editCity(id) {
    apiGet('/api/cities/' + id).then(function(res) {
        var c = res.city;
        $('#city-id').val(c.id);
        $('#cityModalTitle').text('编辑城市');
        $('#city-name').val(c.name);
        $('#city-country').val(c.country);
        $('#city-state').val(c.state);
        $('#city-lat').val(c.latitude);
        $('#city-lon').val(c.longitude);
        $('#city-owm').val(c.owm_city_id);
        $('#city-notes').val(c.notes);
        $('#cityModal').modal('show');
    });
}

function saveCity() {
    var id = $('#city-id').val();
    var name = $('#city-name').val().trim();
    if (!name) { showAlert('请输入城市名称', 'warning'); return; }

    var data = {
        name: name,
        country: $('#city-country').val().trim(),
        state: $('#city-state').val().trim(),
        latitude: parseFloat($('#city-lat').val()) || 0,
        longitude: parseFloat($('#city-lon').val()) || 0,
        owm_city_id: $('#city-owm').val() ? parseInt($('#city-owm').val()) : null,
        notes: $('#city-notes').val().trim(),
    };

    var url = id ? '/api/cities/' + id : '/api/cities';
    var method = id ? apiPut : apiPost;

    method(url, data).then(function(res) {
        $('#cityModal').modal('hide');
        showAlert(res.message, 'success');
        loadCities();
    }).catch(function(xhr) {
        var msg = xhr.responseJSON ? xhr.responseJSON.error : '保存失败';
        showAlert(msg, 'error');
    });
}

function confirmDeleteCity(id) {
    deleteCityId = id;
    $('#deleteCityModal').modal('show');
}
