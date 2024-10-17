from flask import Flask,jsonify, render_template,abort,request
import json

app = Flask(__name__)

users = [
    {"id": 1, "name": "lulu"},
    {"id": 2, "name": "chacha"},
]

@app.route('/')
def hello_world():
    return "Hello, World it's me again!\n"

welcome = "Welcome to 3ESE API! of lulu and chacha"

@app.route('/api/welcome/',methods=['GET','POST','RETRIEVE','DELETE'])
def api_welcome():
    global welcome  # Declare 'welcome' as global
    
    if request.method == 'POST':
        data = request.get_json()
        if "welcome_message" in data:  # Check if "welcome" is in the incoming JSON
            welcome = data["welcome_message"]  # Update the global 'welcome'
            return jsonify({"message": "Welcome message updated."}),200 #200 is the HTTP status code for OK

    elif request.method == 'RETRIEVE':
        return welcome
    elif request.method == 'DELETE':
        welcome =""
        return jsonify({"message": "Welcome message deleted."}), 200

    elif request.method == 'GET':
        return welcome
    
@app.route('/api/welcome/<int:index>' ,methods=['GET','POST','PUT','PATCH','DELETE'])
def api_welcome_index(index):
    global welcome  # Declare 'welcome' as global

#redirection erreur 404
    if index < 0 or index >= len(welcome):
        abort(404)

    elif request.method == 'GET':
        return jsonify({"index de la lettre": index, "lettre": welcome[index]}), 200
    
    elif request.method == 'PUT':
        data = request.get_json()
        if "word" in data:  # Check if "word" is in the incoming JSON
            welcome = welcome[:index] + data["word"] + welcome[index:] 
            return jsonify({"mot ": data["word"], "a été inséré à l'index"  : index }), 200

    elif request.method == 'PATCH':
        data = request.get_json()
        if "letter" in data:  # Check if "word" is in the incoming JSON
            welcome = welcome[:index] + data["letter"] + welcome[index:] 
            return jsonify({"lettre à l'index ": index, "à été remplacé par" : welcome[index] }), 200
    elif request.method == 'DELETE':
        welcome = welcome[:index] + welcome[index + 1:] 
        return "lettre à l'index " + str(index) + " a été supprimé", 200

    #solution 1
    #return json.dumps({"index": index, "val": welcome[index]}), {"Content-Type": "application/json"}
    #solution 2
    #message = {"index": index, "val": welcome[index]}
    #return jsonify({'message': message})

# Helper function to find a user by ID
def find_user(user_id):
    return next((user for user in users if user["id"] == user_id), None)


@app.errorhandler(404)
def page_not_found(error):
    return render_template('page_not_found.html'), 404

@app.route('/api/request/', methods=['GET', 'POST'])
@app.route('/api/request/<path>', methods=['GET','POST'])
def api_request(path=None):
    resp = {
            "method":   request.method,
            "url" :  request.url,
            "path" : path,
            "args": request.args,
            "headers": dict(request.headers),
    }
    if request.method == 'POST':
        resp["POST"] = {
                "data" : request.get_json(),
                }
    return jsonify(resp)

if __name__ == '__main__':
    app.run(debug=True)  # Ensure this line is included