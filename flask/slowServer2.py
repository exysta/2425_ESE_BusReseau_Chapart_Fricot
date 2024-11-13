from flask import Flask, jsonify, request, abort

app = Flask(__name__)

# Stockage des données pour temp, pres, scale
temperatures = []
pressures = []
scale = None  # K

# --- Température ---

@app.route('/temp/', methods=['POST'])
def create_temperature():
    data = request.get_json()
    if "value" not in data:
        return jsonify({"error": "No temperature value provided"}), 400
    temperatures.append(data["value"])
    return jsonify({"message": "Temperature added", "value": data["value"]}), 201

@app.route('/temp/', methods=['GET'])
def get_temperatures():
    return jsonify({"temperatures": temperatures})

@app.route('/temp/<int:index>', methods=['GET'])
def get_temperature(index):
    if index < 0 or index >= len(temperatures):
        abort(404)
    return jsonify({"temperature": temperatures[index]})

@app.route('/temp/<int:index>', methods=['DELETE'])
def delete_temperature(index):
    if index < 0 or index >= len(temperatures):
        abort(404)
    deleted = temperatures.pop(index)
    return jsonify({"message": f"Temperature at index {index} deleted", "deleted_value": deleted}), 200

# --- Pression ---

@app.route('/pres/', methods=['POST'])
def create_pressure():
    data = request.get_json()
    if "value" not in data:
        return jsonify({"error": "No pressure value provided"}), 400
    pressures.append(data["value"])
    return jsonify({"message": "Pressure added", "value": data["value"]}), 201

@app.route('/pres/', methods=['GET'])
def get_pressures():
    return jsonify({"pressures": pressures})

@app.route('/pres/<int:index>', methods=['GET'])
def get_pressure(index):
    if index < 0 or index >= len(pressures):
        abort(404)
    return jsonify({"pressure": pressures[index]})

@app.route('/pres/<int:index>', methods=['DELETE'])
def delete_pressure(index):
    if index < 0 or index >= len(pressures):
        abort(404)
    deleted = pressures.pop(index)
    return jsonify({"message": f"Pressure at index {index} deleted", "deleted_value": deleted}), 200

# --- Scale (K) ---

@app.route('/scale/', methods=['GET'])
def get_scale():
    if scale is None:
        return jsonify({"error": "Scale not set"}), 404
    return jsonify({"scale": scale})

@app.route('/scale/<int:index>', methods=['POST'])
def set_scale(index):
    global scale
    data = request.get_json()
    if "value" not in data:
        return jsonify({"error": "No scale value provided"}), 400
    scale = data["value"]
    return jsonify({"message": f"Scale set to {scale} for index {index}"}), 200

# --- Angle ---

@app.route('/angle/', methods=['GET'])
def get_angle():
    if not temperatures or scale is None:
        return jsonify({"error": "Insufficient data to calculate angle"}), 400
    angle = sum(temperatures) * scale
    return jsonify({"angle": angle})

# Gestion des erreurs pour les routes non trouvées
@app.errorhandler(404)
def page_not_found(error):
    return jsonify({"message": "Resource not found"}), 404

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)