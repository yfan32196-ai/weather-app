from flask import Blueprint, request, jsonify
from flask_login import login_required

from app.services.c_analysis import run_c_analysis

api_c_analysis_bp = Blueprint('api_c_analysis', __name__, url_prefix='/api/analysis')


@api_c_analysis_bp.route('/c', methods=['GET'])
@login_required
def c_analysis():
    """运行 C 语言气象分析引擎并返回结果。"""
    city_id = request.args.get('city_id', type=int)
    result = run_c_analysis(city_id=city_id)
    return jsonify(result)
