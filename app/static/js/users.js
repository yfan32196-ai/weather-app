/**
 * Users page - CRUD operations (admin only).
 */

let deleteUserId = null;

$(function() {
    loadUsers();

    $('#btn-confirm-user-delete').on('click', function() {
        if (deleteUserId) {
            apiDelete('/api/users/' + deleteUserId).then(function() {
                $('#deleteUserModal').modal('hide');
                showAlert('用户已删除', 'success');
                loadUsers();
            }).catch(function(xhr) {
                var msg = xhr.responseJSON ? xhr.responseJSON.error : '删除失败';
                showAlert(msg, 'error');
            });
        }
    });
});

function loadUsers() {
    apiGet('/api/users').then(function(res) {
        renderUsers(res.users);
    });
}

function renderUsers(users) {
    var tbody = $('#users-tbody');
    tbody.empty();

    if (!users.length) {
        tbody.html('<tr><td colspan="6" class="text-center py-4 text-muted">暂无用户</td></tr>');
        return;
    }

    var roleBadges = {
        'admin': '<span class="badge bg-warning text-dark">管理员</span>',
        'editor': '<span class="badge bg-info text-dark">编辑者</span>',
        'viewer': '<span class="badge bg-secondary">观察者</span>',
    };

    users.forEach(function(u) {
        var actions = '';
        actions += '<button class="btn btn-sm btn-outline-secondary me-1" onclick="editUser(' + u.id + ')" title="编辑"><i class="bi bi-pencil"></i></button>';
        actions += '<button class="btn btn-sm btn-outline-danger" onclick="confirmDeleteUser(' + u.id + ')" title="删除"><i class="bi bi-trash"></i></button>';

        tbody.append(
            '<tr>' +
            '<td><strong>' + $('<div>').text(u.username).html() + '</strong></td>' +
            '<td>' + $('<div>').text(u.email).html() + '</td>' +
            '<td>' + (roleBadges[u.role] || u.role) + '</td>' +
            '<td>' + (u.is_active ? '<span class="text-success">● 激活</span>' : '<span class="text-danger">● 禁用</span>') + '</td>' +
            '<td class="small">' + formatDateTime(u.created_at) + '</td>' +
            '<td>' + actions + '</td>' +
            '</tr>'
        );
    });
}

function clearUserForm() {
    $('#user-id').val('');
    $('#userModalTitle').text('添加用户');
    $('#user-username').val('');
    $('#user-password').val('');
    $('#password-hint').text('');
    $('#user-email').val('');
    $('#user-role').val('viewer');
    $('#user-active').prop('checked', true);
}

function editUser(id) {
    apiGet('/api/users').then(function(res) {
        var u = res.users.find(function(u) { return u.id === id; });
        if (!u) return;

        $('#user-id').val(u.id);
        $('#userModalTitle').text('编辑用户');
        $('#user-username').val(u.username);
        $('#user-password').val('');
        $('#password-hint').text('（留空则保持原密码）');
        $('#user-email').val(u.email);
        $('#user-role').val(u.role);
        $('#user-active').prop('checked', u.is_active);
        $('#userModal').modal('show');
    });
}

function saveUser() {
    var id = $('#user-id').val();
    var username = $('#user-username').val().trim();
    var password = $('#user-password').val();
    var role = $('#user-role').val();

    if (!username) { showAlert('请输入用户名', 'warning'); return; }
    if (!id && !password) { showAlert('请输入密码', 'warning'); return; }

    var data = {
        username: username,
        role: role,
        email: $('#user-email').val().trim(),
        is_active: $('#user-active').is(':checked'),
    };
    if (password) data.password = password;

    var url = id ? '/api/users/' + id : '/api/users';
    var method = id ? apiPut : apiPost;

    method(url, data).then(function(res) {
        $('#userModal').modal('hide');
        showAlert(res.message, 'success');
        loadUsers();
    }).catch(function(xhr) {
        var msg = xhr.responseJSON ? xhr.responseJSON.error : '保存失败';
        showAlert(msg, 'error');
    });
}

function confirmDeleteUser(id) {
    deleteUserId = id;
    $('#deleteUserModal').modal('show');
}
