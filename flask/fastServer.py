# Import FastAPI, HTTPException, and Request for building an API with FastAPI framework
from fastapi import FastAPI, HTTPException, Request
# Import JSONResponse to send custom JSON responses for certain requests
from fastapi.responses import JSONResponse
# Import BaseModel from Pydantic to define request models with type checking
from pydantic import BaseModel
# Import Optional and Dict from typing for optional fields and type hints
from typing import Optional, Dict

# Initialize the FastAPI app instance
app = FastAPI()

# Sample user data, a list of dictionaries with user IDs and names
users = [
    {"id": 1, "name": "lulu"},
    {"id": 2, "name": "chacha"},
]

# Define a model for a welcome message request body, with an optional welcome_message field
class WelcomeMessage(BaseModel):
    welcome_message: Optional[str] = None

# Define a model for a word-based request body, with a word field
class WordData(BaseModel):
    word: str

# Define a model for a letter-based request body, with a letter field
class LetterData(BaseModel):
    letter: str

# Initialize a welcome message string that can be modified via the API
welcome = "Welcome to 3ESE API! of lulu and chacha"

# Define a simple route at the root URL that returns a greeting message
@app.get("/")
def hello_world():
    return "Hello, World it's me again!\n"

# Define a route with multiple methods to manage the welcome message
@app.api_route("/api/welcome/", methods=["GET", "POST", "RETRIEVE", "DELETE"])
async def api_welcome(request: Request, data: Optional[WelcomeMessage] = None):
    global welcome
    
    # Handle a POST request to update the welcome message
    if request.method == 'POST':
        if data and data.welcome_message:
            welcome = data.welcome_message
            return {"message": "Welcome message updated."}
    
    # Handle a custom 'RETRIEVE' method to get the current welcome message
    elif request.method == 'RETRIEVE':
        return welcome
    
    # Handle DELETE requests to clear the welcome message
    elif request.method == 'DELETE':
        welcome = ""
        return {"message": "Welcome message deleted."}
    
    # Handle GET requests to return the current welcome message
    elif request.method == 'GET':
        return welcome

# Define a route to manage the welcome message at specific index positions
@app.api_route("/api/welcome/{index}", methods=["GET", "POST", "PUT", "PATCH", "DELETE"])
async def api_welcome_index(index: int, request: Request, word_data: Optional[WordData] = None, letter_data: Optional[LetterData] = None):
    global welcome
    
    # Check if the index is valid, and if not, raise a 404 error
    if index < 0 or index >= len(welcome):
        raise HTTPException(status_code=404, detail="Index out of range")
    
    # Handle GET request to retrieve the character at the specified index
    if request.method == 'GET':
        return {"index de la lettre": index, "lettre": welcome[index]}
    
    # Handle PUT request to insert a word starting at the specified index
    elif request.method == 'PUT':
        if word_data and word_data.word:
            welcome = welcome[:index] + word_data.word + welcome[index:]
            return {"mot": word_data.word, "a été inséré à l'index": index}
    
    # Handle PATCH request to replace a single character at the specified index
    elif request.method == 'PATCH':
        if letter_data and letter_data.letter:
            welcome = welcome[:index] + letter_data.letter + welcome[index:]
            return {"lettre à l'index": index, "à été remplacé par": welcome[index]}
    
    # Handle DELETE request to remove a character at the specified index
    elif request.method == 'DELETE':
        welcome = welcome[:index] + welcome[index + 1:]
        return {"message": f"lettre à l'index {index} a été supprimé"}

# Helper function to find a user by ID in the users list
def find_user(user_id: int):
    return next((user for user in users if user["id"] == user_id), None)

# Custom exception handler for 404 errors that returns a JSON response
@app.exception_handler(404)
async def page_not_found(request: Request, exc: HTTPException):
    return JSONResponse(status_code=404, content={"message": "Page not found"})

# Define a general-purpose request information route that logs request details
@app.api_route("/api/request/", methods=["GET", "POST"])
@app.api_route("/api/request/{path:path}", methods=["GET", "POST"])
async def api_request(request: Request, path: Optional[str] = None):
    # Prepare a response dictionary with request method, URL, path, query params, and headers
    resp = {
        "method": request.method,
        "url": str(request.url),
        "path": path,
        "args": dict(request.query_params),
        "headers": dict(request.headers),
    }
    # If the request is a POST, include the JSON body in the response
    if request.method == 'POST':
        resp["POST"] = {
            "data": await request.json(),
        }
    return resp

# Main entry point to run the FastAPI app using uvicorn, on localhost and port 5000
if __name__ == '__main__':
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=5000)
